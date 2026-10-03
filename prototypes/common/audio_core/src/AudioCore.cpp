#include "original_sequencer/prototype/AudioCore.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>

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
    playedSequenceGeneration_.store(0, std::memory_order_relaxed);
    playedSequenceStep_.store(16, std::memory_order_relaxed);
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
    clearBurst();
    commandQueue_.reset();
    hasPendingCommand_ = false;
    diagnosticSampleRate_.store(sampleRate, std::memory_order_relaxed);
    clearRealtimeDiagnostics();
    diagnosticAudioRestartCount_.store(audioRestartCount_, std::memory_order_relaxed);
}

void AudioCore::reset() noexcept {
    clearBurst();
    commandQueue_.reset();
    hasPendingCommand_ = false;
    clearRealtimeDiagnostics();
}

void AudioCore::shutdown() noexcept {
    initialized_ = false;
    sampleRate_ = 0.0;
    maxCallbackFrames_ = 0;
    clearBurst();
    commandQueue_.reset();
    hasPendingCommand_ = false;
    diagnosticSampleRate_.store(0.0, std::memory_order_relaxed);
    clearRealtimeDiagnostics();
}

bool AudioCore::enqueueCommand(const AudioCommand& command) noexcept {
    if ((command.type != AudioCommandType::trigger && command.type != AudioCommandType::sequenceStep)
        || (command.sequenceGeneration != 0 && command.sequenceStep > 16)
        || !std::isfinite(command.value)
        || command.value < 0.0F || command.value > 1.0F) return false;
    return commandQueue_.tryPush(command);
}

void AudioCore::clearBurst() noexcept {
    burstPhase_ = 0.0;
    burstAmplitude_ = 0.0F;
    burstRemaining_ = 0;
    burstLength_ = 0;
}

void AudioCore::renderBurst(const OutputView& output, std::uint32_t begin, std::uint32_t end) noexcept {
    if (sampleRate_ <= 0.0 || burstRemaining_ == 0) return;
    const auto increment = 2.0 * std::numbers::pi_v<double> * 220.0 / sampleRate_;
    for (auto frame = begin; frame < end && burstRemaining_ > 0; ++frame) {
        const auto envelope = static_cast<float>(burstRemaining_) / static_cast<float>(burstLength_);
        const auto value = static_cast<float>(std::cos(burstPhase_)) * burstAmplitude_ * envelope;
        for (std::uint32_t channel = 0; channel < output.channels; ++channel) {
            if (output.interleaved != nullptr)
                output.interleaved[static_cast<std::size_t>(frame) * output.channels + channel] = value;
            else if (output.planar != nullptr && output.planar[channel] != nullptr)
                output.planar[channel][static_cast<std::size_t>(output.offset) + frame] = value;
        }
        burstPhase_ += increment;
        if (burstPhase_ >= 2.0 * std::numbers::pi_v<double>) burstPhase_ -= 2.0 * std::numbers::pi_v<double>;
        --burstRemaining_;
    }
}

void AudioCore::consumeCommands(const OutputView& output, std::uint64_t callbackStartFrame, std::uint32_t frameCount) noexcept {
    if (frameCount == 0) return;
    std::uint32_t cursor = 0;
    // Bound work even if the producer keeps refilling during this callback.
    for (std::size_t processed = 0; processed < kCommandQueueCapacity + 1; ++processed) {
        if (!hasPendingCommand_) {
            if (!commandQueue_.tryPop(pendingCommand_)) break;
            hasPendingCommand_ = true;
        }
        if (pendingCommand_.sequenceGeneration != 0
            && pendingCommand_.sequenceGeneration != activeSequenceGeneration_.load(std::memory_order_acquire)) {
            hasPendingCommand_ = false;
            continue; // discard before timestamp check, allowing an earlier restart
        }
        // Subtraction also handles frame timelines near UINT64_MAX without overflow.
        if (pendingCommand_.targetFrame > callbackStartFrame
            && pendingCommand_.targetFrame - callbackStartFrame >= frameCount) break;
        const auto offset = std::max(cursor, pendingCommand_.targetFrame <= callbackStartFrame
            ? 0U : static_cast<std::uint32_t>(pendingCommand_.targetFrame - callbackStartFrame));
        renderBurst(output, cursor, offset);
        cursor = offset;
        if (pendingCommand_.sequenceGeneration != 0 && pendingCommand_.sequenceStep < 16) {
            playedSequenceStep_.store(pendingCommand_.sequenceStep, std::memory_order_relaxed);
            playedSequenceGeneration_.store(pendingCommand_.sequenceGeneration, std::memory_order_release);
        }
        if (pendingCommand_.type == AudioCommandType::sequenceStep) {
            hasPendingCommand_ = false;
            continue;
        }
        diagnosticTriggerCount_.fetch_add(1, std::memory_order_relaxed);
        diagnosticLastTriggerOffset_.store(offset, std::memory_order_relaxed);
        clearBurst();
        if (sampleRate_ > 0.0 && std::isfinite(sampleRate_)) {
            // P3 test voice: a monophonic 50 ms decaying cosine burst, max 8%.
            burstLength_ = static_cast<std::uint32_t>(std::clamp(sampleRate_ * 0.05, 1.0, 96000.0));
            burstRemaining_ = burstLength_;
            burstAmplitude_ = 0.08F * pendingCommand_.value;
        }
        hasPendingCommand_ = false;
    }
    renderBurst(output, cursor, frameCount);
}

void AudioCore::render(float* interleavedOutput, std::uint32_t frameCount, std::uint32_t channelCount, std::uint64_t callbackStartFrame) noexcept {
    renderOutput(OutputView{interleavedOutput, nullptr, channelCount, 0}, frameCount, callbackStartFrame);
}

void AudioCore::renderPlanar(float* const* output, std::uint32_t frameCount, std::uint32_t channelCount, std::uint64_t callbackStartFrame, std::uint32_t outputOffset) noexcept {
    renderOutput(OutputView{nullptr, output, channelCount, outputOffset}, frameCount, callbackStartFrame);
}

void AudioCore::renderOutput(const OutputView& output, std::uint32_t frameCount, std::uint64_t callbackStartFrame) noexcept {
    diagnosticCallbackStartFrame_.store(callbackStartFrame, std::memory_order_relaxed);
    diagnosticCallbackFrames_.store(frameCount, std::memory_order_relaxed);
    diagnosticRenderedFrames_.fetch_add(frameCount, std::memory_order_relaxed);
    if (output.interleaved != nullptr)
        std::fill_n(output.interleaved, static_cast<std::size_t>(frameCount) * output.channels, 0.0F);
    else if (output.planar != nullptr) {
        for (std::uint32_t channel = 0; channel < output.channels; ++channel) {
            if (output.planar[channel] != nullptr)
                std::fill_n(output.planar[channel] + output.offset, frameCount, 0.0F);
        }
    }
    consumeCommands(output, callbackStartFrame, frameCount);
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
        .queueDepth = static_cast<std::uint32_t>(commandQueue_.depth() + (hasPendingCommand_.load(std::memory_order_relaxed) ? 1U : 0U)),
        .queueHighWaterMark = static_cast<std::uint32_t>(commandQueue_.highWaterMark()),
        .queueOverflowCount = commandQueue_.overflowCount(),
        .triggerCount = diagnosticTriggerCount_.load(std::memory_order_relaxed),
        .lastTriggerOffset = diagnosticLastTriggerOffset_.load(std::memory_order_relaxed),
        .sequenceStep = playedSequenceGeneration_.load(std::memory_order_acquire) == activeSequenceGeneration_.load(std::memory_order_acquire)
            ? playedSequenceStep_.load(std::memory_order_relaxed) : 16U,
    };
}

}  // namespace original_sequencer::prototype
