#pragma once

#include "original_sequencer/prototype/AudioCommandQueue.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace original_sequencer::prototype {

struct DiagnosticsSnapshot {
    double sampleRate = 0.0;
    std::uint32_t callbackFrames = 0;
    std::uint32_t callbackFramesMin = 0;
    std::uint32_t callbackFramesMax = 0;
    std::uint64_t renderedFrames = 0;
    std::uint64_t callbackStartFrame = 0;
    double callbackDurationUs = 0.0;
    double callbackLoad = 0.0;
    double callbackLoadP95 = 0.0;
    double callbackLoadP99 = 0.0;
    double callbackLoadPeak = 0.0;
    std::uint32_t audioRestartCount = 0;
    std::uint32_t queueDepth = 0;
    std::uint32_t queueHighWaterMark = 0;
    std::uint64_t queueOverflowCount = 0;
    std::uint64_t triggerCount = 0;
    std::uint32_t lastTriggerOffset = 0;
};

class AudioCore {
public:
    static constexpr std::size_t kCommandQueueCapacity = 256;

    void initialize(double sampleRate, std::uint32_t maxCallbackFrames) noexcept;
    void reset() noexcept;
    void shutdown() noexcept;
    [[nodiscard]] bool enqueueCommand(const AudioCommand& command) noexcept;
    void render(float* interleavedOutput, std::uint32_t frameCount, std::uint32_t channelCount, std::uint64_t callbackStartFrame) noexcept;
    void recordCallbackTiming(std::uint64_t callbackStartFrame, std::uint32_t frameCount, double durationUs) noexcept;
    [[nodiscard]] DiagnosticsSnapshot diagnostics() const noexcept;

private:
    static constexpr std::size_t kLoadHistogramBuckets = 1001;
    static constexpr double kLoadBucketWidth = 0.001;
    void clearRealtimeDiagnostics() noexcept;
    void consumeCommands(std::uint64_t callbackStartFrame, std::uint32_t frameCount) noexcept;
    [[nodiscard]] double loadPercentile(double percentile) const noexcept;

    double sampleRate_ = 0.0;
    std::uint32_t maxCallbackFrames_ = 0;
    std::uint32_t audioRestartCount_ = 0;
    bool initialized_ = false;
    AudioCommandQueue<kCommandQueueCapacity> commandQueue_{};
    bool hasPendingCommand_ = false;
    AudioCommand pendingCommand_{};

    std::atomic<double> diagnosticSampleRate_{0.0};
    std::atomic<std::uint32_t> diagnosticCallbackFrames_{0};
    std::atomic<std::uint32_t> diagnosticCallbackFramesMin_{0};
    std::atomic<std::uint32_t> diagnosticCallbackFramesMax_{0};
    std::atomic<std::uint64_t> diagnosticRenderedFrames_{0};
    std::atomic<std::uint64_t> diagnosticCallbackStartFrame_{0};
    std::atomic<double> diagnosticCallbackDurationUs_{0.0};
    std::atomic<double> diagnosticCallbackLoad_{0.0};
    std::atomic<double> diagnosticCallbackLoadPeak_{0.0};
    std::array<std::atomic<std::uint64_t>, kLoadHistogramBuckets> loadHistogram_{};
    std::atomic<std::uint64_t> loadHistogramSamples_{0};
    std::atomic<std::uint32_t> diagnosticAudioRestartCount_{0};
    std::atomic<std::uint64_t> diagnosticTriggerCount_{0};
    std::atomic<std::uint32_t> diagnosticLastTriggerOffset_{0};
};

}  // namespace original_sequencer::prototype
