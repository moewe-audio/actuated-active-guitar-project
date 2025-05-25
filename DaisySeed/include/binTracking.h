#pragma once

#include <cmath>
#include <algorithm>
#include "Constants.h"

#define GROUP_SIZE 3
#define NUM_PEAKS 1
#define NUM_BINS (FFT_SIZE / 2)
#define NUM_GROUPS ((NUM_BINS + GROUP_SIZE - 1) / GROUP_SIZE)
#define INTEGRATOR_DECAY 0.98


class BinTracking {
public:

    void init(int minFreq,
              int maxFreq) {
        this->minFreq = minFreq;
        this->maxFreq = maxFreq;

        for (int i = 0; i < numGroups; ++i) {
            prevGroupMag[i] = 0.0f;
            accumError[i]   = 0.0f;
        }

        // PI control
        kp = 1.0f;
        ki = 0.1f;
    }

    void process(const float *inData, float *outGains) {
        for (int g = 0; g < numGroups; ++g) {
            int start = g * groupSize;
            int end   = std::min(start + groupSize, numBins);

            float sum = 0.0f;
            for (int k = start; k < end; ++k) {
                float re = inData[k];
                float im = inData[k + numBins];
                sum += std::sqrt(re * re + im * im);
            }
            float groupMag = (end > start) ? (sum / (end - start)) : 0.0f;


            float error     = groupMag - prevGroupMag[g];
            prevGroupMag[g] = groupMag;

            accumError[g] = accumError[g] * INTEGRATOR_DECAY + error;

            float control = kp * error + ki * accumError[g];
            float gval    = 1.0f / (1.0f + control);
            gval          = daisysp::fclamp(gval, 0.0f, 1.0f);

            for (int k = start; k < end; ++k) {
                outGains[k] = gval;
            }
        }
    }

private:
    int   fftSize    = 0;
    float sampleRate = 0.0f;
    int   minFreq    = 0;
    int   maxFreq    = 0;
    int   groupSize  = 1;
    int   numBins    = 0;
    int   numGroups  = 0;
    int   numPeaks   = 1;

    float prevGroupMag[NUM_GROUPS];
    float accumError[NUM_GROUPS];

    float kp = 0.08f;
    float ki = 0.99f;
};
