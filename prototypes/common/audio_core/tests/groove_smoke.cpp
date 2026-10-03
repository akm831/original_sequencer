#include "original_sequencer/prototype/PrototypeSequencer.h"
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
using namespace original_sequencer::prototype;
int main() {
    // Independent voices sum without replacing one another, with identical stereo output.
    std::array<std::array<float, 2048>, 5> wave{};
    for (unsigned voice = 1; voice <= 5; ++voice) {
        AudioCore core; core.initialize(48000, 2048);
        for (unsigned v = 1; v <= 4; ++v)
            if (voice == v || voice == 5) assert(core.enqueueCommand({AudioCommandType::trigger, 37, v, .7F}));
        core.render(wave[voice-1].data(), 1024, 2, 0);
        for (unsigned i = 0; i < 2048; i += 2) {
            assert(wave[voice-1][i] == wave[voice-1][i+1]);
            assert(std::isfinite(wave[voice-1][i]) && std::abs(wave[voice-1][i]) <= .8F);
            if (i < 74) assert(wave[voice-1][i] == 0);
        }
    }
    for (unsigned i = 0; i < 2048; ++i) {
        float sum = 0; for (unsigned v = 0; v < 4; ++v) sum += wave[v][i];
        assert(std::abs(sum - wave[4][i]) < 1e-6F);
    }
    for (unsigned v = 0; v < 4; ++v) {
        bool audible = false; for (float x : wave[v]) audible |= x != 0;
        assert(audible);
        if (v > 0) assert(wave[v] != wave[v-1]);
    }
    AudioCore core; core.initialize(48000, 512); PrototypeSequencer seq(core);
    assert(!seq.setTrack(4,0,1,0,1,false));
    assert(!seq.setTrack(0,4,1,0,1,false));
    assert(!seq.setTrack(0,0,65536,0,1,false));
    assert(!seq.setTrack(0,0,1,0,std::numeric_limits<float>::quiet_NaN(),false));
    assert(!seq.selectPattern(4));
    assert(seq.setTrack(0,0,1,1,1,false));
    assert(seq.setTrack(0,1,65535,0,1,true)); // mute suppresses all sixteen events
    for (unsigned t = 0; t < 4; ++t) assert(seq.setTrack(1,t,1,1,.8F,false));
    assert(seq.play());
    std::array<float,96> out{};
    for (std::uint64_t frame=0; frame<192000; frame+=96) {
        if (frame == 960) { assert(seq.selectPattern(1)); assert(seq.state().queuedPattern == 1); }
        seq.advance(); core.render(out.data(),96,1,frame);
        auto state = seq.state();
        if (frame+96 <= 96256) assert(state.currentPattern == 0);
        if (frame >= 96384) { assert(state.currentPattern == 1); assert(state.queuedPattern == 4); }
    }
    assert(core.diagnostics().triggerCount == 5); // one kick then four voices at next bar
    assert(seq.state().missedSteps == 0 && core.diagnostics().queueOverflowCount == 0);
    assert(seq.selectPattern(2)); seq.stop();
    assert(seq.state().currentPattern == 1 && seq.state().queuedPattern == 4);
    auto count = core.diagnostics().triggerCount;
    for (std::uint64_t frame=192000; frame<207000; frame+=96) core.render(out.data(),96,1,frame);
    assert(core.diagnostics().triggerCount == count);
    for (float x : out) assert(x == 0);
    assert(seq.play());
    for (std::uint64_t frame=207072; frame<208000; frame+=96) { seq.advance(); core.render(out.data(),96,1,frame); }
    assert(seq.state().currentPattern == 1);
}
