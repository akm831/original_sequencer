#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "original_sequencer/prototype/AudioCore.h"
#include "original_sequencer/prototype/ScheduledTriggerInput.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

namespace {

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer {
public:
    MainComponent() {
        title.setText("JUCE/C++ — P3 scheduled trigger", juce::dontSendNotification);
        title.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(title);

        diagnostics.setJustificationType(juce::Justification::centred);
        diagnostics.setText("Audio device: starting...", juce::dontSendNotification);
        addAndMakeVisible(diagnostics);

        triggerButton.setButtonText("Trigger 50 ms test burst");
        triggerButton.onClick = [this] {
            bool accepted = false;
            {
                std::lock_guard<std::mutex> lock(commandMutex);
                accepted = audioPrepared && triggerInput.submit(37, 1.0F);
            }
            triggerStatus.setText(accepted ? "Trigger accepted" : "Trigger rejected: audio unavailable, queue full, or earlier timestamp",
                                  juce::dontSendNotification);
        };
        addAndMakeVisible(triggerButton);
        addAndMakeVisible(triggerStatus);
        setSize(640, 600);

        setAudioChannels(0, 2);
        startTimerHz(5);
    }

    ~MainComponent() override {
        stopTimer();
        shutdownAudio();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        std::lock_guard<std::mutex> lock(commandMutex);
        const auto maxCallbackFrames = static_cast<std::uint32_t>(
            std::max(samplesPerBlockExpected, 1));

        if (audioWasPrepared.exchange(true, std::memory_order_relaxed)) {
            audioRestartCount.fetch_add(1, std::memory_order_relaxed);
        }

        actualSampleRate.store(sampleRate, std::memory_order_relaxed);
        actualCallbackFrames.store(0, std::memory_order_relaxed);
        callbackStartFrame = 0;
        audioCore.initialize(sampleRate, maxCallbackFrames);
        triggerInput.reset();
        audioPrepared = true;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
        if (bufferToFill.buffer == nullptr) {
            return;
        }

        const auto callbackBegin = std::chrono::steady_clock::now();

        const auto frameCount = static_cast<std::uint32_t>(
            std::max(bufferToFill.numSamples, 0));
        const auto channelCount = static_cast<std::uint32_t>(
            std::max(bufferToFill.buffer->getNumChannels(), 0));

        actualCallbackFrames.store(frameCount, std::memory_order_relaxed);
        audioCore.renderPlanar(bufferToFill.buffer->getArrayOfWritePointers(),
                              frameCount, channelCount, callbackStartFrame,
                              static_cast<std::uint32_t>(bufferToFill.startSample));
        const auto durationUs = std::chrono::duration<double, std::micro>(
            std::chrono::steady_clock::now() - callbackBegin).count();
        audioCore.recordCallbackTiming(callbackStartFrame, frameCount, durationUs);
        callbackStartFrame += frameCount;
    }

    void releaseResources() override {
        std::lock_guard<std::mutex> lock(commandMutex);
        audioPrepared = false;
        audioCore.shutdown();
        triggerInput.reset();
        actualCallbackFrames.store(0, std::memory_order_relaxed);
        actualSampleRate.store(0.0, std::memory_order_relaxed);
        callbackStartFrame = 0;
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(24);
        title.setBounds(bounds.removeFromTop(72));
        diagnostics.setBounds(bounds.removeFromTop(300));
        triggerButton.setBounds(bounds.removeFromTop(48));
        triggerStatus.setBounds(bounds.removeFromTop(72));
    }

private:
    void timerCallback() override {
        const auto snapshot = audioCore.diagnostics();
        diagnostics.setText(
            "sampleRate: "
                + juce::String(actualSampleRate.load(std::memory_order_relaxed), 1)
                + " Hz\ncallbackFrames: "
                + juce::String(actualCallbackFrames.load(std::memory_order_relaxed))
                + " (min " + juce::String(snapshot.callbackFramesMin)
                + " / max " + juce::String(snapshot.callbackFramesMax) + ")"
                + "\ncallbackDurationUs: " + juce::String(snapshot.callbackDurationUs, 2)
                + "\ncallbackStartFrame: " + juce::String(static_cast<juce::int64>(snapshot.callbackStartFrame))
                + "\naudioRestartCount: "
                + juce::String(audioRestartCount.load(std::memory_order_relaxed))
                + "\ncallbackLoad: " + juce::String(snapshot.callbackLoad * 100.0, 2) + "%"
                + "\nload P95 / P99 / Peak: " + juce::String(snapshot.callbackLoadP95 * 100.0, 1)
                + " / " + juce::String(snapshot.callbackLoadP99 * 100.0, 1)
                + " / " + juce::String(snapshot.callbackLoadPeak * 100.0, 2) + "%"
                + "\nrenderedFrames: " + juce::String(static_cast<juce::int64>(snapshot.renderedFrames))
                + "\nqueueDepth / highWater / overflow: " + juce::String(snapshot.queueDepth)
                + " / " + juce::String(snapshot.queueHighWaterMark)
                + " / " + juce::String(static_cast<juce::int64>(snapshot.queueOverflowCount))
                + "\ntriggerCount / offset: " + juce::String(static_cast<juce::int64>(snapshot.triggerCount))
                + " / " + juce::String(snapshot.lastTriggerOffset)
                + "\ntestBurst: 50 ms / 220 Hz @ max 8%",
            juce::dontSendNotification);
    }

    juce::Label title;
    juce::Label diagnostics;
    juce::TextButton triggerButton;
    juce::Label triggerStatus;
    original_sequencer::prototype::AudioCore audioCore;
    original_sequencer::prototype::ScheduledTriggerInput triggerInput{audioCore};
    std::mutex commandMutex;
    bool audioPrepared = false;
    std::atomic<double> actualSampleRate{0.0};
    std::atomic<std::uint32_t> actualCallbackFrames{0};
    std::atomic<std::uint32_t> audioRestartCount{0};
    std::atomic<bool> audioWasPrepared{false};
    std::uint64_t callbackStartFrame = 0;
};

class MainWindow final : public juce::DocumentWindow {
public:
    MainWindow()
        : juce::DocumentWindow("Sequencer Prototype",
                               juce::Colours::black,
                               juce::DocumentWindow::allButtons) {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class PrototypeApplication final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Sequencer Prototype"; }
    const juce::String getApplicationVersion() override { return "0.0.1"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override { window = std::make_unique<MainWindow>(); }
    void shutdown() override { window.reset(); }

private:
    std::unique_ptr<MainWindow> window;
};

}  // namespace

START_JUCE_APPLICATION(PrototypeApplication)
