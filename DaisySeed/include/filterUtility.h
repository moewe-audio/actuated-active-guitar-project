#ifndef FILTER3RDO_H
#define FILTER3RDO_H

#include <array>
#include <cmath>

#include "Constants.h"
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

class BiQuad {
public:
    BiQuad() : fs(SAMPLE_RATE) {
    };

    explicit BiQuad(float fs) : fs(fs) {
    };

    void setCoefficients(double a1, double a2, double b0, double b1, double b2);

    float process(float input);

    void setFilterParams(float fc, float Q);

    void setBandpass(float freq, float Q);


    std::array<double, 5> getCoefficients() const;

    float getQ() const { return Q; };
    float getFc() const { return fc; };

    float rumbleFilter1[5] = {-1.9819275f, 0.98202301f, 0.98725471f, -1.9745094f, 0.98725471f};
    float rumbleFilter2[5] = {-1.9924183f, 0.99251427f, 1.0f, -2.0f, 1.0f};

private:
    double a1{}, a2{}, b0{}, b1{}, b2 = 0.f;
    double x[2]                       = {0.0f, 0.0f};
    double y[2]                       = {0.0f, 0.0f};
    double fs;
    double Q{}, fc{};
};

inline void BiQuad::setCoefficients(double a1, double a2, double b0, double b1, double b2) {
    this->a1 = a1;
    this->a2 = a2;
    this->b0 = b0;
    this->b1 = b1;
    this->b2 = b2;
}

inline float BiQuad::process(float input) {
    float out = b0 * input + b1 * x[0] + b2 * x[1] - a1 * y[0] - a2 * y[1];
    x[1]      = x[0];
    x[0]      = input;
    y[1]      = y[0];
    y[0]      = out;
    return out;
}

inline void BiQuad::setFilterParams(float fc, float Q) {
    const float K0 = 1.0; // gain factor
    this->fc       = fc;
    this->Q        = Q;
    // Analog filter coefficients:
    double a1_analog = (2.0 * M_PI * fc) / Q;
    double a2_analog = std::pow(2.0 * M_PI * fc, 2);
    double b0_analog = K0 * (2.0 * M_PI * fc) / Q;
    double T         = 1.0 / fs;
    double K         = 2.0 / T; // equivalently, 2*fs
    double A         = K * K + a1_analog * K + a2_analog; // coefficient for z^2
    double B         = -2 * K * K + 2 * a2_analog; // coefficient for z^1
    double C         = K * K - a1_analog * K + a2_analog; // coefficient for z^0

    b0 = (b0_analog * K) / A; // coefficient for z^2 term
    b1 = 0.0; // coefficient for z^1 term (remains 0)
    b2 = -(b0_analog * K) / A; // coefficient for z^0 term

    // Digital denominator (normalized so a0 is 1):
    a1 = B / A;
    a2 = C / A;
}

inline void BiQuad::setBandpass(float freq, float Q) {
    this->fc  = freq;
    this->Q   = Q;
    double ω0 = 2 * M_PI * freq / fs;
    double α  = sin(ω0) / (2 * Q);
    // raw band-pass:
    b0       = α;
    b1       = 0;
    b2       = -α;
    float a0 = 1 + α;
    a1       = -2 * cos(ω0);
    a2       = 1 - α;
    // normalize so that gain at ω0 is unity:
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;

    setCoefficients(a1, a2, b0, b1, b2);
}

inline std::array<double, 5> BiQuad::getCoefficients() const {
    return {a1, a2, b0, b1, b2};
}

#endif // FILTER3RDO_H
