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
    prototype_initialize(handle, 48000.0, 256);
    assert(prototype_set_bpm(handle, 59.0) == 0);
    assert(prototype_set_bpm(handle, 240.0) == 1);
    assert(prototype_set_step(handle, 16, 1) == 0);
    assert(prototype_set_step(handle, 1, 1) == 1);
    assert(prototype_set_playing(handle, 1) == 1);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 0);
    for (int i = 0; i < 500; ++i)
        prototype_render_for_test(handle, output.data(), 96, 2);
    const auto sequence = prototype_get_sequence_state(handle);
    assert(sequence.running == 1 && sequence.bpm == 240.0);
    assert(sequence.step_mask == 0x1113 && sequence.current_step < 16);
    assert(sequence.missed_steps == 0);
    assert(prototype_get_diagnostics(handle).trigger_count > 4);
    assert(prototype_set_playing(handle, 0) == 1);
    const auto stoppedCount = prototype_get_diagnostics(handle).trigger_count;
    for (int i = 0; i < 100; ++i)
        prototype_render_for_test(handle, output.data(), 96, 2);
    assert(prototype_get_diagnostics(handle).trigger_count == stoppedCount);
    assert(prototype_get_sequence_state(handle).current_step == 16);
    for (auto sample : output) assert(sample == 0.0F);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 1);
    assert(prototype_set_playing(handle, 1) == 1);
    assert(prototype_set_playing(handle, 0) == 1);
    assert(prototype_schedule_trigger(handle, 0, 1.0F) == 1);
    prototype_stop_audio(handle);
    assert(prototype_set_playing(handle, 1) == 0);
    assert(prototype_get_sequence_state(handle).step_mask == 0x1113);
    prototype_initialize(handle, 48000, 256);
    assert(prototype_set_track(nullptr,0,0,1,0,1,0) == 0);
    assert(prototype_set_track(handle,4,0,1,0,1,0) == 0);
    assert(prototype_select_pattern(handle,4) == 0);
    assert(prototype_set_track(handle,2,0,1,1,.8F,0) == 1);
    assert(prototype_set_sound(nullptr,2,0,.5F,.5F,.5F,.5F,.5F,.5F,0) == 0);
    assert(prototype_set_sound(handle,2,0,.5F,.5F,.5F,.5F,2.F,.5F,0) == 0);
    assert(prototype_set_sound(handle,2,0,.5F,.5F,.5F,.5F,.5F,.5F,0) == 1);
    assert(prototype_set_note(handle,2,3,16,36,0) == 0);
    assert(prototype_set_note(handle,2,3,0,85,0) == 0);
    assert(prototype_set_note(handle,2,3,0,48,1) == 1);
    assert(prototype_select_pattern(handle,2) == 1);
    assert(prototype_get_sequence_state(handle).current_pattern == 2);
    assert(prototype_set_playing(handle,1) == 1);
    for (int i=0; i<20; ++i) prototype_render_for_test(handle,output.data(),96,2);
    assert(prototype_get_sequence_state(handle).current_pattern == 2);
    assert(prototype_get_diagnostics(handle).trigger_count == 1);
    prototype_initialize(handle,48000,256);
    PrototypeTrackConfig config{};
    config.mask=1; config.accents=1; config.level=.7F;
    config.pitch=.5F; config.decay=.5F; config.tone=.5F; config.cutoff=.4F;
    config.resonance=.7F; config.envelope=.6F; config.flags=1;
    for(auto& note:config.notes) note=36;
    static_assert(sizeof(PrototypeTrackConfig)==112);
    assert(prototype_update_track(handle,3,3,nullptr)==0);
    assert(prototype_update_track(handle,3,3,&config)==1);
    config.notes[0]=85; config.mask=65535;
    assert(prototype_update_track(handle,3,3,&config)==0); // invalid update is all-or-nothing
    assert(prototype_select_pattern(handle,3)==1);
    assert(prototype_set_playing(handle,1)==1);
    for(int i=0;i<200;i++) prototype_render_for_test(handle,output.data(),96,2);
    assert(prototype_get_diagnostics(handle).trigger_count==1);
    assert(prototype_set_playing(handle,0)==1);
    for(int i=0;i<12;i++) prototype_render_for_test(handle,output.data(),96,2);
    for(float sample:output) assert(sample==0); // new voices fade out within 20 ms
    prototype_destroy(handle);
    prototype_destroy(nullptr);
    return 0;
}
