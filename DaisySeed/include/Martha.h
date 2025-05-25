#ifndef MARTHA_H
#define MARTHA_H

#pragma once

#include <vector>
#include "daisysp.h"
#include "Sigmund.h"

#define MAX_VOICES 14

using namespace daisysp;

struct RangedParam {
    float start, end;
};

struct Params {
    float       ampThreshold      = 0.6;
    RangedParam attackRangeMs     = {0.001, 0.2};
    RangedParam releaseRangeMs    = {0.01, 0.2};
    RangedParam tremRangeHz       = {0.25, 40};
    RangedParam glissRangeSemitones      = {-50.f, 10};
    RangedParam glissOnsetDelayMs = {50, 750};
    RangedParam glissDurationMs   = {500, 2000};

    static float getRandValue(RangedParam param) {
        return Random::GetFloat(param.start, param.end);
    }
};

struct Voice {
    bool       active  = false;
    int        trackId = -1;
    Oscillator osc;
    Oscillator tremLfo;
    Adsr       env;
    float      amp                   = 0.0f;
    bool       gate                  = false;
    int        glissandoDelayCounter = 0;
    float      glissandoIncrement    = 0.f;
    int        glissandoEnvCounter   = 0;
    int        glissandoOnsetDelay   = 0;
    int        glissandoDuration     = 0;
    Params     params;
    float      sampleRate;
    float      freq;

    void init(const float sample_rate, const Params &params) {
        this->params = params;
        sampleRate   = sample_rate;
        osc.Init(sample_rate);
        osc.SetWaveform(Oscillator::WAVE_SIN);
        env.Init(sample_rate);
    }

    void noteOn(int id, float freq, float velocity) {
        trackId = id;
        amp     = velocity;
        osc.SetFreq(freq);
        this->freq          = freq * 2.0;
        active              = true;
        gate                = true;
        const float attack  = Params::getRandValue(params.attackRangeMs) * 1e-3f;
        const float release = Params::getRandValue(params.releaseRangeMs) * 1e-3f;
        env.SetTime(ADSR_SEG_ATTACK, attack);
        env.SetTime(ADSR_SEG_DECAY, 0.100f);
        env.SetSustainLevel(0.6f);
        env.SetTime(ADSR_SEG_RELEASE, release);
        tremLfo.Init(sampleRate);
        const float lfoFreq = Params::getRandValue(params.tremRangeHz);
        tremLfo.SetFreq(lfoFreq);
        tremLfo.SetWaveform(Oscillator::WAVE_SIN);
        tremLfo.SetAmp(1.0);
        glissandoDelayCounter = 0;
        glissandoEnvCounter   = 0;
        glissandoOnsetDelay   = 0;
        float durSec          = Params::getRandValue(params.glissDurationMs) * 1e-3f;
        glissandoDuration     = (int) (durSec * sampleRate);
        float delaySec        = Params::getRandValue(params.glissOnsetDelayMs) * 1e-3f;
        glissandoOnsetDelay   = (int) (delaySec * sampleRate);
        if (glissandoDuration != 0.0f) {
            float semi   = Params::getRandValue(params.glissRangeSemitones);
            float target = freq * std::pow(2.f, semi/12.f);
            float diff   = target - freq;
            glissandoIncrement = diff / float(glissandoDuration);
        }
    }

    void noteOff(int id) {
        if (active && trackId == id)
            gate = false;
    }

    float process() {
        if (!active)
            return 0.0f;

        if (glissandoDelayCounter < glissandoOnsetDelay) {
            glissandoDelayCounter++;
        } else if (glissandoEnvCounter < glissandoDuration) {
            glissandoEnvCounter++;
            osc.SetFreq(freq + glissandoIncrement * glissandoEnvCounter);
        }

        float out = osc.Process() * env.Process(gate) * amp * tremLfo.Process();
        if (!env.IsRunning())
            active = false;
        return out / MAX_VOICES;
    }
};

class Martha {
public:
    explicit Martha() {
    }

    void init(float sampleRate) {
        voices_.resize(MAX_VOICES);
        for (auto &v: voices_)
            v.init(sampleRate, params_);
    }

    void processTracks(const std::vector<Track> &tracks) {
        for (auto &t: tracks) {
            int   id       = t.id;
            float f        = t.freq, a = t.amp;
            int   voiceIdx = -1;
            for (int i = 0; i < voices_.size(); ++i) {
                Voice &v = voices_[i];
                if (v.trackId == id) {
                    voiceIdx = i;
                    break;
                }
            }

            if (voiceIdx == -1 && a > params_.ampThreshold) {
                int vi = allocateVoice();
                if (vi >= 0) voices_[vi].noteOn(id, f, a);
            }
        }

        for (auto &v: voices_) {
            if (v.active) {
                bool stillThere = false;
                for (auto &t: tracks)
                    if (t.id == v.trackId) {
                        stillThere = true;
                        break;
                    }

                if (!stillThere)
                    v.noteOff(v.trackId);
            }
        }
    }

    float render() {
        float mix = 0.0f;
        for (auto &v: voices_)
            mix += v.process();
        return mix;
    }

private:
    int allocateVoice() const {
        for (int i = 0; i < MAX_VOICES; ++i)
            if (!voices_[i].active)
                return i;
        return -1;
    }
    
    std::vector<Voice> voices_;
    Params             params_ = {
        0.6f,
        {2, 100},
        {400, 900},
        {0.25, 100},
        {0, 0},
        {200, 500},
        {1000, 1500}
    };
};

#endif // MARTHA_H
