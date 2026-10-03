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
    uint32_t callback_frames_min;
    uint32_t callback_frames_max;
    uint64_t rendered_frames;
    uint64_t callback_start_frame;
    double callback_duration_us;
    double callback_load;
    double callback_load_p95;
    double callback_load_p99;
    double callback_load_peak;
    uint32_t audio_restart_count;
    uint32_t queue_depth;
    uint32_t queue_high_water_mark;
    uint64_t queue_overflow_count;
    uint64_t trigger_count;
    uint32_t last_trigger_offset;
} PrototypeDiagnostics;
typedef struct PrototypeSequenceState {
    double bpm;
    uint32_t running;
    uint32_t step_mask;
    uint32_t current_step;
    uint64_t missed_steps;
    uint32_t current_pattern;
    uint32_t queued_pattern;
} PrototypeSequenceState;
PROTOTYPE_FFI_EXPORT int32_t prototype_set_track(void* handle, uint32_t pattern, uint32_t track, uint32_t mask, uint32_t accents, float level, int32_t muted);
PROTOTYPE_FFI_EXPORT int32_t prototype_set_sound(void* handle, uint32_t pattern, uint32_t track, float pitch, float decay, float tone, float cutoff, float resonance, float envelope, uint32_t waveform);
int32_t prototype_set_note(void* handle, uint32_t pattern, uint32_t track, uint32_t step, uint32_t note, int32_t flag);
int32_t prototype_select_pattern(void* handle, uint32_t pattern);
PROTOTYPE_FFI_EXPORT int32_t prototype_set_playing(void* handle, int32_t playing);
PROTOTYPE_FFI_EXPORT int32_t prototype_set_bpm(void* handle, double bpm);
PROTOTYPE_FFI_EXPORT int32_t prototype_set_step(void* handle, uint32_t step, int32_t enabled);
PROTOTYPE_FFI_EXPORT PrototypeSequenceState prototype_get_sequence_state(void* handle);
PROTOTYPE_FFI_EXPORT void* prototype_create(void);
PROTOTYPE_FFI_EXPORT void prototype_destroy(void* handle);
PROTOTYPE_FFI_EXPORT void prototype_initialize(void* handle, double sample_rate, uint32_t max_callback_frames);
PROTOTYPE_FFI_EXPORT int32_t prototype_start_audio(void* handle);
PROTOTYPE_FFI_EXPORT void prototype_stop_audio(void* handle);
// Scheduled P3 test input; rejects queue overflow, invalid value, stopped audio,
// and timestamps earlier than the last accepted command. Not a Live Pad API.
PROTOTYPE_FFI_EXPORT int32_t prototype_schedule_trigger(void* handle, uint32_t delay_frames, float value);
#if !defined(__ANDROID__)
// Headless host test path. Caller owns lifecycle and must not render concurrently.
PROTOTYPE_FFI_EXPORT void prototype_render_for_test(void* handle, float* output, uint32_t frames, uint32_t channels);
#endif
PROTOTYPE_FFI_EXPORT PrototypeDiagnostics prototype_get_diagnostics(void* handle);
#ifdef __cplusplus
}
#endif
