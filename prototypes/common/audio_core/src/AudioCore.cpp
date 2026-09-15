#include "original_sequencer/prototype/AudioCore.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace original_sequencer::prototype {

void AudioCore::clearRealtimeDiagnostics() noexcept {
    diagnosticCallbackFrames_.store(0, std::memory_order_relaxed);
    diagnosticCallbackFramesMin_.store(0, std::memory_order_relaxed);
    diagnosticCallbackFramesMax_.store(0, std::memory_order_relaxed);
    diagnosticRenderedFrames_.store(0, std::memory_order_relaxed);
    diagnosticCallbackStartFrame_.store(0, std::memory_order_relaxed);
    diagnosticCallbackDurationUs_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoad_.store(0.0, std::memory_order_relaxed);
    diagnosticCallbackLoadPeak_.store(0.0, std::memory_order_relaxed);
    diagnosticTriggerCount_.store(0, std::memory_order_relaxed);
    diagnosticLastTriggerOffset_.store(0, std::memory_order_relaxed);
    for (auto& bucket : loadHistogram_) bucket.store(0, std::memory_order_relaxed);
    loadHistogramSamples_.store(0, std::memory_order_relaxed);
}

void AudioCore::initialize(double sampleRate, std::uint32_t maxCallbackFrames) noexcept {
    if (initialized_) ++audioRestartCount_;
    sampleRate_ = sampleRate;
    maxCallbackFrames_ = maxCallbackFrames;
    initialized_ = true;
    commandQueue_.reset();
    hasPendingCommand_ = false;
    diagnosticSampleRate_.store(sampleRate, std::memory_order_relaxed);
    clearRealtimeDiagnostics();
    diagnosticAudioRestartCount_.store(audioRestartCount_, std::memory_order_relaxed);
}

void AudioCore::reset() noexcept {
    commandQueue_.reset();
    hasPendingCommand_ = false;
    clearRealtimeDiagnostics();
}

void AudioCore::shutdown() noexcept {
    initialized_ = false;
    sampleRate_ = 0.0;
    maxCallbackFrames_ = 0;
    commandQueue_.reset();
    hasPendingCommand_ = false;
    diagnosticSampleRate_.store(0.0, std::memory_order_relaxed);
    clearRealtimeDiagnostics();
}

bool AudioCore::enqueueCommand(const AudioCommand& command) noexcept {
    return commandQueue_.tryPush(command);
}

void AudioCore::consumeCommands(std::uint64_t callbackStartFrame, std::uint32_t frameCount) noexcept {
    if (frameCount == 0) return;
    const auto callbackEndFrame = callbackStartFrame > std::numeric_limits<std::uint64_t>::max() - frameCount
        ? std::numeric_limits<std::uint64_t>::max()
        : callbackStartFrame + frameCount;

    for (;;) {
        if (!hasPendingCommand_) {
            if (!commandQueue_.tryPop(pendingCommand_)) return;
            hasPendingCommand_ = true;
        }

        if (pendingCommand_.targetFrame >= callbackEndFrame) return;

        const auto offset = pendingCommand_.targetFrame <= callbackStartFrame
            ? 0U
            : static_cast<std::uint32_t>(pendingCommand_.targetFrame - callbackStartFrame);

        if (pendingCommand_.type == AudioCommandType::trigger) {
            diagnosticTriggerCount_.fetch_add(1, std::memory_order_relaxed);
            diagnosticLastTriggerOffset_.store(offset, std::memory_order_relaxed);
        }
        hasPendingCommand_ = false;
    }
}

void AudioCore::render(float* interleavedOutput, std::uint32_t frameCount, std::uint32_t channelCount, std::uint64_t callbackStartFrame) noexcept {
    diagnosticCallbackStartFrame_.store(callbackStartFrame, std::memory_order_relaxed);
    diagnosticCallbackFrames_.store(frameCount, std::memory_order_relaxed);
    diagnosticRenderedFrames_.fetch_add(frameCount, std::memory_order_relaxed);
    consumeCommands(callbackStartFrame, frameCount);
    if (interleavedOutput == nullptr || channelCount == 0) return;
    std::fill_n(interleavedOutput, static_cast<std::size_t>(frameCount) * channelCount, 0.0F);
}

void AudioCore::recordCallbackTiming(std::uint64_t callbackStartFrame, std::uint32_t frameCount, double durationUs) noexcept {
    diagnosticCallbackStartFrame_.store(callbackStartFrame, std::memory_order_relaxed);
    diagnosticCallbackDurationUs_.store(durationUs, std::memory_order_relaxed);

    auto minFrames = diagnosticCallbackFramesMin_.load(std::memory_order_relaxed);
    while ((minFrames == 0 || frameCount < minFrames) && !diagnosticCallbackFramesMin_.compare_exchange_weak(minFrames, frameCount, std::memory_order_relaxed)) {}
    auto maxFrames = diagnosticCallbackFramesMax_.load(std::memory_order_relaxed);
    while (frameCount > maxFrames && !diagnosticCallbackFramesMax_.compare_exchange_weak(maxFrames, frameCount, std::memory_order_relaxed)) {}

    const auto sampleRate = diagnosticSampleRate_.load(std::memory_order_relaxed);
    double load = 0.0;
    if (sampleRate > 0.0 && frameCount > 0) {
        const auto budgetUs = static_cast<double>(frameCount) * 1000000.0 / sampleRate;
        if (budgetUs > 0.0) load = durationUs / budgetUs;
    }
    diagnosticCallbackLoad_.store(load, std::memory_order_relaxed);
    auto peak = diagnosticCallbackLoadPeak_.load(std::memory_order_relaxed);
    while (load > peak && !diagnosticCallbackLoadPeak_.compare_exchange_weak(peak, load, std::memory_order_relaxed)) {}

    const auto bucket = static_cast<std::size_t>(std::clamp(load / kLoadBucketWidth, 0.0, static_cast<double>(kLoadHistogramBuckets - 1)));
    loadHistogram_[bucket].fetch_add(1, std::memory_order_relaxed);
    loadHistogramSamples_.fetch_add(1, std::memory_order_relaxed);
}

double AudioCore::loadPercentile(double percentile) const noexcept {
    const auto total = loadHistogramSamples_.load(std::memory_order_relaxed);
    if (total == 0) return 0.0;
    const auto target = static_cast<std::uint64_t>(std::ceil(percentile * static_cast<double>(total)));
    std::uint64_t cumulative = 0;
    for (std::size_t i = 0; i < loadHistogram_.size(); ++i) {
        cumulative += loadHistogram_[i].load(std::memory_order_relaxed);
        if (cumulative >= target) return static_cast<double>(i) * kLoadBucketWidth;
    }
    return static_cast<double>(kLoadHistogramBuckets - 1) * kLoadBucketWidth;
}

DiagnosticsSnapshot AudioCore::diagnostics() const noexcept {
    return DiagnosticsSnapshot{
        .sampleRate = diagnosticSampleRate_.load(std::memory_order_relaxed),
        .callbackFrames = diagnosticCallbackFrames_.load(std::memory_order_relaxed),
        .callbackFramesMin = diagnosticCallbackFramesMin_.load(std::memory_order_relaxed),
        .callbackFramesMax = diagnosticCallbackFramesMax_.load(std::memory_order_relaxed),
        .renderedFrames = diagnosticRenderedFrames_.load(std::memory_order_relaxed),
        .callbackStartFrame = diagnosticCallbackStartFrame_.load(std::memory_order_relaxed),
        .callbackDurationUs = diagnosticCallbackDurationUs_.load(std::memory_order_relaxed),
        .callbackLoad = diagnosticCallbackLoad_.load(std::memory_order_relaxed),
        .callbackLoadP95 = loadPercentile(0.95),
        .callbackLoadP99 = loadPercentile(0.99),
        .callbackLoadPeak = diagnosticCallbackLoadPeak_.load(std::memory_order_relaxed),
        .audioRestartCount = diagnosticAudioRestartCount_.load(std::memory_order_relaxed),
        .queueDepth = static_cast<std::uint32_t>(commandQueue_.depth() + (hasPendingCommand_ ? 1U : 0U)),
        .queueHighWaterMark = static_cast<std::uint32_t>(commandQueue_.highWaterMark()),
        .queueOverflowCount = commandQueue_.overflowCount(),
        .triggerCount = diagnosticTriggerCount_.load(std::memory_order_relaxed),
        .lastTriggerOffset = diagnosticLastTriggerOffset_.load(std::memory_order_relaxed),
    };
}

}  // namespace original_sequencer::prototype
