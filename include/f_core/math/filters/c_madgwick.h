#ifndef C_MADGWICK_H
#define C_MADGWICK_H

#include <zsl/zsl.h>
#include <zsl/orientation/orientation.h>
class CMadgwick {
public:
    /**
     * Constructor
     * @param frequencyHz Frequency in Hz for the filter
     */
    CMadgwick(zsl_real_t frequencyHz) : frequencyHz(frequencyHz), beta(-1) {
    }

    /**
     * Initialize the beta term for the Madgwick filter
     * @return ZSL status code
     */
    int Initialize() {
        return zsl_fus_cal_madg(&zsl_fus_data_gyr, &zsl_fus_data_acc,
             &zsl_fus_data_mag, 100.0, NULL, &beta);
    }

    int Feed() {

    }

private:
    zsl_real_t frequencyHz;
    zsl_real_t beta;
};

#endif //C_MADGWICK_H
