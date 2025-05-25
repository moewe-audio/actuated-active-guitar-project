#pragma once
#include <cstring>
#include <cmath>
#include "arm_math.h"


/// FFTProcessor: block‐based FFT/IFFT with 50% overlap‐add and Hann windowing.
class FFTProcessor {
public:
    static constexpr size_t FFT_SIZE = 4096;
    static constexpr size_t HOP_SIZE = FFT_SIZE / 2;

    /// Initialize FFT instance, precompute Hann window, and clear buffers.
    void init()
    {
        arm_rfft_fast_init_f32(&fftInst_, FFT_SIZE);
        for(size_t i = 0; i < FFT_SIZE; i++)
            window_[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));

        std::memset(inBuf_,    0, sizeof(inBuf_));
        std::memset(procBuf_,  0, sizeof(procBuf_));
        std::memset(ringBuf_,  0, sizeof(ringBuf_));

        inIndex_    = 0;
        readIndex_  = 0;
        writeIndex_ = 0;
    }

    float processSample(float input)
    {
        inBuf_[inIndex_++] = input;
        if(inIndex_ >= FFT_SIZE)
        {
            for(size_t i = 0; i < FFT_SIZE; i++)
                procBuf_[i] = inBuf_[i] * window_[i];

            arm_rfft_fast_f32(&fftInst_, procBuf_, procBuf_, 0);
            arm_rfft_fast_f32(&fftInst_, procBuf_, procBuf_, 1);
            for(size_t i = 0; i < FFT_SIZE; i++)
            {
                float v = procBuf_[i] * window_[i];
                ringBuf_[(writeIndex_ + i) % FFT_SIZE] += v;
            }

            writeIndex_ = (writeIndex_ + HOP_SIZE) % FFT_SIZE;
            std::memmove(inBuf_, inBuf_ + HOP_SIZE, (FFT_SIZE - HOP_SIZE) * sizeof(float));
            inIndex_ = FFT_SIZE - HOP_SIZE;
        }

        float output = ringBuf_[readIndex_];
        ringBuf_[readIndex_] = 0.0f; // clear after reading
        readIndex_ = (readIndex_ + 1) % FFT_SIZE;

        return output;
    }

private:
    arm_rfft_fast_instance_f32 fftInst_;
    float inBuf_[FFT_SIZE];    // sliding input window
    float procBuf_[FFT_SIZE];  // FFT/IFFT workspace
    float window_[FFT_SIZE];    // Hann window
    float ringBuf_[FFT_SIZE];   // circular overlap-add buffer

    size_t inIndex_    = 0;     // where to write next input
    size_t readIndex_  = 0;     // where to read next output
    size_t writeIndex_ = 0;     // where to add next processed block
};
