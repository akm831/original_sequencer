#include "prototype_bridge.h"

#include "original_sequencer/prototype/AudioCore.h"

using original_sequencer::prototype::AudioCore;

extern "C" {

void* prototype_create(void) {
    return new AudioCore{};
}

void prototype_destroy(void* handle) {
    delete static_cast<AudioCore*>(handle);
}

void prototype_initialize(void* handle, double sample_rate, uint32_t max_callback_frames) {
    if (handle == nullptr) {
        return;
    }
    static_cast<AudioCore*>(handle)->initialize(sample_rate, max_callback_frames);
}

PrototypeDiagnostics prototype_get_diagnostics(void* handle) {
    if (handle == nullptr) {
        return PrototypeDiagnostics{};
    }
    const auto snapshot = static_cast<AudioCore*>(handle)->diagnostics();
    return PrototypeDiagnostics{
        snapshot.sampleRate,
        snapshot.callbackFrames,
        snapshot.renderedFrames,
        snapshot.audioRestartCount,
    };
}

}
