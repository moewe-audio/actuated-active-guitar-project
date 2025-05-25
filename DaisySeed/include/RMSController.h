//
// Created by Levin on 26.04.25.
//

#ifndef RMSCONTROLLER_H
#define RMSCONTROLLER_H

#include <cmath>

class RmsTrackerController {
public:
    RmsTrackerController(float targetRms, float smoothingTimeMs, float adaptationRate,
                         float initialGain = 0.0f, float minGain = 0.0f, float maxGain = 1.0f, float sampleRate = 48000.0f)
    : targetRms_(targetRms), gain_(initialGain), minGain_(minGain), maxGain_(maxGain)
    {
        float smoothingTimeSec = smoothingTimeMs * 0.001f;
        alpha_ = expf(-1.0f / (smoothingTimeSec * sampleRate));
        oneMinusAlpha_ = 1.0f - alpha_;
        smoothedPower_ = 0.0f;
        Ki_ = adaptationRate;
    }

    float processSample(float inputSample) {
        float samplePower = inputSample * inputSample;
        smoothedPower_ = alpha_ * smoothedPower_ + oneMinusAlpha_ * samplePower;
        float currentRms = sqrtf(smoothedPower_);
        float error = targetRms_ - currentRms;
        gain_ += Ki_ * error;
        if (gain_ > maxGain_) gain_ = maxGain_;
        if (gain_ < minGain_) gain_ = minGain_;
        return gain_;
    }

    void setTargetRms(float newTarget) {
        targetRms_ = newTarget;
    }

    void reset() {
        smoothedPower_ = 0.0f;
        gain_ = 0.0f;
    }

private:
    float targetRms_;
    float gain_;
    float minGain_, maxGain_;
    float alpha_, oneMinusAlpha_;
    float smoothedPower_;
    float Ki_;
};

#endif //RMSCONTROLLER_H
