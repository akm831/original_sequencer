#include "prototype_bridge.h"

#include "original_sequencer/prototype/AudioCore.h"

#if defined(__ANDROID__)
#include "android_audio_backend.h"
#endif

using original_sequencer::prototype::AudioCore;

namespace {

struct PrototypeEngine {
    AudioCore core;
#if defined(__ANDROID__)
    original_sequencer::prototype::flutter_native::AndroidAudioBackend audio{core};
#endif
};

PrototypeEngine* asEngine(void* handle) noexcept {
    return static_cast<PrototypeEngine*>(handle);
}

}  // namespace

extern "C" {

void* prototype_create(void) {
    return new PrototypeEngine{};
}

void prototype_destroy(void* handle) {
    if (handle == nullptr) {
        return;
    }
#if defined(__ANDROID__)
    asEngine(handle)->audio.stop();
#endif
    delete asEngine(handle);
}

void prototype_initialize(void* handle, double sample_rate, uint32_t max_callback_frames) {
    if (handle == nullptr) {
        return;
    }
    asEngine(handle)->core.initialize(sample_rate, max_callback_frames);
}

int32_t prototype_start_audio(void* handle) {
    if (handle == nullptr) {
        return 0;
    }
#if defined(__ANDROID__)
    return asEngine(handle)->audio.start() ? 1 : 0;
#else
    return 0;
#endif
}

void prototype_stop_audio(void* handle) {
    if (handle == nullptr) {
        return;
    }
#if defined(__ANDROID__)
    asEngine(handle)->audio.stop();
#endif
}

PrototypeDiagnostics prototype_get_diagnostics(void* handle) {
    if (handle == nullptr) {
        return PrototypeDiagnostics{};
    }
    const auto snapshot = asEngine(handle)->core.diagnostics();
    return PrototypeDiagnostics{
        snapshot.sampleRate,
        snapshot.callbackFrames,
        snapshot.renderedFrames,
        snapshot.audioRestartCount,
    };
}

}
