#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "original_sequencer/prototype/AudioCore.h"

#include <algorithm>
#include <cstdint>
#include <memory>

namespace {

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer {
public:
    MainComponent() {
        title.setText("JUCE/C++ — P1 audio callback", juce::dontSendNotification);
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
        audioCore.initialize(sampleRate, maxCallbackFrames);
        callbackStartFrame = 0;
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

        audioCore.render(nullptr, frameCount, channelCount, callbackStartFrame);
        callbackStartFrame += frameCount;
    }

    void releaseResources() override {
        audioCore.shutdown();
        callbackStartFrame = 0;
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(24);
        title.setBounds(bounds.removeFromTop(72));
        diagnostics.setBounds(bounds.removeFromTop(96));
    }

private:
    void timerCallback() override {
        const auto snapshot = audioCore.diagnostics();

        diagnostics.setText(
            "sampleRate: " + juce::String(snapshot.sampleRate, 1)
                + " Hz\ncallbackFrames: " + juce::String(snapshot.callbackFrames)
                + "\nrenderedFrames: " + juce::String(snapshot.renderedFrames)
                + "\naudioRestartCount: " + juce::String(snapshot.audioRestartCount),
            juce::dontSendNotification);
    }

    juce::Label title;
    juce::Label diagnostics;
    original_sequencer::prototype::AudioCore audioCore;
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
