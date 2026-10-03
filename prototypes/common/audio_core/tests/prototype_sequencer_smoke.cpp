#include "original_sequencer/prototype/PrototypeSequencer.h"
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
using namespace original_sequencer::prototype;

int main() {
    AudioCore core;
    core.initialize(48000.0, 512);
    PrototypeSequencer seq(core);
    seq.deviceReset();
    assert(seq.state().stepMask == 0x1111);
    assert(!seq.setBpm(59));
    assert(!seq.setBpm(241));
    assert(!seq.setBpm(std::numeric_limits<double>::quiet_NaN()));
    assert(!seq.setStep(16, true));
    for (unsigned i = 0; i < 16; ++i) assert(seq.setStep(i, true));
    assert(seq.play());
    assert(seq.play()); // idempotent: does not double-schedule
    std::array<float, 96> output{};
    std::vector<std::uint64_t> events;
    for (std::uint64_t frame = 0; frame < 100000; frame += 96) {
        seq.advance();
        auto before = core.diagnostics().triggerCount;
        core.render(output.data(), 96, 1, frame);
        const auto d = core.diagnostics();
        if (d.triggerCount > before) events.push_back(frame + d.lastTriggerOffset);
    }
    assert(events.size() == 17);
    for (std::size_t i = 0; i < events.size(); ++i) assert(events[i] == 256 + i * 6000);
    assert(seq.state().currentStep == 0);
    assert(seq.state().missedSteps == 0);
    assert(core.diagnostics().queueOverflowCount == 0);

    // Stop invalidates future events, leaves only the bounded 50 ms voice tail.
    seq.stop();
    const auto count = core.diagnostics().triggerCount;
    for (std::uint64_t f = 100032; f < 110000; f += 96) core.render(output.data(), 96, 1, f);
    assert(core.diagnostics().triggerCount == count);
    for (auto sample : output) assert(sample == 0);
    assert(seq.state().currentStep == 16);

    // Immediate stop/play with a retained future command must discard it before
    // timestamp comparison; the old timeline cannot block the new first step.
    core.initialize(48000, 512);
    seq.deviceReset();
    assert(seq.play());
    core.render(output.data(), 96, 1, 0); // retains future first event
    seq.stop();
    assert(seq.play());
    for (std::uint64_t f = 96; f < 768; f += 96) {
        seq.advance();
        core.render(output.data(), 96, 1, f);
    }
    assert(core.diagnostics().triggerCount == 1);
    assert(seq.state().currentStep == 0);

    // Fractional tempo: rounding each absolute time avoids accumulated drift.
    core.initialize(44100, 512);
    seq.deviceReset();
    assert(seq.setBpm(123));
    assert(seq.play());
    events.clear();
    for (std::uint64_t frame = 0; frame < 441000; frame += 96) {
        seq.advance();
        auto before = core.diagnostics().triggerCount;
        core.render(output.data(), 96, 1, frame);
        const auto d = core.diagnostics();
        if (d.triggerCount > before) events.push_back(frame + d.lastTriggerOffset);
    }
    assert(events.size() > 80);
    for (std::size_t i = 0; i < events.size(); ++i)
        assert(events[i] == static_cast<std::uint64_t>(std::floor(256.0L + i * 44100.0L * 60 / (123 * 4) + 0.5L)));

    // Tempo and pattern edits preserve phase, only affect unscheduled events.
    assert(seq.setBpm(240));
    for (unsigned i = 0; i < 16; ++i) assert(seq.setStep(i, false));
    const auto beforeEdit = core.diagnostics().triggerCount;
    for (std::uint64_t f = 441024; f < 470000; f += 96) {
        seq.advance(); core.render(output.data(), 96, 1, f);
    }
    assert(core.diagnostics().triggerCount <= beforeEdit + 1);
    assert(seq.state().running == 1);
    assert(seq.state().currentStep < 16); // disabled steps still advance playhead

    // A delayed scheduler skips missed steps rather than sounding a catch-up burst.
    core.render(nullptr, 44100, 0, 470016);
    seq.advance();
    assert(seq.state().missedSteps > 0);
    const auto beforeGap = core.diagnostics().triggerCount;
    core.render(output.data(), 96, 1, 514116);
    assert(core.diagnostics().triggerCount <= beforeGap + 1);

    // Queue overflow stops transport without blocking; reset preserves edits.
    core.initialize(48000, 512);
    seq.deviceReset();
    for (unsigned i = 0; i < 256; ++i) assert(core.enqueueCommand({AudioCommandType::trigger, 100000, 0, 1}));
    assert(!seq.play());
    assert(seq.state().running == 0);
    assert(core.diagnostics().queueOverflowCount == 1);
    assert(seq.state().bpm == 240);
    assert(seq.state().stepMask == 0);
}
