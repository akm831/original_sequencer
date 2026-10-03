#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "original_sequencer/prototype/AudioCore.h"
#include "original_sequencer/prototype/PrototypeSequencer.h"
#include "original_sequencer/prototype/ScheduledTriggerInput.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

namespace {
class MainComponent final : public juce::AudioAppComponent, private juce::Timer {
public:
    MainComponent() {
        title.setText("JUCE — 16-step sequencer", juce::dontSendNotification);
        title.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(title);
        transportButton.onClick = [this] {
            std::lock_guard<std::mutex> lock(commandMutex);
            if (sequencer.state().running) sequencer.stop();
            else if (audioPrepared) (void)sequencer.play();
        };
        addAndMakeVisible(transportButton);
        tempo.setRange(60.0, 240.0, 1.0);
        tempo.setValue(120.0, juce::dontSendNotification);
        tempo.setSliderStyle(juce::Slider::LinearHorizontal);
        tempo.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 36);
        tempo.setTextValueSuffix(" BPM");
        tempo.onValueChange = [this] {
            std::lock_guard<std::mutex> lock(commandMutex);
            (void)sequencer.setBpm(tempo.getValue());
        };
        addAndMakeVisible(tempo);
        for (std::size_t i = 0; i < steps.size(); ++i) {
            auto& button = steps[i];
            button.setButtonText(juce::String(static_cast<int>(i + 1)));
            button.onClick = [this, i] {
                std::lock_guard<std::mutex> lock(commandMutex);
                const auto mask = sequencer.state().stepMask;
                (void)sequencer.setStep(static_cast<std::uint32_t>(i), (mask & (1U << i)) == 0);
            };
            addAndMakeVisible(button);
        }
        diagnosticsButton.setButtonText("Audio diagnostics");
        diagnosticsButton.onClick = [this] {
            showingDiagnostics = !showingDiagnostics;
            diagnosticsButton.setButtonText(showingDiagnostics ? "Back to steps" : "Audio diagnostics");
            resized();
        };
        addAndMakeVisible(diagnosticsButton);
        diagnostics.setJustificationType(juce::Justification::topLeft);
        addChildComponent(diagnostics);
        triggerButton.setButtonText("Test sound (when stopped)");
        triggerButton.onClick = [this] {
            bool accepted = false;
            {
                std::lock_guard<std::mutex> lock(commandMutex);
                accepted = audioPrepared && !sequencer.state().running && triggerInput.submit(37, 1.0F);
            }
            triggerStatus.setText(accepted ? "Test accepted" : "Test unavailable", juce::dontSendNotification);
        };
        addChildComponent(triggerButton);
        addChildComponent(triggerStatus);
        setSize(420, 720);
        setAudioChannels(0, 2);
        // Control-side queue producer. UI timer only reads and paints state.
        schedulerThread = std::thread([this] {
            while (!exitScheduler.load(std::memory_order_acquire)) {
                {
                    std::lock_guard<std::mutex> lock(commandMutex);
                    if (audioPrepared) sequencer.advance();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        startTimerHz(60);
    }

    ~MainComponent() override {
        stopTimer();
        exitScheduler.store(true, std::memory_order_release);
        if (schedulerThread.joinable()) schedulerThread.join();
        shutdownAudio();
    }

    void stopTransport() {
        std::lock_guard<std::mutex> lock(commandMutex);
        sequencer.stop();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        std::lock_guard<std::mutex> lock(commandMutex);
        if (audioWasPrepared) ++audioRestartCount;
        audioWasPrepared = true;
        audioCore.initialize(sampleRate, static_cast<std::uint32_t>(std::max(samplesPerBlockExpected, 1)));
        sequencer.deviceReset();
        triggerInput.reset();
        callbackStartFrame = 0;
        audioPrepared = true;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
        if (bufferToFill.buffer == nullptr) return;
        const auto begin = std::chrono::steady_clock::now();
        const auto frames = static_cast<std::uint32_t>(std::max(bufferToFill.numSamples, 0));
        audioCore.renderPlanar(bufferToFill.buffer->getArrayOfWritePointers(), frames,
            static_cast<std::uint32_t>(std::max(bufferToFill.buffer->getNumChannels(), 0)),
            callbackStartFrame, static_cast<std::uint32_t>(bufferToFill.startSample));
        const auto durationUs = std::chrono::duration<double, std::micro>(
            std::chrono::steady_clock::now() - begin).count();
        audioCore.recordCallbackTiming(callbackStartFrame, frames, durationUs);
        callbackStartFrame += frames;
    }

    void releaseResources() override {
        std::lock_guard<std::mutex> lock(commandMutex);
        audioPrepared = false;
        sequencer.deviceReset();
        audioCore.shutdown();
        triggerInput.reset();
        callbackStartFrame = 0;
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(16);
        title.setBounds(bounds.removeFromTop(36));
        transportButton.setBounds(bounds.removeFromTop(48));
        bounds.removeFromTop(8);
        tempo.setBounds(bounds.removeFromTop(44));
        diagnosticsButton.setBounds(bounds.removeFromBottom(44));
        bounds.removeFromBottom(8);
        diagnostics.setVisible(showingDiagnostics);
        triggerButton.setVisible(showingDiagnostics);
        triggerStatus.setVisible(showingDiagnostics);
        if (showingDiagnostics) {
            triggerButton.setBounds(bounds.removeFromTop(44));
            triggerStatus.setBounds(bounds.removeFromTop(28));
            diagnostics.setBounds(bounds);
        }
        const auto rowHeight = std::max(1, bounds.getHeight() / 4);
        for (std::size_t row = 0; row < 4; ++row) {
            auto rowBounds = bounds.removeFromTop(rowHeight);
            const auto width = std::max(1, rowBounds.getWidth() / 4);
            for (std::size_t column = 0; column < 4; ++column) {
                auto& button = steps[row * 4 + column];
                button.setVisible(!showingDiagnostics);
                button.setBounds(rowBounds.removeFromLeft(width).reduced(4));
            }
        }
    }

private:
    void timerCallback() override {
        original_sequencer::prototype::SequenceState state;
        bool ready = false;
        std::uint32_t restarts = 0;
        {
            std::lock_guard<std::mutex> lock(commandMutex);
            state = sequencer.state();
            ready = audioPrepared;
            restarts = audioRestartCount;
        }
        transportButton.setEnabled(ready);
        transportButton.setButtonText(state.running ? "Stop" : "Play");
        tempo.setEnabled(ready);
        tempo.setValue(state.bpm, juce::dontSendNotification);
        triggerButton.setEnabled(ready && !state.running);
        for (std::size_t i = 0; i < steps.size(); ++i) {
            auto& button = steps[i];
            const auto enabled = (state.stepMask & (1U << i)) != 0;
            const auto current = state.running && state.currentStep == i;
            button.setEnabled(ready);
            const auto colour = current ? juce::Colours::orange
                : (enabled ? juce::Colours::rebeccapurple : juce::Colours::darkgrey);
            if (button.findColour(juce::TextButton::buttonColourId) != colour)
                button.setColour(juce::TextButton::buttonColourId, colour);
            button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        // Percentiles and diagnostic text are refreshed at 5 Hz, independently.
        if (++diagnosticTicks < 12) return;
        diagnosticTicks = 0;
        const auto d = audioCore.diagnostics();
        diagnostics.setText(
            "sampleRate: " + juce::String(d.sampleRate, 1)
            + "\ncallbackFrames: " + juce::String(d.callbackFrames)
            + " (min " + juce::String(d.callbackFramesMin) + " / max " + juce::String(d.callbackFramesMax) + ")"
            + "\ncallbackDurationUs: " + juce::String(d.callbackDurationUs, 2)
            + "\ncallbackLoad: " + juce::String(d.callbackLoad * 100.0, 2) + "%"
            + "\nP95 / P99 / Peak: " + juce::String(d.callbackLoadP95 * 100.0, 1)
            + " / " + juce::String(d.callbackLoadP99 * 100.0, 1)
            + " / " + juce::String(d.callbackLoadPeak * 100.0, 2) + "%"
            + "\nrenderedFrames: " + juce::String(static_cast<juce::int64>(d.renderedFrames))
            + "\ncallbackStartFrame: " + juce::String(static_cast<juce::int64>(d.callbackStartFrame))
            + "\naudioRestartCount: " + juce::String(restarts)
            + "\nqueueDepth / highWater / overflow: " + juce::String(d.queueDepth)
            + " / " + juce::String(d.queueHighWaterMark)
            + " / " + juce::String(static_cast<juce::int64>(d.queueOverflowCount))
            + "\nmissedSteps: " + juce::String(static_cast<juce::int64>(state.missedSteps))
            + "\ntriggerCount / offset: " + juce::String(static_cast<juce::int64>(d.triggerCount))
            + " / " + juce::String(d.lastTriggerOffset)
            + "\n50 ms / 220 Hz test voice", juce::dontSendNotification);
    }

    juce::Label title, diagnostics, triggerStatus;
    juce::TextButton transportButton, diagnosticsButton, triggerButton;
    juce::Slider tempo;
    std::array<juce::TextButton, 16> steps;
    original_sequencer::prototype::AudioCore audioCore;
    original_sequencer::prototype::PrototypeSequencer sequencer{audioCore};
    original_sequencer::prototype::ScheduledTriggerInput triggerInput{audioCore};
    std::mutex commandMutex;
    bool audioPrepared = false, audioWasPrepared = false, showingDiagnostics = false;
    std::uint32_t audioRestartCount = 0, diagnosticTicks = 0;
    std::uint64_t callbackStartFrame = 0;
    std::atomic<bool> exitScheduler{false};
    std::thread schedulerThread;
};

class MainWindow final : public juce::DocumentWindow {
public:
    MainWindow() : juce::DocumentWindow("Sequencer JUCE", juce::Colours::black, juce::DocumentWindow::allButtons) {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
#if JUCE_ANDROID || JUCE_IOS
        setFullScreen(true);
#else
        centreWithSize(getWidth(), getHeight());
#endif
        setVisible(true);
    }
    void stopTransport() {
        if (auto* component = dynamic_cast<MainComponent*>(getContentComponent())) component->stopTransport();
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class PrototypeApplication final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Sequencer JUCE"; }
    const juce::String getApplicationVersion() override { return "0.0.2"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const juce::String&) override { window = std::make_unique<MainWindow>(); }
    void shutdown() override { window.reset(); }
    void suspended() override { if (window) window->stopTransport(); }
    void resumed() override {} // No automatic transport restart.
private:
    std::unique_ptr<MainWindow> window;
};
} // namespace
START_JUCE_APPLICATION(PrototypeApplication)
