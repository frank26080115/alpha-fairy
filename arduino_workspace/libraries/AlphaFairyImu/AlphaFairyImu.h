#ifndef _ALPHAFAIRYIMU_H_
#define _ALPHAFAIRYIMU_H_

#include <stdint.h>
#include <stdbool.h>

enum
{
  TILT_IS_FLAT = 0,
  TILT_IS_UP,
  TILT_IS_DOWN,
};

class AlphaFairyImu
{
    public:
        AlphaFairyImu();
        void    poll();
        int     getSpin();
        void    resetSpin();
        uint8_t getTilt();
        int16_t getPitch();
        bool    hasChange;
    //private:
        float pitch, roll, yaw; // units are degrees
        float accX, accY, accZ, gyroX, gyroY, gyroZ;
        uint32_t sample_timestamp = 0;
        uint8_t tilt;
        int32_t spin_cnt = 0;
        int16_t pitchi;
        bool spin_has_home = false;
        int16_t pitch_prev = 0, pitch_accum = 0;
        uint32_t spin_timestamp = 0;
};

#endif