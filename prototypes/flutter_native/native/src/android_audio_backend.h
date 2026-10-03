#pragma once

#if defined(__ANDROID__)

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>

#include <oboe/Oboe.h>

#include "original_sequencer/prototype/AudioCore.h"
#include "original_sequencer/prototype/ScheduledTriggerInput.h"
#include "original_sequencer/prototype/PrototypeSequencer.h"

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
    [[nodiscard]] bool setPlaying(bool playing) noexcept;
    [[nodiscard]] bool setBpm(double bpm) noexcept;
    [[nodiscard]] bool setStep(std::uint32_t step, bool enabled) noexcept;
    [[nodiscard]] bool setTrack(std::uint32_t pattern, std::uint32_t track, std::uint32_t mask, std::uint32_t accents, float level, bool muted) noexcept;
    [[nodiscard]] bool setTrackData(std::uint32_t pattern, std::uint32_t track, const PrototypeSequencer::Track& data) noexcept;
    [[nodiscard]] bool setSound(std::uint32_t pattern, std::uint32_t track, SoundSettings sound) noexcept;
    [[nodiscard]] bool setNote(std::uint32_t pattern, std::uint32_t track, std::uint32_t step, std::uint32_t note, bool flag) noexcept;
    [[nodiscard]] bool selectPattern(std::uint32_t pattern) noexcept;
    [[nodiscard]] SequenceState sequenceState() noexcept;
    void initializeWhenStopped(double sampleRate, std::uint32_t maxCallbackFrames) noexcept;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream,
                                           void* audioData,
                                           std::int32_t numFrames) override;

    void onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) override;

private:
    [[nodiscard]] bool openStreamLocked() noexcept;

    AudioCore& core_;
    ScheduledTriggerInput triggerInput_{core_};
    PrototypeSequencer sequencer_{core_};
    std::mutex streamMutex_;
    bool closing_ = false;
    std::shared_ptr<oboe::AudioStream> stream_;
    std::atomic<bool> shouldRun_{false};
    std::uint64_t callbackStartFrame_ = 0;
    std::atomic<bool> exitScheduler_{false};
    std::thread schedulerThread_;
};

}  // namespace original_sequencer::prototype::flutter_native

#endif
