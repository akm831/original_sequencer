#include "original_sequencer/prototype/AudioCore.h"
#include <array>
#include <cassert>
#include <cmath>
#include <limits>

using namespace original_sequencer::prototype;

int main() {
    AudioCore core;
    core.initialize(48000.0, 512);
    assert(!core.enqueueCommand({AudioCommandType::trigger, 0, 0, -1.0F}));
    assert(!core.enqueueCommand({AudioCommandType::trigger, 0, 0, std::numeric_limits<float>::quiet_NaN()}));
    assert(!core.enqueueCommand({AudioCommandType::trigger, 0, 0, 2.0F}));
    assert(!core.enqueueCommand({static_cast<AudioCommandType>(42), 0, 0, 1.0F}));

    std::array<float, 192> output{};
    assert(core.enqueueCommand({AudioCommandType::trigger, 1037, 0, 1.0F}));
    assert(core.enqueueCommand({AudioCommandType::trigger, 1095, 0, 0.5F}));
    assert(core.enqueueCommand({AudioCommandType::trigger, 1096, 0, 0.25F}));
    core.render(output.data(), 96, 2, 1000);
    for (std::size_t i = 0; i < 37 * 2; ++i) assert(output[i] == 0.0F);
    assert(output[37 * 2] == 0.08F);
    assert(output[95 * 2] == 0.04F);
    for (std::size_t i = 0; i < 96; ++i) assert(output[i * 2] == output[i * 2 + 1]);
    assert(core.diagnostics().triggerCount == 2);
    core.render(output.data(), 96, 2, 1096);
    assert(output[0] == 0.02F);
    assert(core.diagnostics().lastTriggerOffset == 0);

    // Same timestamps: FIFO, last retrigger wins in this monophonic P3 voice.
    core.reset();
    assert(core.enqueueCommand({AudioCommandType::trigger, 10, 0, 1.0F}));
    assert(core.enqueueCommand({AudioCommandType::trigger, 10, 0, 0.5F}));
    core.render(output.data(), 96, 2, 0);
    assert(output[20] == 0.04F);
    assert(core.diagnostics().triggerCount == 2);

    // Late commands sound at offset 0; reset clears already active sound.
    core.reset();
    assert(core.enqueueCommand({AudioCommandType::trigger, 0, 0, 1.0F}));
    core.render(output.data(), 96, 2, 1000);
    assert(output[0] == 0.08F);
    core.reset();
    core.render(output.data(), 96, 2, 1096);
    for (auto sample : output) assert(sample == 0.0F);

    // Compare a long callback with unevenly partitioned callbacks: waveform
    // and envelope must not depend on buffer size, including burst termination.
    AudioCore whole, split;
    whole.initialize(48000.0, 4096);
    split.initialize(48000.0, 512);
    for (auto* engine : {&whole, &split}) {
        assert(engine->enqueueCommand({AudioCommandType::trigger, 37, 0, 1.0F}));
        assert(engine->enqueueCommand({AudioCommandType::trigger, 512, 0, 0.5F}));
    }
    std::array<float, 4096> expected{}, actual{};
    whole.render(expected.data(), 4096, 1, 0);
    std::uint32_t cursor = 0;
    for (auto size : {13U, 24U, 59U, 160U, 256U, 1U, 383U, 3200U}) {
        split.render(actual.data() + cursor, size, 1, cursor);
        cursor += size;
    }
    assert(cursor == actual.size());
    assert(actual == expected);
    for (std::size_t i = 2912; i < expected.size(); ++i) assert(expected[i] == 0.0F);
    for (auto sample : expected) assert(std::isfinite(sample) && std::abs(sample) <= 0.08F);

    // A timestamp at UINT64_MAX still has a valid offset in this final buffer.
    core.reset();
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    assert(core.enqueueCommand({AudioCommandType::trigger, maximum, 0, 1.0F}));
    core.render(output.data(), 96, 2, maximum - 95);
    assert(output[190] == 0.08F);
    assert(core.diagnostics().lastTriggerOffset == 95);
}
