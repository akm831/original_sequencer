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
    diagnosticCallbackStartFrame_.store(0, std::memory_order_relaxed);
    diagnosticCallbackDurationUs_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoad_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoadPeak_.store(0.0, std::memory_order_relaxed);
    diagnosticAudioRestartCount_.store(audioRestartCount_, std::memory_order_relaxed);
}

void AudioCore::reset() noexcept {
    diagnosticCallbackFrames_.store(0, std::memory_order_relaxed);
    diagnosticRenderedFrames_.store(0, std::memory_order_relaxed);
    diagnosticCallbackStartFrame_.store(0, std::memory_order_relaxed);
    diagnosticCallbackDurationUs_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoad_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoadPeak_.store(0.0, std::memory_order_relaxed);
}

void AudioCore::shutdown() noexcept {
    initialized_ = false;
    sampleRate_ = 0.0;
    maxCallbackFrames_ = 0;

    diagnosticSampleRate_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackFrames_.store(0, std::memory_order_relaxed);
    diagnosticRenderedFrames_.store(0, std::memory_order_relaxed);
    diagnosticCallbackStartFrame_.store(0, std::memory_order_relaxed);
    diagnosticCallbackDurationUs_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoad_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoadPeak_.store(0.0, std::memory_order_relaxed);
}

void AudioCore::render(float* interleavedOutput,
                       std::uint32_t frameCount,
                       std::uint32_t channelCount,
                       std::uint64_t callbackStartFrame) noexcept {
    diagnosticCallbackStartFrame_.store(callbackStartFrame, std::memory_order_relaxed);
    diagnosticCallbackFrames_.store(frameCount, std::memory_order_relaxed);
    diagnosticRenderedFrames_.fetch_add(frameCount, std::memory_order_relaxed);

    if (interleavedOutput == nullptr || channelCount == 0) {
        return;
    }

    const auto sampleCount = static_cast<std::size_t>(frameCount) * channelCount;
    std::fill_n(interleavedOutput, sampleCount, 0.0F);
}

void AudioCore::recordCallbackTiming(std::uint64_t callbackStartFrame,
                                     std::uint32_t frameCount,
                                     double durationUs) noexcept {
    diagnosticCallbackStartFrame_.store(callbackStartFrame, std::memory_order_relaxed);
    diagnosticCallbackDurationUs_.store(durationUs, std::memory_order_relaxed);

    const auto sampleRate = diagnosticSampleRate_.load(std::memory_order_relaxed);
    double load = 0.0;
    if (sampleRate > 0.0 && frameCount > 0) {
        const auto budgetUs = static_cast<double>(frameCount) * 1000000.0 / sampleRate;
        if (budgetUs > 0.0) {
            load = durationUs / budgetUs;
        }
    }
    diagnosticCallbackLoad_.store(load, std::memory_order_relaxed);

    auto peak = diagnosticCallbackLoadPeak_.load(std::memory_order_relaxed);
    while (load > peak &&
           !diagnosticCallbackLoadPeak_.compare_exchange_weak(
               peak, load, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

DiagnosticsSnapshot AudioCore::diagnostics() const noexcept {
    return DiagnosticsSnapshot{
        .sampleRate = diagnosticSampleRate_.load(std::memory_order_relaxed),
        .callbackFrames = diagnosticCallbackFrames_.load(std::memory_order_relaxed),
        .renderedFrames = diagnosticRenderedFrames_.load(std::memory_order_relaxed),
        .callbackStartFrame = diagnosticCallbackStartFrame_.load(std::memory_order_relaxed),
        .callbackDurationUs = diagnosticCallbackDurationUs_.load(std::memory_order_relaxed),
        .callbackLoad = diagnosticCallbackLoad_.load(std::memory_order_relaxed),
        .callbackLoadPeak = diagnosticCallbackLoadPeak_.load(std::memory_order_relaxed),
        .audioRestartCount = diagnosticAudioRestartCount_.load(std::memory_order_relaxed),
    };
}

}  // namespace original_sequencer::prototype
