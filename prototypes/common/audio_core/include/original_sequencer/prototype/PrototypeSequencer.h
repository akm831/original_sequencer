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
    std::uint32_t currentPattern = 0, queuedPattern = 4;
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
        if (running_) {
            const auto d = core_.diagnostics();
            if (d.sequenceStep < 16) activePattern_ = d.sequencePattern;
        }
        queuedPattern_ = 4;
        bassSlideNext_ = false;
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
    struct Track {
        std::uint32_t mask = 0, accents = 0; float level = .8F; bool muted = false;
        SoundSettings sound{};
        std::array<std::uint32_t,16> notes{36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36};
        std::uint32_t flags = 0; // Bass: outgoing slide; Hat: open articulation.
    };
    [[nodiscard]] bool setSound(std::uint32_t pattern, std::uint32_t track, SoundSettings sound) noexcept {
        if (pattern >= 4 || track >= 4 || !sound.valid()) return false;
        tracks_[pattern][track].sound = sound; return true;
    }
    [[nodiscard]] bool setNote(std::uint32_t pattern, std::uint32_t track, std::uint32_t step,
                               std::uint32_t note, bool flag) noexcept {
        if (pattern>=4 || track>=4 || step>=16 || note<24 || note>84) return false;
        auto& data=tracks_[pattern][track]; data.notes[step]=note;
        if(flag) data.flags |= 1U<<step; else data.flags &= ~(1U<<step);
        return true;
    }
    [[nodiscard]] bool setTrack(std::uint32_t pattern, std::uint32_t track, std::uint32_t mask,
                               std::uint32_t accents, float level, bool muted) noexcept {
        if (pattern >= 4 || track >= 4 || mask > 65535 || accents > 65535
            || !std::isfinite(level) || level < 0 || level > 1) return false;
        auto& data = tracks_[pattern][track];
        data.mask=mask; data.accents=accents; data.level=level; data.muted=muted; groove_ = true;
        return true;
    }
    [[nodiscard]] bool selectPattern(std::uint32_t pattern) noexcept {
        if (pattern >= 4) return false;
        if (running_) queuedPattern_ = pattern;
        else { activePattern_ = pattern; queuedPattern_ = 4; }
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
            if (groove_ && nextStep_ == 0 && queuedPattern_ < 4) activePattern_ = queuedPattern_;
            const auto type = !groove_ && (mask_ & (1U << nextStep_)) ? AudioCommandType::trigger : AudioCommandType::sequenceStep;
            if (!core_.enqueueCommand({type, target, 0, 1.0F, generation_, nextStep_, activePattern_})) { stop(); return; }
            if (groove_) {
                for (std::uint32_t track = 0; track < 4; ++track) {
                    const auto& data = tracks_[activePattern_][track];
                    if (data.muted || !(data.mask & (1U << nextStep_))) continue;
                    const auto velocity = (data.accents & (1U << nextStep_)) ? 1.0F : 0.65F;
                    AudioCommand command{AudioCommandType::trigger, target, track + 1,
                        data.level * velocity, generation_, nextStep_, activePattern_};
                    command.sound=data.sound; command.note=data.notes[nextStep_];
                    command.accent=(data.accents & (1U<<nextStep_)) != 0;
                    command.openHat=track==2 && (data.flags & (1U<<nextStep_));
                    // Slide belongs to the outgoing step. Only adjacent enabled notes connect.
                    const auto next=(nextStep_+1)%16;
                    command.slide=track==3 && bassSlideNext_ && bassPattern_==activePattern_
                        && nextStep_==(bassStep_+1)%16 && target-bassFrame_ < period*1.5;
                    const bool sustain=track==3 && (data.flags & (1U<<nextStep_)) && (data.mask & (1U<<next));
                    command.gateFrames=static_cast<std::uint32_t>(std::clamp(period*(sustain ? 1.05L : .65L),1.0L,192000.0L));
                    if (!core_.enqueueCommand(command)) { stop(); return; }
                    if (track==3) { bassSlideNext_=sustain; bassPattern_=activePattern_; bassStep_=nextStep_; bassFrame_=target; }
                }
            }
            nextFrame_ += period; // retain fractional frames to avoid tempo drift
            nextStep_ = (nextStep_ + 1) % 16;
        }
    }
    [[nodiscard]] SequenceState state() const noexcept {
        const auto d = core_.diagnostics();
        return {bpm_, running_ ? 1U : 0U, mask_,
                running_ ? d.sequenceStep : 16U, missedSteps_,
                running_ && d.sequenceStep < 16 ? d.sequencePattern : activePattern_,
                running_ && queuedPattern_ != d.sequencePattern ? queuedPattern_ : 4U};
    }

private:
    std::array<std::array<Track, 4>, 4> tracks_{};
    bool groove_ = false, bassSlideNext_ = false;
    std::uint32_t bassPattern_=0,bassStep_=16;
    std::uint64_t bassFrame_=0;
    std::uint32_t activePattern_ = 0, queuedPattern_ = 4;
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
