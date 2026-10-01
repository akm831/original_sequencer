#pragma once

#if defined(__ANDROID__)

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>

#include <oboe/Oboe.h>

#include "original_sequencer/prototype/AudioCore.h"
#include "original_sequencer/prototype/ScheduledTriggerInput.h"

namespace original_sequencer::prototype::flutter_native {

class AndroidAudioBackend final : public oboe::AudioStreamDataCallback,
                                  public oboe::AudioStreamErrorCallback {
public:
    explicit AndroidAudioBackend(AudioCore& core) noexcept;
    ~AndroidAudioBackend() override;

    AndroidAudioBackend(const AndroidAudioBackend&) = delete;
    AndroidAudioBackend& operator=(const AndroidAudioBackend&) = delete;

    [[nodiscard]] bool start() noexcept;
    void stop() noexcept;
    [[nodiscard]] bool scheduleTrigger(std::uint32_t delayFrames, float value) noexcept;
    void initializeWhenStopped(double sampleRate, std::uint32_t maxCallbackFrames) noexcept;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream,
                                           void* audioData,
                                           std::int32_t numFrames) override;

    void onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) override;

private:
    [[nodiscard]] bool openStreamLocked() noexcept;

    AudioCore& core_;
    ScheduledTriggerInput triggerInput_{core_};
    std::mutex streamMutex_;
    bool closing_ = false;
    std::shared_ptr<oboe::AudioStream> stream_;
    std::atomic<bool> shouldRun_{false};
    std::uint64_t callbackStartFrame_ = 0;
};

}  // namespace original_sequencer::prototype::flutter_native

#endif
