#pragma once

#if defined(__ANDROID__)

#include <cstdint>
#include <memory>

#include <oboe/Oboe.h>

#include "original_sequencer/prototype/AudioCore.h"

namespace original_sequencer::prototype::flutter_native {

class AndroidAudioBackend final : public oboe::AudioStreamDataCallback {
public:
    explicit AndroidAudioBackend(AudioCore& core) noexcept;
    ~AndroidAudioBackend() override;

    AndroidAudioBackend(const AndroidAudioBackend&) = delete;
    AndroidAudioBackend& operator=(const AndroidAudioBackend&) = delete;

    [[nodiscard]] bool start() noexcept;
    void stop() noexcept;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream,
                                           void* audioData,
                                           std::int32_t numFrames) override;

private:
    AudioCore& core_;
    std::shared_ptr<oboe::AudioStream> stream_;
    std::uint64_t callbackStartFrame_ = 0;
    double sinePhase_ = 0.0;
};

}  // namespace original_sequencer::prototype::flutter_native

#endif
