#include "original_sequencer/prototype/AudioCore.h"

#include <array>
#include <cassert>
#include <thread>

int main() {
    using original_sequencer::prototype::AudioCore;

    AudioCore core;
    core.initialize(48000.0, 512);

    std::array<float, 256> output{};
    output.fill(1.0F);
    core.render(output.data(), 128, 2, 0);

    for (const auto sample : output) {
        assert(sample == 0.0F);
    }

    const auto first = core.diagnostics();
    assert(first.sampleRate == 48000.0);
    assert(first.callbackFrames == 128);
    assert(first.renderedFrames == 128);
    assert(first.audioRestartCount == 0);

    core.initialize(44100.0, 256);
    const auto restarted = core.diagnostics();
    assert(restarted.sampleRate == 44100.0);
    assert(restarted.callbackFrames == 0);
    assert(restarted.renderedFrames == 0);
    assert(restarted.audioRestartCount == 1);

    std::thread renderThread([&core] {
        for (std::uint64_t i = 0; i < 10000; ++i) {
            core.render(nullptr, 64, 0, i * 64);
        }
    });

    std::uint64_t lastRenderedFrames = 0;
    for (;;) {
        const auto snapshot = core.diagnostics();
        assert(snapshot.sampleRate == 44100.0);
        assert(snapshot.callbackFrames == 0 || snapshot.callbackFrames == 64);
        assert(snapshot.renderedFrames >= lastRenderedFrames);
        lastRenderedFrames = snapshot.renderedFrames;
        if (snapshot.renderedFrames >= 640000) {
            break;
        }
    }
    renderThread.join();

    const auto finalSnapshot = core.diagnostics();
    assert(finalSnapshot.callbackFrames == 64);
    assert(finalSnapshot.renderedFrames == 640000);
}
