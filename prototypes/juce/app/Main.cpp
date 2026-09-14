#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "original_sequencer/prototype/AudioCore.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>

namespace {

constexpr double kTestToneFrequencyHz = 220.0;
constexpr float kTestToneAmplitude = 0.08F;
constexpr double kTwoPi = 6.28318530717958647692;

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer {
public:
    MainComponent() {
        title.setText("JUCE/C++ — P1 audio callback + sine", juce::dontSendNotification);
        title.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(title);

        diagnostics.setJustificationType(juce::Justification::centred);
        diagnostics.setText("Audio device: starting...", juce::dontSendNotification);
        addAndMakeVisible(diagnostics);

        setSize(640, 360);

        setAudioChannels(0, 2);
        startTimerHz(5);
    }

    ~MainComponent() override {
        stopTimer();
        shutdownAudio();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        const auto maxCallbackFrames = static_cast<std::uint32_t>(
            std::max(samplesPerBlockExpected, 1));

        if (audioWasPrepared.exchange(true, std::memory_order_relaxed)) {
            audioRestartCount.fetch_add(1, std::memory_order_relaxed);
        }

        actualSampleRate.store(sampleRate, std::memory_order_relaxed);
        actualCallbackFrames.store(0, std::memory_order_relaxed);
        callbackStartFrame = 0;
        sinePhase = 0.0;
        sinePhaseDelta = sampleRate > 0.0
            ? (kTwoPi * kTestToneFrequencyHz) / sampleRate
            : 0.0;
        audioCore.initialize(sampleRate, maxCallbackFrames);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
        if (bufferToFill.buffer == nullptr) {
            return;
        }

        bufferToFill.clearActiveBufferRegion();

        const auto frameCount = static_cast<std::uint32_t>(
            std::max(bufferToFill.numSamples, 0));
        const auto channelCount = static_cast<std::uint32_t>(
            std::max(bufferToFill.buffer->getNumChannels(), 0));

        actualCallbackFrames.store(frameCount, std::memory_order_relaxed);
        audioCore.render(nullptr, frameCount, channelCount, callbackStartFrame);
        renderTestTone(bufferToFill);
        callbackStartFrame += frameCount;
    }

    void releaseResources() override {
        audioCore.shutdown();
        actualCallbackFrames.store(0, std::memory_order_relaxed);
        callbackStartFrame = 0;
        sinePhase = 0.0;
        sinePhaseDelta = 0.0;
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(24);
        title.setBounds(bounds.removeFromTop(72));
        diagnostics.setBounds(bounds.removeFromTop(120));
    }

private:
    void renderTestTone(const juce::AudioSourceChannelInfo& bufferToFill) noexcept {
        if (bufferToFill.buffer == nullptr || sinePhaseDelta <= 0.0) {
            return;
        }

        const auto channelCount = bufferToFill.buffer->getNumChannels();
        for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
            const auto value = static_cast<float>(std::sin(sinePhase)) * kTestToneAmplitude;

            for (int channel = 0; channel < channelCount; ++channel) {
                bufferToFill.buffer->setSample(
                    channel,
                    bufferToFill.startSample + sample,
                    value);
            }

            sinePhase += sinePhaseDelta;
            if (sinePhase >= kTwoPi) {
                sinePhase -= kTwoPi;
            }
        }
    }

    void timerCallback() override {
        diagnostics.setText(
            "sampleRate: "
                + juce::String(actualSampleRate.load(std::memory_order_relaxed), 1)
                + " Hz\ncallbackFrames: "
                + juce::String(actualCallbackFrames.load(std::memory_order_relaxed))
                + "\naudioRestartCount: "
                + juce::String(audioRestartCount.load(std::memory_order_relaxed))
                + "\ntestTone: 220 Hz @ 8%",
            juce::dontSendNotification);
    }

    juce::Label title;
    juce::Label diagnostics;
    original_sequencer::prototype::AudioCore audioCore;
    std::atomic<double> actualSampleRate{0.0};
    std::atomic<std::uint32_t> actualCallbackFrames{0};
    std::atomic<std::uint32_t> audioRestartCount{0};
    std::atomic<bool> audioWasPrepared{false};
    std::uint64_t callbackStartFrame = 0;
    double sinePhase = 0.0;
    double sinePhaseDelta = 0.0;
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
