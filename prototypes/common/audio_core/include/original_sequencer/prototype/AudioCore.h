#pragma once

#include <array>
#include <atomic>
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
};

class AudioCore {
public:
    void initialize(double sampleRate, std::uint32_t maxCallbackFrames) noexcept;
    void reset() noexcept;
    void shutdown() noexcept;
    void render(float* interleavedOutput, std::uint32_t frameCount, std::uint32_t channelCount, std::uint64_t callbackStartFrame) noexcept;
    void recordCallbackTiming(std::uint64_t callbackStartFrame, std::uint32_t frameCount, double durationUs) noexcept;
    [[nodiscard]] DiagnosticsSnapshot diagnostics() const noexcept;

private:
    // 0.1 percentage point buckets from 0% through 100%; final bucket also contains >100%.
    // This keeps sub-1% prototype loads visible without allocating or sorting on the audio thread.
    static constexpr std::size_t kLoadHistogramBuckets = 1001;
    static constexpr double kLoadBucketWidth = 0.001;
    void clearRealtimeDiagnostics() noexcept;
    [[nodiscard]] double loadPercentile(double percentile) const noexcept;

    double sampleRate_ = 0.0;
    std::uint32_t maxCallbackFrames_ = 0;
    std::uint32_t audioRestartCount_ = 0;
    bool initialized_ = false;

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
};

}  // namespace original_sequencer::prototype
