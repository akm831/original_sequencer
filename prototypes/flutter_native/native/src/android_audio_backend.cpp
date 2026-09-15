#if defined(__ANDROID__)

#include "android_audio_backend.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace original_sequencer::prototype::flutter_native {
namespace {
constexpr double kTestToneFrequencyHz = 220.0;
constexpr float kTestToneAmplitude = 0.08F;
}

AndroidAudioBackend::AndroidAudioBackend(AudioCore& core) noexcept : core_(core) {}

AndroidAudioBackend::~AndroidAudioBackend() {
    stop();
}

bool AndroidAudioBackend::start() noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    shouldRun_.store(true, std::memory_order_release);

    if (stream_) {
        return true;
    }

    if (!openStreamLocked()) {
        shouldRun_.store(false, std::memory_order_release);
        return false;
    }

    return true;
}

bool AndroidAudioBackend::openStreamLocked() noexcept {
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Stereo);
    builder.setDataCallback(this);
    builder.setErrorCallback(this);

    std::shared_ptr<oboe::AudioStream> stream;
    const auto openResult = builder.openStream(stream);
    if (openResult != oboe::Result::OK || !stream) {
        return false;
    }

    const auto sampleRate = stream->getSampleRate();
    const auto framesPerBurst = stream->getFramesPerBurst();
    const auto maxCallbackFrames = static_cast<std::uint32_t>(
        std::max<std::int32_t>(framesPerBurst > 0 ? framesPerBurst * 2 : 0, 256));

    core_.initialize(static_cast<double>(sampleRate), maxCallbackFrames);
    callbackStartFrame_ = 0;
    sinePhase_ = 0.0;
    stream_ = std::move(stream);

    const auto startResult = stream_->requestStart();
    if (startResult != oboe::Result::OK) {
        stream_->close();
        stream_.reset();
        return false;
    }

    return true;
}

void AndroidAudioBackend::stop() noexcept {
    std::shared_ptr<oboe::AudioStream> streamToClose;
    {
        std::lock_guard<std::mutex> lock(streamMutex_);
        shouldRun_.store(false, std::memory_order_release);
        streamToClose = std::move(stream_);
    }

    if (streamToClose) {
        streamToClose->requestStop();
        streamToClose->close();
    }

    core_.shutdown();
}

void AndroidAudioBackend::onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) {
    if (error != oboe::Result::ErrorDisconnected ||
        !shouldRun_.load(std::memory_order_acquire)) {
        return;
    }

    std::lock_guard<std::mutex> lock(streamMutex_);
    if (!shouldRun_.load(std::memory_order_acquire)) {
        return;
    }

    if (stream_ && stream_.get() == audioStream) {
        stream_.reset();
    }

    // Oboe invokes this callback after the old stream has already been stopped and closed.
    // Reopening here establishes a new device stream and intentionally resets the frame origin.
    (void)openStreamLocked();
}

oboe::DataCallbackResult AndroidAudioBackend::onAudioReady(oboe::AudioStream* audioStream,
                                                            void* audioData,
                                                            std::int32_t numFrames) {
    if (audioStream == nullptr || audioData == nullptr || numFrames <= 0) {
        return oboe::DataCallbackResult::Continue;
    }

    const auto channelCount = static_cast<std::uint32_t>(audioStream->getChannelCount());
    const auto frameCount = static_cast<std::uint32_t>(numFrames);
    auto* output = static_cast<float*>(audioData);

    core_.render(output, frameCount, channelCount, callbackStartFrame_);

    const auto sampleRate = static_cast<double>(audioStream->getSampleRate());
    if (sampleRate > 0.0 && channelCount > 0) {
        const auto phaseIncrement = 2.0 * std::numbers::pi_v<double> * kTestToneFrequencyHz / sampleRate;
        for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
            const auto sample = static_cast<float>(std::sin(sinePhase_)) * kTestToneAmplitude;
            const auto frameOffset = static_cast<std::size_t>(frame) * channelCount;
            for (std::uint32_t channel = 0; channel < channelCount; ++channel) {
                output[frameOffset + channel] += sample;
            }

            sinePhase_ += phaseIncrement;
            if (sinePhase_ >= 2.0 * std::numbers::pi_v<double>) {
                sinePhase_ -= 2.0 * std::numbers::pi_v<double>;
            }
        }
    }

    callbackStartFrame_ += frameCount;
    return oboe::DataCallbackResult::Continue;
}

}  // namespace original_sequencer::prototype::flutter_native

#endif
