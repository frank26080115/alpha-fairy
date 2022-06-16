#include "AlphaFairy.h"
#include <M5StickC.h>

float imu_pitch, imu_roll, imu_yaw; // units are degrees
uint8_t imu_angle = ANGLE_IS_FLAT;
bool imu_hasChange = false;

void app_anglePoll()
{
    uint8_t old_angle = imu_angle;
    M5.IMU.getAhrsData(&imu_pitch, &imu_roll, &imu_yaw);
    if (imu_roll >= 30) {
        imu_angle = ANGLE_IS_UP;
    }
    else if (imu_roll <= -30) {
        imu_angle = ANGLE_IS_DOWN;
    }
    else {
        imu_angle = ANGLE_IS_FLAT;
    }
    if (old_angle != imu_angle) {
        imu_hasChange = true;
    }
}

bool app_isAngleUp()
{
    return imu_roll >= 30;
}

bool app_isAngleDown()
{
    return imu_roll <= -30;
}
