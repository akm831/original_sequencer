#include "original_sequencer/prototype/AudioCore.h"
#include <vector>
#include <array>
#include <cassert>
#include <cmath>
using namespace original_sequencer::prototype;
std::vector<float> render(AudioCommand command, double rate, unsigned block) {
    AudioCore core; core.initialize(rate,512); assert(core.enqueueCommand(command));
    std::vector<float> result(96000);
    for(unsigned frame=0;frame<result.size();frame+=block)
        core.render(result.data()+frame,std::min(block,static_cast<unsigned>(result.size()-frame)),1,frame);
    return result;
}
double energy(const std::vector<float>& v) { double e=0; for(float x:v) e+=x*x; return e; }
int crossings(const std::vector<float>& v) {
    int count=0; for(unsigned i=2000;i<18000;i++) if(v[i-1]<=0 && v[i]>0) ++count; return count;
}
int main() {
    AudioCommand command{AudioCommandType::trigger,37,4,.8F};
    command.note=57; command.gateFrames=24000; command.sound.cutoff=1;
    command.sound.resonance=0; command.sound.envelope=0;
    const auto low=render(command,48000,96);
    command.note=69; const auto high=render(command,48000,257);
    assert(std::abs(crossings(high)-2*crossings(low))<=2);
    command.note=57; assert(render(command,48000,257)==low); // chunk-independent oscillator/filter state
    command.sound.waveform=1; assert(render(command,48000,96)!=low);
    command.sound.waveform=0; command.accent=true;
    assert(energy(render(command,48000,96))>energy(low));
    command.accent=false; command.sound.cutoff=0;
    assert(energy(render(command,48000,96))<energy(low));
    // Open and closed hat share a choke group. Closed articulation shortens the tail.
    command.voice=3; command.sound=SoundSettings{};
    auto closed=render(command,48000,96); command.openHat=true;
    auto open=render(command,48000,96); assert(energy(open)>energy(closed));
    AudioCore choke; choke.initialize(48000,512);
    command.targetFrame=0; assert(choke.enqueueCommand(command));
    command.targetFrame=4800; command.openHat=false; assert(choke.enqueueCommand(command));
    std::array<float,96> output{};
    for(unsigned f=0;f<14000;f+=96) choke.render(output.data(),96,1,f);
    for(float x:output) assert(x==0);
    // Extreme settings must stay finite and bounded at supported reference rates.
    for(double rate : {44100.0,48000.0}) for(unsigned voice=1;voice<=4;++voice)
      for(unsigned wave=0;wave<2;++wave) for(float extreme : {0.F,1.F}) {
        command={AudioCommandType::trigger,37,voice,1.F};
        command.note=wave ? 84 : 24; command.gateFrames=12000; command.openHat=true;
        command.sound={extreme,extreme,extreme,extreme,extreme,extreme,wave};
        auto result=render(command,rate,257);
        for(unsigned i=0;i<result.size();++i) {
            assert(std::isfinite(result[i]) && std::abs(result[i])<=.8F);
            if(i<37 || i>60000) assert(result[i]==0);
        }
    }
    // A connected slide preserves the envelope/filter, a normal note retriggers it.
    AnalogVoice slide,retrigger;
    SoundSettings settings;
    slide.trigger(3,settings,.7,36,false,false,false,7000,48000,1);
    retrigger.trigger(3,settings,.7,36,false,false,false,7000,48000,1);
    for(int i=0;i<6000;++i) assert(slide.sample()==retrigger.sample());
    slide.trigger(3,settings,.7,48,false,true,false,4000,48000,1);
    retrigger.trigger(3,settings,.7,48,false,false,false,4000,48000,1);
    double difference=0;
    for(int i=0;i<3000;++i) difference+=std::abs(slide.sample()-retrigger.sample());
    assert(difference>1);
    AudioCore invalid; invalid.initialize(48000,512);
    command.sound.resonance=2; assert(!invalid.enqueueCommand(command));
    command.sound=SoundSettings{}; command.note=85; assert(!invalid.enqueueCommand(command));
}
