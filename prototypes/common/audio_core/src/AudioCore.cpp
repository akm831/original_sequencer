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
    initialized_ = true;

    diagnosticSampleRate_.store(sampleRate, std::memory_order_relaxed);
    diagnosticCallbackFrames_.store(0, std::memory_order_relaxed);
    diagnosticRenderedFrames_.store(0, std::memory_order_relaxed);
    diagnosticAudioRestartCount_.store(audioRestartCount_, std::memory_order_relaxed);
}

void AudioCore::reset() noexcept {
    diagnosticCallbackFrames_.store(0, std::memory_order_relaxed);
    diagnosticRenderedFrames_.store(0, std::memory_order_relaxed);
}

void AudioCore::shutdown() noexcept {
    initialized_ = false;
    sampleRate_ = 0.0;
    maxCallbackFrames_ = 0;

    diagnosticSampleRate_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackFrames_.store(0, std::memory_order_relaxed);
    diagnosticRenderedFrames_.store(0, std::memory_order_relaxed);
}

void AudioCore::render(float* interleavedOutput,
                       std::uint32_t frameCount,
                       std::uint32_t channelCount,
                       std::uint64_t callbackStartFrame) noexcept {
    (void)callbackStartFrame;
    diagnosticCallbackFrames_.store(frameCount, std::memory_order_relaxed);
    diagnosticRenderedFrames_.fetch_add(frameCount, std::memory_order_relaxed);

    if (interleavedOutput == nullptr || channelCount == 0) {
        return;
    }

    const auto sampleCount = static_cast<std::size_t>(frameCount) * channelCount;
    std::fill_n(interleavedOutput, sampleCount, 0.0F);
}

DiagnosticsSnapshot AudioCore::diagnostics() const noexcept {
    return DiagnosticsSnapshot{
        .sampleRate = diagnosticSampleRate_.load(std::memory_order_relaxed),
        .callbackFrames = diagnosticCallbackFrames_.load(std::memory_order_relaxed),
        .renderedFrames = diagnosticRenderedFrames_.load(std::memory_order_relaxed),
        .audioRestartCount = diagnosticAudioRestartCount_.load(std::memory_order_relaxed),
    };
}

}  // namespace original_sequencer::prototype
