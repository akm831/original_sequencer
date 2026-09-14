#pragma once

#include <stdint.h>

#if defined(_WIN32)
#define PROTOTYPE_FFI_EXPORT __declspec(dllexport)
#else
#define PROTOTYPE_FFI_EXPORT __attribute__((visibility("default"))) __attribute__((used))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PrototypeDiagnostics {
    double sample_rate;
    uint32_t callback_frames;
    uint64_t rendered_frames;
    uint32_t audio_restart_count;
} PrototypeDiagnostics;

PROTOTYPE_FFI_EXPORT void* prototype_create(void);
PROTOTYPE_FFI_EXPORT void prototype_destroy(void* handle);
PROTOTYPE_FFI_EXPORT void prototype_initialize(void* handle, double sample_rate, uint32_t max_callback_frames);
PROTOTYPE_FFI_EXPORT int32_t prototype_start_audio(void* handle);
PROTOTYPE_FFI_EXPORT void prototype_stop_audio(void* handle);
PROTOTYPE_FFI_EXPORT PrototypeDiagnostics prototype_get_diagnostics(void* handle);

#ifdef __cplusplus
}
#endif
