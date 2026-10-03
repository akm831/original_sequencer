#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace original_sequencer::prototype {
// Normalized, versioned engine state. No device or UI dependency.
struct SoundSettings {
    float pitch = .5F, decay = .35F, tone = .5F;
    float cutoff = .35F, resonance = .55F, envelope = .5F;
    std::uint32_t waveform = 0;
    bool valid() const noexcept {
        for (float x : {pitch,decay,tone,cutoff,resonance,envelope})
            if (!std::isfinite(x) || x < 0 || x > 1) return false;
        return waveform <= 1;
    }
};

// Character synthesis, not a circuit model of any hardware instrument.
// All memory is fixed; base coefficients are prepared at event boundaries.
class AnalogVoice {
public:
    void trigger(unsigned kind, const SoundSettings& s, float velocity, unsigned note,
                 bool accent, bool slide, bool openHat, unsigned gateFrames,
                 double sampleRate, std::uint64_t generation) noexcept {
        const bool legato = kind == 3 && slide && remaining_ > 0 && generation == generation_;
        if (!legato) {
            phase_ = 0; filter_ = {}; hatPhases_ = {}; hp_ = 0;
            amp_ = 0; env_ = 1; frequency_ = 440 * std::pow(2.0, (static_cast<int>(note)-69)/12.0);
            age_ = 0;
        }
        kind_ = kind; settings_ = s; generation_ = generation;
        rate_ = sampleRate; velocity_ = velocity; accent_ = accent;
        target_ = 440 * std::pow(2.0, (static_cast<int>(note)-69)/12.0);
        glide_ = legato ? std::exp(-1.0/(.065*rate_)) : 0;
        const double duration = kind == 0 ? .08 + s.decay*.9 :
            kind == 1 ? .06 + s.decay*.3 : kind == 2 ? (openHat ? .18+s.decay*.5 : .025+s.decay*.11) : .02;
        remaining_ = static_cast<unsigned>(duration * rate_) + (kind == 3 ? gateFrames : 0);
        gate_ = kind == 3 ? gateFrames : 0;
        drumDecay_ = std::exp(-6.0 / std::max(1.0, duration*rate_));
        envDecay_ = std::exp(-1.0 / ((.05+s.decay*.7)*rate_));
        baseCutoff_ = 70*std::pow(80.0,s.cutoff);
        attack_ = 1-std::exp(-1.0/(.002*rate_));
        release_ = std::exp(-1.0/(.006*rate_));
        if (!legato) noise_ = 0x9e3779b9U + kind;
    }
    float sample() noexcept {
        if (remaining_ == 0) return 0;
        --remaining_; ++age_;
        const double tau = 2*std::numbers::pi_v<double>;
        noise_ ^= noise_<<13; noise_ ^= noise_>>17; noise_ ^= noise_<<5;
        const double noise = static_cast<double>(noise_)/2147483648.0-1;
        double out = 0;
        if (kind_ == 3) {
            frequency_ = target_ + glide_*(frequency_-target_);
            const double dt = std::min(.2,frequency_/rate_);
            phase_ += dt; if (phase_ >= 1) phase_ -= 1;
            double osc = 2*phase_-1-blep(phase_,dt);
            if (settings_.waveform) {
                osc = (phase_<.5 ? 1.0 : -1.0)+blep(phase_,dt)-blep(std::fmod(phase_+.5,1.0),dt);
            }
            env_ *= envDecay_;
            if (gate_ > 0) { --gate_; amp_ += attack_*(1-amp_); }
            else amp_ *= release_;
            // Saturated feedback, three low-pass poles, two integration substeps.
            // Cutoff modulation in octaves; accent increases brightness and level.
            const double cutoff = std::min(rate_*.12,baseCutoff_*(1+env_*settings_.envelope*(accent_ ? 16 : 10)));
            const double g = 1-std::exp(-tau*cutoff/(rate_*2));
            for (unsigned sub=0; sub<2; ++sub) {
                double input = soft(osc - settings_.resonance*3.6*filter_[2]);
                for (auto& pole : filter_) { pole += g*(input-pole); input = soft(pole); }
            }
            out = filter_[2]*amp_*(accent_ ? 1.3 : 1.0);
        } else {
            const double elapsed = age_/rate_;
            if (kind_ == 0) {
                const double freq = 35+settings_.pitch*40 + (65+settings_.tone*100)*std::exp(-elapsed*65);
                phase_ += freq/rate_; if (phase_>=1) phase_-=1;
                out = std::sin(tau*phase_) + noise*settings_.tone*.12*std::exp(-elapsed*400);
            } else if (kind_ == 1) {
                const double freq = 130+settings_.pitch*100;
                phase_ += freq/rate_; if (phase_>=1) phase_-=1;
                hp_ += .14*(noise-hp_);
                hatPhases_[0] += freq*1.47/rate_; if (hatPhases_[0]>=1) hatPhases_[0]-=1;
                const double body = (std::sin(tau*phase_)+.5*std::sin(tau*hatPhases_[0]))*.45;
                out = body*(1-settings_.tone*.7)+(noise-hp_)*(.25+settings_.tone*.85);
            } else {
                constexpr std::array<double,6> freqs{211,307,421,547,631,809};
                double metal = 0;
                for (unsigned i=0; i<6; ++i) {
                    hatPhases_[i] += freqs[i]*(2+settings_.pitch*2)/rate_;
                    if (hatPhases_[i]>=1) hatPhases_[i]-=1;
                    metal += hatPhases_[i]<.5 ? 1 : -1;
                }
                metal = metal/6 + noise*settings_.tone*.3;
                hp_ += (.06+settings_.tone*.2)*(metal-hp_);
                out = metal-hp_;
            }
            env_ *= drumDecay_;
            out *= env_;
        }
        return static_cast<float>(out*velocity_*(kind_==3 ? .32 : .22));
    }
    bool active() const noexcept { return remaining_ != 0; }
private:
    static double soft(double x) noexcept { return x/(1+std::abs(x)); }
    static double blep(double t,double dt) noexcept {
        if(t<dt) { t/=dt; return t+t-t*t-1; }
        if(t>1-dt) { t=(t-1)/dt; return t*t+t+t+1; }
        return 0;
    }
    SoundSettings settings_{};
    unsigned kind_=0,remaining_=0,gate_=0,age_=0;
    std::uint64_t generation_=0;
    std::uint32_t noise_=1;
    double rate_=48000,phase_=0,frequency_=110,target_=110,glide_=0,velocity_=0;
    double env_=0,amp_=0,drumDecay_=0,envDecay_=0,attack_=0,release_=0,baseCutoff_=0,hp_=0;
    bool accent_=false;
    std::array<double,3> filter_{};
    std::array<double,6> hatPhases_{};
};
}
