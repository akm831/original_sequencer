#include "prototype_bridge.h"
#include "original_sequencer/prototype/AudioCore.h"
#include "original_sequencer/prototype/ScheduledTriggerInput.h"
#include "original_sequencer/prototype/PrototypeSequencer.h"
#include <mutex>
#if defined(__ANDROID__)
#include "android_audio_backend.h"
#endif

using original_sequencer::prototype::AudioCore;
namespace {
struct PrototypeEngine {
    AudioCore core;
#if defined(__ANDROID__)
    original_sequencer::prototype::flutter_native::AndroidAudioBackend audio{core};
#else
    std::mutex producerMutex;
    original_sequencer::prototype::ScheduledTriggerInput triggerInput{core};
    original_sequencer::prototype::PrototypeSequencer sequencer{core};
    std::uint64_t nextFrame = 0;
#endif
};
PrototypeEngine* asEngine(void* handle) noexcept { return static_cast<PrototypeEngine*>(handle); }
}

extern "C" {
void* prototype_create(void) { return new PrototypeEngine{}; }

// Handle destruction is owner-only: no concurrent bridge call is allowed.
void prototype_destroy(void* handle) {
    if (!handle) return;
#if defined(__ANDROID__)
    asEngine(handle)->audio.stop();
#endif
    delete asEngine(handle);
}

void prototype_initialize(void* handle, double sample_rate, uint32_t max_callback_frames) {
    if (!handle) return;
    auto& engine = *asEngine(handle);
#if defined(__ANDROID__)
    engine.audio.initializeWhenStopped(sample_rate, max_callback_frames);
#else
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    engine.core.initialize(sample_rate, max_callback_frames);
    engine.sequencer.deviceReset();
    engine.triggerInput.reset();
    engine.nextFrame = 0;
#endif
}

int32_t prototype_start_audio(void* handle) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.start() ? 1 : 0;
#else
    return 0; // No real audio device in host tests.
#endif
}

void prototype_stop_audio(void* handle) {
    if (!handle) return;
#if defined(__ANDROID__)
    asEngine(handle)->audio.stop();
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    engine.core.shutdown();
    engine.sequencer.deviceReset();
    engine.triggerInput.reset();
    engine.nextFrame = 0;
#endif
}

int32_t prototype_schedule_trigger(void* handle, uint32_t delay_frames, float value) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.scheduleTrigger(delay_frames, value) ? 1 : 0;
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    if (engine.sequencer.state().running) return 0;
    return engine.triggerInput.submit(delay_frames, value) ? 1 : 0;
#endif
}

int32_t prototype_set_track(void* handle, uint32_t pattern, uint32_t track, uint32_t mask, uint32_t accents, float level, int32_t muted) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.setTrack(pattern, track, mask, accents, level, muted != 0) ? 1 : 0;
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    return engine.sequencer.setTrack(pattern, track, mask, accents, level, muted != 0) ? 1 : 0;
#endif
}
int32_t prototype_select_pattern(void* handle, uint32_t pattern) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.selectPattern(pattern) ? 1 : 0;
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    return engine.sequencer.selectPattern(pattern) ? 1 : 0;
#endif
}

int32_t prototype_set_playing(void* handle, int32_t playing) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.setPlaying(playing != 0) ? 1 : 0;
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    if (!playing) { engine.sequencer.stop(); return 1; }
    return engine.sequencer.play() ? 1 : 0;
#endif
}

int32_t prototype_set_bpm(void* handle, double bpm) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.setBpm(bpm) ? 1 : 0;
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    return engine.sequencer.setBpm(bpm) ? 1 : 0;
#endif
}

int32_t prototype_set_step(void* handle, uint32_t step, int32_t enabled) {
    if (!handle) return 0;
#if defined(__ANDROID__)
    return asEngine(handle)->audio.setStep(step, enabled != 0) ? 1 : 0;
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    return engine.sequencer.setStep(step, enabled != 0) ? 1 : 0;
#endif
}

PrototypeSequenceState prototype_get_sequence_state(void* handle) {
    if (!handle) return {120.0, 0, 0, 16, 0, 0, 4};
#if defined(__ANDROID__)
    const auto s = asEngine(handle)->audio.sequenceState();
#else
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    const auto s = engine.sequencer.state();
#endif
    return {s.bpm, s.running, s.stepMask, s.currentStep, s.missedSteps, s.currentPattern, s.queuedPattern};
}

#if !defined(__ANDROID__)
void prototype_render_for_test(void* handle, float* output, uint32_t frames, uint32_t channels) {
    if (!handle) return;
    auto& engine = *asEngine(handle);
    std::lock_guard<std::mutex> lock(engine.producerMutex);
    engine.sequencer.advance();
    engine.core.render(output, frames, channels, engine.nextFrame);
    engine.nextFrame += frames;
}
#endif

PrototypeDiagnostics prototype_get_diagnostics(void* handle) {
    if (!handle) return PrototypeDiagnostics{};
    const auto s = asEngine(handle)->core.diagnostics();
    return PrototypeDiagnostics{s.sampleRate, s.callbackFrames, s.callbackFramesMin,
        s.callbackFramesMax, s.renderedFrames, s.callbackStartFrame, s.callbackDurationUs,
        s.callbackLoad, s.callbackLoadP95, s.callbackLoadP99, s.callbackLoadPeak,
        s.audioRestartCount, s.queueDepth, s.queueHighWaterMark, s.queueOverflowCount,
        s.triggerCount, s.lastTriggerOffset};
}
}
