#include "prototype_bridge.h"

#include <cassert>
#include <array>
#include <limits>
#include <thread>

int main() {
    const auto empty = prototype_get_diagnostics(nullptr);
    assert(empty.sample_rate == 0.0);
    assert(empty.callback_frames == 0);
    assert(empty.rendered_frames == 0);
    assert(empty.audio_restart_count == 0);

    void* handle = prototype_create();
    assert(handle != nullptr);

    prototype_initialize(handle, 48000.0, 512);
    const auto initialized = prototype_get_diagnostics(handle);
    assert(initialized.sample_rate == 48000.0);
    assert(initialized.callback_frames == 0);
    assert(initialized.rendered_frames == 0);
    assert(initialized.audio_restart_count == 0);

    assert(prototype_schedule_trigger(nullptr, 0, 1.0F) == 0);
    assert(prototype_schedule_trigger(handle, 96001, 1.0F) == 0);
    assert(prototype_schedule_trigger(handle, 0, std::numeric_limits<float>::infinity()) == 0);
    assert(prototype_schedule_trigger(handle, 37, 1.0F) == 1);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 0); // FIFO time order
    assert(prototype_get_diagnostics(handle).queue_depth == 1);
    std::array<float, 192> output{};
    for (int i = 0; i < 3; ++i) {
        prototype_render_for_test(handle, output.data(), 96, 2);
        for (auto sample : output) assert(sample == 0.0F);
    }
    prototype_render_for_test(handle, output.data(), 96, 2);
    // Lead 256 + delay 37 = frame 293, fourth callback starts at 288.
    for (int i = 0; i < 10; ++i) assert(output[i] == 0.0F);
    assert(output[10] == 0.08F);
    const auto triggered = prototype_get_diagnostics(handle);
    assert(triggered.queue_depth == 0);
    assert(triggered.queue_high_water_mark == 1);
    assert(triggered.queue_overflow_count == 0);
    assert(triggered.trigger_count == 1);
    assert(triggered.last_trigger_offset == 5);
    for (int i = 0; i < 256; ++i) assert(prototype_schedule_trigger(handle, 0, 1.0F) == 1);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 0);
    assert(prototype_get_diagnostics(handle).queue_overflow_count == 1);

    prototype_initialize(handle, 44100.0, 256);
    const auto restarted = prototype_get_diagnostics(handle);
    assert(restarted.sample_rate == 44100.0);
    assert(restarted.audio_restart_count == 1);

    assert(restarted.queue_depth == 0);
    assert(restarted.trigger_count == 0);
    assert(restarted.queue_overflow_count == 0);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 1);
    prototype_stop_audio(handle);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 0);
    assert(prototype_get_diagnostics(handle).queue_depth == 0);
    prototype_render_for_test(handle, output.data(), 96, 2);
    for (auto sample : output) assert(sample == 0.0F);
    // Control-side lifecycle and submissions serialize; UI diagnostics may
    // observe resets concurrently but must stay within queue bounds.
    prototype_initialize(handle, 48000.0, 256);
    std::thread producer([&] {
        for (int i = 0; i < 10000; ++i) (void)prototype_schedule_trigger(handle, 0, 1.0F);
    });
    std::thread lifecycle([&] {
        for (int i = 0; i < 100; ++i) {
            prototype_stop_audio(handle);
            prototype_initialize(handle, 48000.0, 256);
        }
    });
    for (int i = 0; i < 10000; ++i)
        assert(prototype_get_diagnostics(handle).queue_depth <= 257);
    producer.join();
    lifecycle.join();
    prototype_destroy(handle);
    prototype_destroy(nullptr);
    return 0;
}
