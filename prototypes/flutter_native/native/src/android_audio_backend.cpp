#if defined(__ANDROID__)

#include "android_audio_backend.h"

#include <algorithm>
#include <chrono>
#include <cstddef>

namespace original_sequencer::prototype::flutter_native {

AndroidAudioBackend::AndroidAudioBackend(AudioCore& core) noexcept : core_(core) {
    schedulerThread_ = std::thread([this] {
        while (!exitScheduler_.load(std::memory_order_acquire)) {
            {
                std::lock_guard<std::mutex> lock(streamMutex_);
                if (!closing_ && stream_ && shouldRun_.load(std::memory_order_acquire)) sequencer_.advance();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
}

AndroidAudioBackend::~AndroidAudioBackend() {
    exitScheduler_.store(true, std::memory_order_release);
    if (schedulerThread_.joinable()) schedulerThread_.join();
    stop();
}

bool AndroidAudioBackend::start() noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    if (closing_) return false;
    shouldRun_.store(true, std::memory_order_release);
    if (stream_) return true;
    if (!openStreamLocked()) {
        core_.shutdown();
        sequencer_.deviceReset();
        triggerInput_.reset();
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
    if (openResult != oboe::Result::OK || !stream) return false;

    const auto sampleRate = stream->getSampleRate();
    const auto framesPerBurst = stream->getFramesPerBurst();
    const auto maxCallbackFrames = static_cast<std::uint32_t>(
        std::max<std::int32_t>(framesPerBurst > 0 ? framesPerBurst * 2 : 0, 256));
    core_.initialize(static_cast<double>(sampleRate), maxCallbackFrames);
    sequencer_.deviceReset();
    triggerInput_.reset();
    callbackStartFrame_ = 0;
    stream_ = std::move(stream);

    const auto startResult = stream_->requestStart();
    if (startResult != oboe::Result::OK) {
        stream_->close();
        stream_.reset();
        core_.shutdown();
        sequencer_.deviceReset();
        triggerInput_.reset();
        return false;
    }
    return true;
}

void AndroidAudioBackend::stop() noexcept {
    std::shared_ptr<oboe::AudioStream> streamToClose;
    {
        std::lock_guard<std::mutex> lock(streamMutex_);
        if (closing_) return;
        sequencer_.stop();
        closing_ = true;
        shouldRun_.store(false, std::memory_order_release);
        streamToClose = std::move(stream_);
    }
    if (streamToClose) {
        streamToClose->requestStop();
        streamToClose->close();
    }
    std::lock_guard<std::mutex> lock(streamMutex_);
    core_.shutdown();
    sequencer_.deviceReset();
    triggerInput_.reset();
    closing_ = false;
}

void AndroidAudioBackend::onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) {
    if (error != oboe::Result::ErrorDisconnected || !shouldRun_.load(std::memory_order_acquire)) return;
    std::lock_guard<std::mutex> lock(streamMutex_);
    if (!shouldRun_.load(std::memory_order_acquire)) return;
    if (closing_ || !stream_ || stream_.get() != audioStream) return;
    stream_.reset();
    if (!openStreamLocked()) {
        core_.shutdown();
        sequencer_.deviceReset();
        triggerInput_.reset();
        shouldRun_.store(false, std::memory_order_release);
    }
}

bool AndroidAudioBackend::scheduleTrigger(std::uint32_t delayFrames, float value) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    if (closing_ || !stream_ || !shouldRun_.load(std::memory_order_acquire)) return false;
    if (sequencer_.state().running) return false;
    return triggerInput_.submit(delayFrames, value);
}

bool AndroidAudioBackend::setPlaying(bool playing) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    if (!playing) { sequencer_.stop(); return true; }
    if (closing_ || !stream_ || !shouldRun_.load(std::memory_order_acquire)) return false;
    return sequencer_.play();
}

bool AndroidAudioBackend::setBpm(double bpm) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    return sequencer_.setBpm(bpm);
}

bool AndroidAudioBackend::setStep(std::uint32_t step, bool enabled) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    return sequencer_.setStep(step, enabled);
}

bool AndroidAudioBackend::setTrack(std::uint32_t pattern, std::uint32_t track, std::uint32_t mask, std::uint32_t accents, float level, bool muted) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    return sequencer_.setTrack(pattern, track, mask, accents, level, muted);
}

bool AndroidAudioBackend::selectPattern(std::uint32_t pattern) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    return sequencer_.selectPattern(pattern);
}

SequenceState AndroidAudioBackend::sequenceState() noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    return sequencer_.state();
}

void AndroidAudioBackend::initializeWhenStopped(double sampleRate, std::uint32_t maxCallbackFrames) noexcept {
    std::lock_guard<std::mutex> lock(streamMutex_);
    if (closing_ || stream_) return;
    core_.initialize(sampleRate, maxCallbackFrames);
    sequencer_.deviceReset();
    triggerInput_.reset();
}

oboe::DataCallbackResult AndroidAudioBackend::onAudioReady(oboe::AudioStream* audioStream,
                                                            void* audioData,
                                                            std::int32_t numFrames) {
    if (audioStream == nullptr || audioData == nullptr || numFrames <= 0) {
        return oboe::DataCallbackResult::Continue;
    }

    const auto callbackBegin = std::chrono::steady_clock::now();
    const auto currentStartFrame = callbackStartFrame_;
    const auto channelCount = static_cast<std::uint32_t>(audioStream->getChannelCount());
    const auto frameCount = static_cast<std::uint32_t>(numFrames);
    auto* output = static_cast<float*>(audioData);

    core_.render(output, frameCount, channelCount, currentStartFrame);
    const auto callbackEnd = std::chrono::steady_clock::now();
    const auto durationUs = std::chrono::duration<double, std::micro>(callbackEnd - callbackBegin).count();
    core_.recordCallbackTiming(currentStartFrame, frameCount, durationUs);
    callbackStartFrame_ += frameCount;
    return oboe::DataCallbackResult::Continue;
}

}  // namespace original_sequencer::prototype::flutter_native

#endif
