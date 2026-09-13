#include "prototype_bridge.h"

#include <cassert>

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

    prototype_initialize(handle, 44100.0, 256);
    const auto restarted = prototype_get_diagnostics(handle);
    assert(restarted.sample_rate == 44100.0);
    assert(restarted.audio_restart_count == 1);

    prototype_destroy(handle);
    prototype_destroy(nullptr);
    return 0;
}
