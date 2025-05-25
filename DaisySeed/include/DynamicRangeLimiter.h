#ifndef DYNAMIC_RANGE_LIMITER_H
#define DYNAMIC_RANGE_LIMITER_H

#include <cmath>

class DynamicRangeLimiter {
public:
    DynamicRangeLimiter(float thresholdRms,
                        float attackTimeMs,
                        float releaseTimeMs,
                        float sampleRate)
      : threshold_(thresholdRms),
        env_(0.0f),
        gain_(1.0f)
    {
        float atkSec = attackTimeMs * 0.001f;
        float relSec = releaseTimeMs * 0.001f;
        attackCoeff_  = std::exp(-1.0f / (atkSec * sampleRate));
        releaseCoeff_ = std::exp(-1.0f / (relSec * sampleRate));
    }

    float processSample(float x) {
        float absx = std::fabs(x);

        if (absx > env_) {
            env_ = attackCoeff_  * env_ + (1.0f - attackCoeff_)  * absx;
        } else {
            env_ = releaseCoeff_ * env_ + (1.0f - releaseCoeff_) * absx;
        }

        if (env_ > threshold_ && env_ > 0.0f) {
            gain_ = threshold_ / env_;
        } else {
            gain_ = 1.0f;
        }

        return gain_ * x;
    }

    void reset() {
        env_  = 0.0f;
        gain_ = 1.0f;
    }

    void setThreshold(float newThreshold) {
        threshold_ = newThreshold;
    }

private:
    float threshold_;
    float attackCoeff_;
    float releaseCoeff_;

    float env_;
    float gain_;
};

#endif // DYNAMIC_RANGE_LIMITER_H
