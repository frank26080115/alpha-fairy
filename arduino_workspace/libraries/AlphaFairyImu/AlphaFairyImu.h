#ifndef _ALPHAFAIRYIMU_H_
#define _ALPHAFAIRYIMU_H_

#include <stdint.h>
#include <stdbool.h>

class AlphaFairyImu
{
    public:
        AlphaFairyImu();
        void    poll();
        int     getSpin();
        void    resetSpin();
        int8_t getTilt();
        int16_t getPitch();
        bool    hasChange;
        bool    hasMajorMotion;
    //private:
        float pitch, roll, yaw; // units are degrees
        float pitch_adj, roll_adj;
        float accX, accY, accZ, gyroX, gyroY, gyroZ;
        int16_t pitchi, rolli;
        int16_t pitchai, rollai;
        uint32_t sample_timestamp = 0;
        int8_t tilt;
        int32_t spin_cnt = 0;
        bool spin_has_home = false;
        int16_t pitch_prev = 0, pitch_accum = 0;
        uint32_t spin_timestamp = 0;
};

#endif
