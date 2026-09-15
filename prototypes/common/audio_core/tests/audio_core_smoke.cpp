#include "original_sequencer/prototype/AudioCore.h"

#include <array>
#include <cassert>
#include <thread>

int main() {
    using original_sequencer::prototype::AudioCommand;
    using original_sequencer::prototype::AudioCommandType;
    using original_sequencer::prototype::AudioCore;

    AudioCore core;
    core.initialize(48000.0, 512);

    std::array<float, 256> output{};
    output.fill(1.0F);
    core.render(output.data(), 128, 2, 0);

    for (const auto sample : output) assert(sample == 0.0F);

    const auto first = core.diagnostics();
    assert(first.sampleRate == 48000.0);
    assert(first.callbackFrames == 128);
    assert(first.renderedFrames == 128);
    assert(first.audioRestartCount == 0);

    core.reset();
    assert(core.enqueueCommand(AudioCommand{AudioCommandType::trigger, 12000, 1, 1.0F}));
    assert(core.enqueueCommand(AudioCommand{AudioCommandType::trigger, 12037, 2, 1.0F}));
    assert(core.enqueueCommand(AudioCommand{AudioCommandType::trigger, 12095, 3, 1.0F}));
    assert(core.enqueueCommand(AudioCommand{AudioCommandType::trigger, 12096, 4, 1.0F}));

    core.render(nullptr, 96, 0, 12000);
    const auto callbackOne = core.diagnostics();
    assert(callbackOne.triggerCount == 3);
    assert(callbackOne.lastTriggerOffset == 95);
    assert(callbackOne.queueDepth == 1);
    assert(callbackOne.queueHighWaterMark == 4);
    assert(callbackOne.queueOverflowCount == 0);

    core.render(nullptr, 96, 0, 12096);
    const auto callbackTwo = core.diagnostics();
    assert(callbackTwo.triggerCount == 4);
    assert(callbackTwo.lastTriggerOffset == 0);
    assert(callbackTwo.queueDepth == 0);

    core.reset();
    assert(core.enqueueCommand(AudioCommand{AudioCommandType::trigger, 999, 1, 1.0F}));
    core.render(nullptr, 96, 0, 1000);
    const auto lateCommand = core.diagnostics();
    assert(lateCommand.triggerCount == 1);
    assert(lateCommand.lastTriggerOffset == 0);

    core.initialize(44100.0, 256);
    const auto restarted = core.diagnostics();
    assert(restarted.sampleRate == 44100.0);
    assert(restarted.callbackFrames == 0);
    assert(restarted.renderedFrames == 0);
    assert(restarted.queueDepth == 0);
    assert(restarted.triggerCount == 0);
    assert(restarted.audioRestartCount == 1);

    std::thread renderThread([&core] {
        for (std::uint64_t i = 0; i < 10000; ++i) core.render(nullptr, 64, 0, i * 64);
    });

    std::uint64_t lastRenderedFrames = 0;
    for (;;) {
        const auto snapshot = core.diagnostics();
        assert(snapshot.sampleRate == 44100.0);
        assert(snapshot.callbackFrames == 0 || snapshot.callbackFrames == 64);
        assert(snapshot.renderedFrames >= lastRenderedFrames);
        lastRenderedFrames = snapshot.renderedFrames;
        if (snapshot.renderedFrames >= 640000) break;
    }
    renderThread.join();

    const auto finalSnapshot = core.diagnostics();
    assert(finalSnapshot.callbackFrames == 64);
    assert(finalSnapshot.renderedFrames == 640000);
}
