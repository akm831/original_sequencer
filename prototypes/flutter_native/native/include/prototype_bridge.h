#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PrototypeDiagnostics {
    double sample_rate;
    uint32_t callback_frames;
    uint64_t rendered_frames;
    uint32_t audio_restart_count;
} PrototypeDiagnostics;

void* prototype_create(void);
void prototype_destroy(void* handle);
void prototype_initialize(void* handle, double sample_rate, uint32_t max_callback_frames);
PrototypeDiagnostics prototype_get_diagnostics(void* handle);

#ifdef __cplusplus
}
#endif
