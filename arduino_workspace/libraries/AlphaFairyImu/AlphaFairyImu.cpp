#include "AlphaFairyImu.h"
#include <Wire.h>
#include <Arduino.h>
#include <M5StickCPlus.h>

#define TILT_THRESH 40
#define TILT_HYSTER 20

extern void MahonyAHRSupdate(float gx, float gy, float gz, float ax, float ay,
                      float az, float mx, float my, float mz);
// void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay,
// float az);
extern void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay,
                         float az, float *pitch, float *roll, float *yaw);

AlphaFairyImu::AlphaFairyImu()
{
    hasChange = false;
    tilt = TILT_IS_FLAT;
    resetSpin();
}

void AlphaFairyImu::poll()
{
    uint8_t old_tilt = tilt;
    int32_t old_spin = spin_cnt;
    uint32_t now = millis();

    // MahonyAHRS wants 25 Hz sample frequency (40 millisecond intervals)
    // going any faster and it will integrate more gyro spin than it's supposed to
    // slowing it down will lower gyro sensitivity
    if ((now - sample_timestamp) < 40) {
        return;
    }
    sample_timestamp = now;

    M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
    M5.IMU.getAccelData(&accX, &accY, &accZ);
    MahonyAHRSupdateIMU(gyroX * DEG_TO_RAD, gyroY * DEG_TO_RAD, gyroZ * DEG_TO_RAD, accX, accY, accZ, &pitch, &roll, &yaw);

    // generalize the angle being reported into a tilt direction
    if (roll >= TILT_THRESH || (tilt == TILT_IS_UP && roll >= (TILT_THRESH - TILT_HYSTER))) {
        tilt = TILT_IS_UP;
    }
    else if (roll <= -TILT_THRESH || (tilt == TILT_IS_DOWN && roll <= -(TILT_THRESH - TILT_HYSTER))) {
        tilt = TILT_IS_DOWN;
    }
    else {
        tilt = TILT_IS_FLAT;
    }

    pitchi = lroundf(pitch);

    if (roll > 90 || roll < -90)
    {
        // is upside down
        // the pitch number goes 0 to 90 then back to 0 again as it approaches what should be 180
        // so we read the roll to see if it's about to go upside down, and recalculate pitch
        if (pitchi < 0) {
            pitchi = -90 + (-90 - pitchi);
            pitch  = -90 + (-90 - pitch);
        }
        else {
            pitchi = 90 + (90 - pitchi);
            pitch  = 90 + (90 - pitch);
        }
    }

    if (((pitchi > -45 && pitchi < 0) || (pitchi < 45 && pitchi >= 0)) && (roll < 90 && roll > -90))
    {
        // near flat, which is when we can count the spins

        if (spin_has_home == false) {
            spin_has_home = true;
            spin_cnt = 0;
            pitch_accum = 0;
        }
        if ((now - spin_timestamp) > 500) // rate limit
        {
            // see which direction we have spun
            if (pitch_accum > 180) {
                spin_cnt++;
                spin_timestamp = now;
            }
            else if (pitch_accum < -180) {
                spin_cnt--;
                spin_timestamp = now;
            }
        }
        pitch_accum = 0;
    }
    else
    {
        // we are not near flat, so accumulate the spin amount

        int32_t diff;

        // impose a limit on the frame-to-frame difference in angle detected, to avoid jitter
        #define DIFF_LIMIT 30
        #define LIMIT_DIFF_TO() do { diff = (diff > DIFF_LIMIT) ? DIFF_LIMIT : ((diff < -DIFF_LIMIT) ? -DIFF_LIMIT : diff); } while (0)

        if (pitchi >= 0 && pitch_prev >= 0) {
            diff = pitchi - pitch_prev;
            LIMIT_DIFF_TO();
            pitch_accum += diff;
        }
        else if (pitchi <= 0 && pitch_prev <= 0) {
            diff = pitch_prev - pitchi;
            LIMIT_DIFF_TO();
            pitch_accum -= diff;
        }
        else if (pitchi >= 0 && pitch_prev <= 0) {
            diff = (-180 - pitch_prev) - (180 - pitchi);
            LIMIT_DIFF_TO();
            pitch_accum += diff;
        }
        else if (pitchi <= 0 && pitch_prev >= 0) {
            diff = (180 - pitch_prev) - (-180 - pitchi);
            LIMIT_DIFF_TO();
            pitch_accum += diff;
        }
    }
    pitch_prev = pitchi;

    if (old_tilt != tilt) {
        hasChange = true;
    }
    if (old_spin != spin_cnt) {
        hasChange = true;
    }
}

int AlphaFairyImu::getSpin()
{
    if (spin_has_home == false) {
        return 0;
    }
    return spin_cnt;
}

void AlphaFairyImu::resetSpin()
{
    spin_has_home = false;
    spin_cnt = 0;
    pitch_accum = 0;
}

uint8_t AlphaFairyImu::getTilt()
{
    return tilt;
}

int16_t AlphaFairyImu::getPitch()
{
    return pitchi;
}