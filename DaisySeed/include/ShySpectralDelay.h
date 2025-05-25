#ifndef SHYSPECTRALDELAY_H
#define SHYSPECTRALDELAY_H

#include <cmath>
#include "shy_fft.h"
#include "fourier.h"
#include "wave.h"
#include "binTracking.h"
#include "Constants.h"

#define MIN_FREQ 120.f
#define MIN_FX_FREQ 80.f
#define MAX_FX_FREQ 10000.f
#define MAX_DELAY_FRAMES    50
#define SPECTRAL_RING_LEN    (MAX_DELAY_FRAMES + 1)

using namespace daisysp;

class SpectralDelay {
public:
    SpectralDelay()
        : fft(new ShyFFT<float, FFT_SIZE, RotationPhasor>())
        , window([](float phase)-> float {
              return 0.5f * (1.0f - cosf(2.0f * M_PI * phase));
          })
        , stft(new soundmath::Fourier<float, FFT_SIZE>(
              processor,
              fft,
              &window,
              LAPS,
              in,
              middle,
              out
          )) {
        fft->Init();
    }

    ~SpectralDelay() {
        delete stft;
        delete fft;
    }


    void write(float sample) {
        stft->write(sample);
    }

    float read() {
        return stft->read();
    }

private:
    static void processor(const float *inData, float *outData) {
        static int           head      = 0;
        static bool          inited    = false;
        static constexpr int minBin    = MIN_FREQ * FFT_SIZE / SAMPLE_RATE;
        static constexpr int minFxBin  = MIN_FX_FREQ * FFT_SIZE / SAMPLE_RATE;
        static constexpr int maxFxBin  = MAX_FX_FREQ * FFT_SIZE / SAMPLE_RATE;
        static constexpr int numFxBins = (maxFxBin - minFxBin + 1);
        static float         history[SPECTRAL_RING_LEN][numFxBins * 2];
        static int           delayDist[FFT_SIZE];
        static float         baseDelay[numFxBins];
        const float frameRate  = SAMPLE_RATE / HOP_SIZE;
        static BinTracking magCtrl;
        static float       phaseOffset[numFxBins];
        static float       cosOffset[numFxBins];
        static float       sinOffset[numFxBins];
        static float       depth[numFxBins];
        static float       magGains[FFT_SIZE / 2];
        static Oscillator  lfo_sin, lfo_cos;

        if (!inited) {
            magCtrl.init(MIN_FX_FREQ, MAX_FX_FREQ);

            for (auto &row: history)
                for (float &v: row)
                    v = 0.0f;

            lfo_sin.Init(frameRate);
            lfo_sin.SetWaveform(Oscillator::WAVE_SIN);
            lfo_sin.SetFreq(1.f);
            lfo_sin.SetAmp(1.0f);

            lfo_cos.Init(frameRate);
            lfo_cos.SetWaveform(Oscillator::WAVE_SIN);
            lfo_cos.SetFreq(1.f);
            lfo_cos.SetAmp(1.0f);
            lfo_cos.Reset(0.25f); // +90° → π/2

            for(int k=minFxBin; k<=maxFxBin; ++k)
            {
                int   i    = k - minFxBin;
                float norm = float(i) / numFxBins;           // 0…1

                baseDelay[i] = 20;
                depth    [i] = norm * 20;
            }

            inited = true;
        }

        float sinPhi = lfo_sin.Process();
        float cosPhi = lfo_cos.Process();

        magCtrl.process(inData, magGains);

        for (int k = minFxBin; k <= maxFxBin; ++k) {
            int   i = k - minFxBin;
            float  m    = depth[i] * sin(sinPhi*cosOffset[i] + cosPhi*sinOffset[i]);
            float  D    = baseDelay[i] + m;
            int    d0   = int(floorf(D));
            delayDist[k]  = d0;
        }

        for (int k = 0; k < FFT_SIZE / 2; ++k) {
            if (k >= minFxBin && k <= maxFxBin) {
                int i                    = k - minFxBin;
                history[head][2 * i]     = inData[k];
                history[head][2 * i + 1] = inData[k + FFT_SIZE / 2];
                int   idx0               = (head + delayDist[k]) % SPECTRAL_RING_LEN;
                int   idx1               = (idx0 + 1) % SPECTRAL_RING_LEN;
                float frac               = (baseDelay[i] +
                                            depth[i] * (sinPhi * cosOffset[i] + cosPhi * sinOffset[i]))
                                           - float(delayDist[k]);

                // real + imag interpolation
                float real = history[idx0][2 * i] * (1 - frac)
                             + history[idx1][2 * i] * frac;
                float imag = history[idx0][2 * i + 1] * (1 - frac)
                             + history[idx1][2 * i + 1] * frac;

                outData[k]                = magGains[k] * real;
                outData[k + FFT_SIZE / 2] = imag;
            } else {
                outData[k]                = inData[k];
                outData[k + FFT_SIZE / 2] = inData[k + FFT_SIZE / 2];
            }
        }

        head = (head + 1) % SPECTRAL_RING_LEN;
    }


    ShyFFT<float, FFT_SIZE, RotationPhasor> *fft;
    soundmath::Wave<float>                   window;
    soundmath::Fourier<float, FFT_SIZE> *    stft;
    float                                    in[FFT_SIZE * LAPS * 2];
    float                                    middle[FFT_SIZE * LAPS * 2];
    float                                    out[FFT_SIZE * LAPS * 2];
};

#endif // SHYSPECTRALDELAY_H
