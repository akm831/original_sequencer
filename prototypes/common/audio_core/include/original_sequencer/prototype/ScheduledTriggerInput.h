#pragma once

#include "original_sequencer/prototype/AudioCore.h"
#include <algorithm>
#include <limits>

namespace original_sequencer::prototype {

// Non-realtime adapter helper. Caller serializes submit/reset with lifecycle;
// the audio callback never acquires that mutex. This is a P3 scheduled test
// input, not the future Live Pad path.
class ScheduledTriggerInput {
public:
    explicit ScheduledTriggerInput(AudioCore& core) noexcept : core_(core) {}
    void reset() noexcept { lastTargetFrame_ = 0; }

    [[nodiscard]] bool submit(std::uint32_t delayFrames, float value) noexcept {
        if (delayFrames > 96000) return false;
        const auto snapshot = core_.diagnostics();
        if (snapshot.sampleRate <= 0.0) return false;
        const auto lead = static_cast<std::uint64_t>(std::max(snapshot.callbackFrames, 256U)) + delayFrames;
        if (snapshot.renderedFrames > std::numeric_limits<std::uint64_t>::max() - lead) return false;
        const auto target = snapshot.renderedFrames + lead;
        if (target < lastTargetFrame_) return false;
        if (!core_.enqueueCommand(AudioCommand{AudioCommandType::trigger, target, 0, value})) return false;
        lastTargetFrame_ = target;
        return true;
    }

private:
    AudioCore& core_;
    std::uint64_t lastTargetFrame_ = 0;
};

} // namespace original_sequencer::prototype
