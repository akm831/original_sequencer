#include "original_sequencer/prototype/AudioCore.h"

#include <array>
#include <cassert>

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
    assert(restarted.audioRestartCount == 1);
}
