#include "original_sequencer/prototype/AudioCore.h"

#include <algorithm>
#include <cstddef>

namespace original_sequencer::prototype {

void AudioCore::initialize(double sampleRate, std::uint32_t maxCallbackFrames) noexcept {
    if (initialized_) {
        ++audioRestartCount_;
    }
    sampleRate_ = sampleRate;
    maxCallbackFrames_ = maxCallbackFrames;
    lastCallbackFrames_ = 0;
    renderedFrames_ = 0;
    initialized_ = true;
}

void AudioCore::reset() noexcept {
    lastCallbackFrames_ = 0;
    renderedFrames_ = 0;
}

void AudioCore::shutdown() noexcept {
    initialized_ = false;
    sampleRate_ = 0.0;
    maxCallbackFrames_ = 0;
    lastCallbackFrames_ = 0;
    renderedFrames_ = 0;
}

void AudioCore::render(float* interleavedOutput,
                       std::uint32_t frameCount,
                       std::uint32_t channelCount,
                       std::uint64_t callbackStartFrame) noexcept {
    (void)callbackStartFrame;
    lastCallbackFrames_ = frameCount;
    renderedFrames_ += frameCount;

    if (interleavedOutput == nullptr || channelCount == 0) {
        return;
    }

    const auto sampleCount = static_cast<std::size_t>(frameCount) * channelCount;
    std::fill_n(interleavedOutput, sampleCount, 0.0F);
}

DiagnosticsSnapshot AudioCore::diagnostics() const noexcept {
    return DiagnosticsSnapshot{
        .sampleRate = sampleRate_,
        .callbackFrames = lastCallbackFrames_,
        .renderedFrames = renderedFrames_,
        .audioRestartCount = audioRestartCount_,
    };
}

}  // namespace original_sequencer::prototype
