#include "original_sequencer/prototype/PrototypeSequencer.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
using namespace original_sequencer::prototype;

int main() {
    constexpr std::array<std::uint32_t, 7> buffers{64, 96, 128, 192, 256, 384, 512};
    for (const double rate : {44100.0, 48000.0}) {
        AudioCore core;
        core.initialize(rate, 512);
        PrototypeSequencer seq(core);
        seq.deviceReset();
        assert(seq.setBpm(123));
        for (unsigned i = 0; i < 16; ++i) assert(seq.setStep(i, true));
        assert(seq.play());
        std::array<float, 1024> output{};
        std::uint64_t frame = 0, nextScheduler = 0, events = 0, iteration = 0;
        const auto end = static_cast<std::uint64_t>(rate * 1800); // 30 minutes, accelerated
        while (frame < end) {
            if (frame >= nextScheduler) {
                seq.advance();
                nextScheduler = frame + static_cast<std::uint64_t>(rate * 0.005);
            }
            const auto frames = buffers[iteration++ % buffers.size()];
            const auto before = core.diagnostics().triggerCount;
            core.render(output.data(), frames, 2, frame);
            const auto d = core.diagnostics();
            assert(d.triggerCount <= before + 1);
            if (d.triggerCount != before) {
                const auto expected = static_cast<std::uint64_t>(
                    std::floor(256.0L + events * rate * 60.0L / (123 * 4) + 0.5L));
                assert(frame + d.lastTriggerOffset == expected);
                ++events;
            }
            for (unsigned i = 0; i < frames * 2; ++i) {
                assert(std::isfinite(output[i]));
                assert(std::abs(output[i]) <= 0.080001F);
            }
            assert(d.queueOverflowCount == 0);
            frame += frames;
        }
        assert(events > 14000 && seq.state().missedSteps == 0);
        // Rapid transport edits cannot leak a cancelled epoch or overflow.
        for (unsigned cycle = 0; cycle < 1000; ++cycle) {
            seq.stop();
            assert(seq.setBpm(60 + cycle % 181));
            assert(seq.setStep(cycle % 16, cycle % 2 == 0));
            assert(seq.play());
            seq.stop();
            const auto before = core.diagnostics().triggerCount;
            for (unsigned i = 0; i < 32; ++i) {
                core.render(output.data(), 96, 2, frame);
                frame += 96;
            }
            assert(core.diagnostics().triggerCount == before);
            for (unsigned i = 0; i < 192; ++i) assert(output[i] == 0);
            assert(core.diagnostics().queueOverflowCount == 0);
        }
        // Device resets stop transport, preserve edits, and establish a new clock.
        const auto mask = seq.state().stepMask;
        const auto bpm = seq.state().bpm;
        core.shutdown(); seq.deviceReset();
        assert(!seq.play());
        core.initialize(rate, 512); seq.deviceReset();
        assert(seq.state().stepMask == mask && seq.state().bpm == bpm);
        assert(seq.state().running == 0 && seq.state().currentStep == 16);
        assert(seq.play());
    }
}
