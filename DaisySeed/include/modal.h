//
// Created by Levin on 17.05.25.
//

#ifndef MODAL_H
#define MODAL_H

struct ModalMode {
    float f0;
    float Q;
};

struct ModalFilter {
    int       numModes;
    ModalMode model[];
};

struct Modal {
    double r,      cosTheta, gain;
    double y1 = 0, y2 = 0;

    void init(double f0, double Q, double fs) {
        r        = exp(-M_PI * f0 / (Q * fs));
        double theta = 2 * M_PI * f0 / fs;
        cosTheta     = cos(theta);
        gain = (1.0 - r);
    }

    float process(float x) {
        // y[n] = 2⋅r⋅cosθ⋅y[n-1] − r²⋅y[n-2] + gain⋅x[n]
        double y0 = 2.0 * r * cosTheta * y1
                    - r * r * y2
                    + gain * x;
        y2 = y1;
        y1 = y0;
        return (float) y0;
    }
};

static constexpr ModalFilter BANJO_FILTER{
    42,
    {
        {200.0021, 96.0000},
        {459.0883, 112.0000},
        {719.7046, 128.0000},
        {318.6715, 112.0000},
        {583.4653, 128.0000},
        {846.0968, 144.0000},
        {427.1143, 128.0000},
        {700.0369, 144.0000},
        {966.3873, 160.0000},
        {530.6189, 144.0000},
        {811.7950, 160.0000},
        {1082.4352, 176.0000},
        {631.0997, 160.0000},
        {920.2187, 176.0000},
        {1195.3207, 192.0000},
        {729.4980, 176.0000},
        {1026.1647, 192.0000},
        {1305.7364, 208.0000},
        {826.3564, 192.0000},
        {1130.1805, 208.0000},
        {1414.1567, 224.0000},
        {922.0201, 208.0000},
        {1232.6404, 224.0000},
        {1520.9234, 240.0000},
        {1016.7242, 224.0000},
        {1333.8136, 240.0000},
        {1626.2921, 256.0000},
        {1110.6371, 240.0000},
        {1433.9006, 256.0000},
        {1730.4597, 272.0000},
        {1203.8840, 256.0000},
        {1533.0559, 272.0000},
        {1833.5816, 288.0000},
        {1296.5609, 272.0000},
        {1631.4011, 288.0000},
        {1935.7829, 304.0000},
        {1388.7433, 288.0000},
        {1729.0341, 304.0000},
        {2037.1661, 320.0000},
        {1480.4919, 304.0000},
        {2137.8163, 336.0000}
    }
};

struct ModalBPFilter {
    BiQuad filters[BANJO_FILTER.numModes];

    void init() {
        for (int i = 0; i < BANJO_FILTER.numModes; i++) {
            filters[i] = BiQuad();
            filters[i].setFilterParams(BANJO_FILTER.model[i].f0, BANJO_FILTER.model[i].Q);
        }
    }

    float process(float in) {
        float sum = 0;
        for (int i = 0; i < BANJO_FILTER.numModes; i++) {
            sum += filters[i].process(in);
        }
        return sum;
    }
};

#endif // MODAL_H
