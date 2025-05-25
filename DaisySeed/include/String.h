//
// Created by Levin on 17.05.25.
//

#ifndef STRING_H
#define STRING_H
#pragma once

#include <vector>
#include <cmath>

class StringWaveguide {
public:
    void Init(float fundamentalFreq, float excitationPos = 0.5f) {
        excitationPos_ = excitationPos;
        SetFrequency(fundamentalFreq);
    }

    float Process(float excitationInput) {
        float leftReadIdx  = writeIdx_ - delayLength_ * excitationPos_;
        float rightReadIdx = writeIdx_ - delayLength_ * (1.0f - excitationPos_);
        if (leftReadIdx < 0) leftReadIdx += bufferSize_;
        if (rightReadIdx < 0) rightReadIdx += bufferSize_;
        float tappedL = ReadDelayLine(leftDelay_, leftReadIdx);
        float tappedR = ReadDelayLine(rightDelay_, rightReadIdx);
        float dampedL = damping_ * tappedL;
        float dampedR = damping_ * tappedR;
        float apfOutL = dispersionCoeff_ * (dampedL - apfStateLeft_)
                        + apfStateLeft_;
        apfStateLeft_ = apfOutL;

        float apfOutR = dispersionCoeff_ * (dampedR - apfStateRight_)
                        + apfStateRight_;
        apfStateRight_ = apfOutR;
        float yL = apfOutL + excitationInput * 0.5f;
        float yR = apfOutR + excitationInput * 0.5f;
        WriteDelayLine(leftDelay_, yL);
        WriteDelayLine(rightDelay_, yR);
        writeIdx_ += 1.0f;
        if (writeIdx_ >= bufferSize_) writeIdx_ -= bufferSize_;
        float bridgeReadIdx = writeIdx_ - delayLength_ * 1.0f;
        if (bridgeReadIdx < 0) bridgeReadIdx += bufferSize_;
        float outLeft  = ReadDelayLine(rightDelay_, bridgeReadIdx);
        float outRight = ReadDelayLine(leftDelay_, bridgeReadIdx);
        float rawOut   = outLeft + outRight;
        float y        = rawOut - prevRaw_ + highpassCoeff_ * prevOut_;
        prevRaw_       = rawOut;
        prevOut_       = y;
        return y;
    }

    void SetFrequency(float frequency) {
        frequency_   = frequency;
        delayLength_ = SAMPLE_RATE / frequency_;
        bufferSize_  = static_cast<int>(std::ceil(delayLength_)) + 2;
        leftDelay_.assign(bufferSize_, 0.0f);
        rightDelay_.assign(bufferSize_, 0.0f);
        writeIdx_ = 0.0f;
    }

    void SetExcitationPosition(float pos) {
        excitationPos_ = std::fmax(0.0f, std::fmin(1.0f, pos));
    }

    void SetDamping(float damping) {
        damping_ = damping;
    }

    void SetDispersion(float coeff) {
        dispersionCoeff_ = std::fmax(-0.99f, std::fmin(0.99f, coeff));
    }

private:
    float frequency_;
    float delayLength_;
    float fracDelay_;
    float damping_;

    float excitationPos_; // 0.0 - 1.0

    float prevOut_       = 0.0f;
    float prevRaw_       = 0.0f;
    float highpassCoeff_ = 0.95f;

    float dispersionCoeff_ = 0.0f;
    float apfStateLeft_    = 0.0f;
    float apfStateRight_   = 0.0f;

    std::vector<float> leftDelay_;
    std::vector<float> rightDelay_;
    int                bufferSize_;
    float              writeIdx_;

    float ReadDelayLine(const std::vector<float> &delay, float readIdx) {
        int   idxA = static_cast<int>(readIdx) % bufferSize_;
        int   idxB = (idxA + 1) % bufferSize_;
        float frac = readIdx - static_cast<int>(readIdx);
        return delay[idxA] * (1.0f - frac) + delay[idxB] * frac;
    }


    void WriteDelayLine(std::vector<float> &delay, float value) {
        delay[static_cast<int>(writeIdx_) % bufferSize_] = value;
    }
};

static void InitSympatheticStrings(StringWaveguide strings[],
                                   int             numStrings,
                                   float           baseFrequency,
                                   float           excitationPos = 1.0f,
                                   float           damping       = 0.995f,
                                   float           dispersion    = 0.4f) {
    for (int i = 0; i < numStrings; ++i) {
        float freq = baseFrequency * std::pow(2.0f, i / 12.0f); // i semitones up
        strings[i].Init(freq, excitationPos);
        strings[i].SetDamping(damping);
        strings[i].SetDispersion(dispersion);
    }
}

#endif //STRING_H
