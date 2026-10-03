#pragma once
#include "original_sequencer/prototype/AudioCore.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace original_sequencer::prototype {

struct SequenceState {
    double bpm = 120.0;
    std::uint32_t running = 0;
    std::uint32_t stepMask = 0x1111;
    std::uint32_t currentStep = 16; // 16 = no audible playhead
    std::uint64_t missedSteps = 0;
};

// Control-side model/scheduler. All calls are serialized by the adapter;
// advance runs on a native control thread, never in the audio callback.
class PrototypeSequencer {
public:
    static constexpr std::uint32_t kPpqn = 960;
    static constexpr std::uint32_t kStepTicks = 240;
    explicit PrototypeSequencer(AudioCore& core) noexcept : core_(core) {}

    void deviceReset() noexcept { stop(); missedSteps_ = 0; }
    void stop() noexcept {
        running_ = false;
        if (++generation_ == 0) ++generation_;
        core_.setSequenceGeneration(generation_);
    }
    [[nodiscard]] bool play() noexcept {
        const auto snapshot = core_.diagnostics();
        if (!(snapshot.sampleRate > 0.0)) return false;
        if (running_) return true;
        stop();
        sampleRate_ = snapshot.sampleRate;
        nextFrame_ = static_cast<long double>(snapshot.renderedFrames)
                   + std::max(snapshot.callbackFrames, 256U);
        nextStep_ = 0;
        running_ = true;
        advance();
        return running_;
    }
    [[nodiscard]] bool setBpm(double bpm) noexcept {
        if (!std::isfinite(bpm) || bpm < 60.0 || bpm > 240.0) return false;
        bpm_ = bpm;
        return true;
    }
    [[nodiscard]] bool setStep(std::uint32_t step, bool enabled) noexcept {
        if (step >= 16) return false;
        if (enabled) mask_ |= (1U << step); else mask_ &= ~(1U << step);
        return true;
    }
    void advance() noexcept {
        if (!running_) return;
        const auto snapshot = core_.diagnostics();
        if (snapshot.sampleRate != sampleRate_ || !(sampleRate_ > 0.0)) { stop(); return; }
        const long double period = sampleRate_ * 60.0L * kStepTicks / (bpm_ * kPpqn);
        const long double now = snapshot.renderedFrames;
        if (nextFrame_ < now) {
            const auto skipped = static_cast<std::uint64_t>(std::ceil((now - nextFrame_) / period));
            missedSteps_ += skipped;
            nextStep_ = (nextStep_ + skipped % 16) % 16;
            nextFrame_ += skipped * period;
        }
        const auto horizon = now + std::max(sampleRate_ * 0.05, snapshot.callbackFrames * 2.0);
        for (std::uint32_t count = 0; count < 16 && nextFrame_ < horizon; ++count) {
            if (nextFrame_ > std::numeric_limits<std::uint64_t>::max() - 1.0L) { stop(); return; }
            const auto target = static_cast<std::uint64_t>(std::floor(nextFrame_ + 0.5L));
            const auto type = (mask_ & (1U << nextStep_)) ? AudioCommandType::trigger : AudioCommandType::sequenceStep;
            if (!core_.enqueueCommand({type, target, 0, 1.0F, generation_, nextStep_})) {
                stop(); // Overflow never waits or creates a catch-up burst.
                return;
            }
            nextFrame_ += period; // retain fractional frames to avoid tempo drift
            nextStep_ = (nextStep_ + 1) % 16;
        }
    }
    [[nodiscard]] SequenceState state() const noexcept {
        return {bpm_, running_ ? 1U : 0U, mask_,
                running_ ? core_.diagnostics().sequenceStep : 16U, missedSteps_};
    }

private:
    AudioCore& core_;
    double bpm_ = 120.0;
    double sampleRate_ = 0.0;
    long double nextFrame_ = 0.0;
    std::uint32_t nextStep_ = 0;
    std::uint32_t mask_ = 0x1111;
    std::uint64_t generation_ = 0;
    std::uint64_t missedSteps_ = 0;
    bool running_ = false;
};
} // namespace original_sequencer::prototype
