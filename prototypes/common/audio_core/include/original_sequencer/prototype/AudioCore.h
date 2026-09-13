#pragma once

#include <cstdint>

namespace original_sequencer::prototype {

struct DiagnosticsSnapshot {
    double sampleRate = 0.0;
    std::uint32_t callbackFrames = 0;
    std::uint64_t renderedFrames = 0;
    std::uint32_t audioRestartCount = 0;
};

class AudioCore {
public:
    void initialize(double sampleRate, std::uint32_t maxCallbackFrames) noexcept;
    void reset() noexcept;
    void shutdown() noexcept;

    void render(float* interleavedOutput,
                std::uint32_t frameCount,
                std::uint32_t channelCount,
                std::uint64_t callbackStartFrame) noexcept;

    [[nodiscard]] DiagnosticsSnapshot diagnostics() const noexcept;

private:
    double sampleRate_ = 0.0;
    std::uint32_t maxCallbackFrames_ = 0;
    std::uint32_t lastCallbackFrames_ = 0;
    std::uint64_t renderedFrames_ = 0;
    std::uint32_t audioRestartCount_ = 0;
    bool initialized_ = false;
};

}  // namespace original_sequencer::prototype
