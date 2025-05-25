//
// Created by Levin on 24.04.25.
//

#ifndef RUNNINGRMS_H
#define RUNNINGRMS_H

#include "daisy_seed.h"
#include <cmath>
#include <array>

template <std::size_t N>
class RunningRMS {
public:
    RunningRMS() : writeIdx(0), sumSquares(0.0), count(0) {
        buf.fill(0.0f);
    }

    void addSample(float x) {
        if (count == N) sumSquares -= double(buf[writeIdx]) * buf[writeIdx];
        else           ++count;

        buf[writeIdx] = x;
        sumSquares   += double(x) * x;
        writeIdx      = (writeIdx + 1) % N;
    }

    float getRMS() const {
        return count
             ? std::sqrt(sumSquares / double(count))
             : 0.0f;
    }

    void reset() {
        buf.fill(0.0f);
        writeIdx   = 0;
        sumSquares = 0.0;
        count      = 0;
    }

private:
    std::array<float, N> buf;
    std::size_t writeIdx;
    double      sumSquares;
    std::size_t count;
};


#endif //RUNNINGRMS_H
