#include "original_sequencer/prototype/PrototypeSequencer.h"
#include <array>
#include <vector>
#include <cassert>
#include <cmath>
using namespace original_sequencer::prototype;
std::vector<float> phrase(unsigned mask,unsigned flags) {
    AudioCore core; core.initialize(48000,512); PrototypeSequencer seq(core);
    PrototypeSequencer::Track data; data.mask=mask;data.flags=flags;data.notes[0]=36;data.notes[1]=48;data.notes[2]=43;
    assert(seq.setTrackData(0,3,data)); assert(seq.play());
    std::vector<float> out(24000);
    for(unsigned frame=0;frame<out.size();frame+=96) { seq.advance(); core.render(out.data()+frame,96,1,frame); }
    return out;
}
int main() {
    assert(phrase(3,1)!=phrase(3,0)); // adjacent active notes slide
    assert(phrase(5,1)==phrase(5,0)); // an outgoing flag cannot slide over a rest
    constexpr std::array<unsigned,7> buffers{64,96,128,192,256,384,512};
    for(double rate : {44100.,48000.}) {
        AudioCore core; core.initialize(rate,512); PrototypeSequencer seq(core);
        PrototypeSequencer::Track data;
        for(unsigned p=0;p<4;++p) for(unsigned t=0;t<4;++t) {
            data.mask=65535;data.accents=0x5555;data.flags=t==3 ? 65535 : 0xaaaa;
            data.sound={1,1,1,1,1,1,p%2}; data.level=1;
            for(unsigned step=0;step<16;++step) data.notes[step]=24+(step*7)%61;
            assert(seq.setTrackData(p,t,data));
        }
        assert(seq.setBpm(240)); assert(seq.play());
        std::array<float,1024> out{};
        std::uint64_t frame=0,iteration=0,nextChange=0;
        while(frame<rate*300) { // five minutes of dense analog synthesis per sample rate
            if(frame>=nextChange) {
                assert(seq.selectPattern((iteration/7)%4));
                data.sound.cutoff=static_cast<float>((iteration%11)/10.);
                assert(seq.setTrackData((iteration/7)%4,3,data));
                nextChange=frame+static_cast<unsigned>(rate*.7);
            }
            seq.advance();
            const auto count=buffers[iteration++%buffers.size()];
            core.render(out.data(),count,2,frame);
            for(unsigned i=0;i<count*2;i+=2) {
                assert(std::isfinite(out[i]) && std::abs(out[i])<=.8F);
                assert(out[i]==out[i+1]);
            }
            frame+=count;
        }
        assert(seq.state().missedSteps==0 && core.diagnostics().queueOverflowCount==0);
        seq.stop();
        const auto triggers=core.diagnostics().triggerCount;
        for(unsigned i=0;i<32;++i) { core.render(out.data(),96,2,frame);frame+=96; }
        for(unsigned i=0;i<192;++i) assert(out[i]==0);
        assert(core.diagnostics().triggerCount==triggers);
        for(unsigned cycle=0;cycle<1000;++cycle) {
            assert(seq.play()); seq.stop();
            core.render(out.data(),96,2,frame); frame+=96;
        }
        assert(core.diagnostics().queueOverflowCount==0);
    }
    // Retrigger smoothing starts at the preceding sample rather than jumping to zero.
    AnalogVoice voice; SoundSettings sound;
    voice.trigger(0,sound,1,36,false,false,false,6000,48000,1);
    float previous=0;for(int i=0;i<500;++i) previous=voice.sample();
    voice.trigger(0,sound,1,36,false,false,false,6000,48000,1);
    assert(std::abs(voice.sample()-previous)<1e-6);
}
