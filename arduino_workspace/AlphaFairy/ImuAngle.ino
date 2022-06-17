#include "AlphaFairy.h"

float imu_pitch, imu_roll, imu_yaw; // units are degrees
uint8_t imu_angle = ANGLE_IS_FLAT;
bool imu_hasChange = false;

void app_anglePoll()
{
    uint8_t old_angle = imu_angle;
    M5.IMU.getAhrsData(&imu_pitch, &imu_roll, &imu_yaw); // this will do the I2C transactions and calculations
    // generalize the angle being reported
    if (imu_roll >= ANGLE_THRESH || (imu_angle == ANGLE_IS_UP && imu_roll >= (ANGLE_THRESH - ANGLE_HYSTER))) {
        imu_angle = ANGLE_IS_UP;
    }
    else if (imu_roll <= -ANGLE_THRESH || (imu_angle == ANGLE_IS_DOWN && imu_roll <= -(ANGLE_THRESH - ANGLE_HYSTER))) {
        imu_angle = ANGLE_IS_DOWN;
    }
    else {
        imu_angle = ANGLE_IS_FLAT;
    }
    if (old_angle != imu_angle) {
        imu_hasChange = true;
    }
}

/*
note: the calculations here sucks, not anywhere near good enough for most robotics applications, but we just need a generalized angle
*/
