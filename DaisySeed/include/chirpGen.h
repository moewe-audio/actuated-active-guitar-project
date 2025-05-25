#ifndef CHIRP_H
#define CHIRP_H

#include <cmath>
#include <cstddef>

class Chirp {
public:
    Chirp() : sampleCounter(0), playing(false) {}
    void init(float f0, float f1, float durationSec, float sampleRate);
    void start();
    void stop();
    float nextSample();
    bool isPlaying() const;

private:
    float f0, f1, durationSec, sampleRate;
    float beta; // rate of exponential increase
    float A;    // phase scaling factor: 2*pi*f0 / beta
    std::size_t totalSamples;
    std::size_t sampleCounter;
    bool playing;
};

void Chirp::init(float f0, float f1, float durationSec, float sampleRate)
{
    this->f0 = f0;
    this->f1 = f1;
    this->durationSec = durationSec;
    this->sampleRate = sampleRate;

    totalSamples = static_cast<std::size_t>(durationSec * sampleRate);
    beta = logf(f1 / f0) / durationSec;
    A = 2 * M_PI * f0 / beta;

    sampleCounter = 0;
    playing = false;
}

void Chirp::start()
{
    sampleCounter = 0;
    playing = true;
}

void Chirp::stop()
{
    playing = false;
}

float Chirp::nextSample()
{
    if (!playing) {
        return 0.0f;
    }
    if (sampleCounter >= totalSamples) {
        playing = false;
        return 0.0f;
    }

    float t = static_cast<float>(sampleCounter) / sampleRate;
    float phase = A * (expf(beta * t) - 1);
    float sample = sinf(phase);

    sampleCounter++;
    return sample;
}

bool Chirp::isPlaying() const
{
    return playing;
}

#endif // CHIRP_H
