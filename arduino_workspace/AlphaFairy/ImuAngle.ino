#include "AlphaFairy.h"

float imu_pitch, imu_roll, imu_yaw; // units are degrees
uint8_t imu_angle = ANGLE_IS_FLAT;
bool imu_hasChange = false;

int32_t imu_pitchi;
int32_t spin_cnt = 0;
bool spin_has_home = false;
int32_t imu_pitch_prev = 0, imu_pitch_accum = 0;
uint32_t spin_timestamp = 0;

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

    imu_pitchi = lroundf(imu_pitch);

    if (imu_roll > 90 || imu_roll < -90)
    {
        // is upside down
        // the pitch number goes 0 to 90 then back to 0 again as it approaches what should be 180
        // so we read the roll to see if it's about to go upside down, and recalculate pitch
        if (imu_pitchi < 0) {
            imu_pitchi = -90 + (-90 - imu_pitchi);
            imu_pitch = -90 + (-90 - imu_pitch);
        }
        else {
            imu_pitchi = 90 + (90 - imu_pitchi);
            imu_pitch = 90 + (90 - imu_pitch);
        }
    }

    if ((imu_pitchi > -45 && imu_pitchi < 0) || (imu_pitchi < 45 && imu_pitchi >= 0))
    {
        // near flat, which is when we can count the spins

        if (spin_has_home == false) {
            spin_has_home = true;
            spin_cnt = 0;
            imu_pitch_accum = 0;
        }
        uint32_t now = millis();
        if ((now - spin_timestamp) > 500) // rate limit
        {
            // see which direction we have spun
            if (imu_pitch_accum > 180) {
                spin_cnt++;
                spin_timestamp = now;
            }
            else if (imu_pitch_accum < -180) {
                spin_cnt--;
                spin_timestamp = now;
            }
        }
        imu_pitch_accum = 0;
    }
    else
    {
        // we are not near flat, so accumulate the spin amount
        if (imu_pitchi >= 0 && imu_pitch_prev >= 0) {
            imu_pitch_accum += imu_pitchi - imu_pitch_prev;
        }
        else if (imu_pitchi <= 0 && imu_pitch_prev <= 0) {
            imu_pitch_accum -= imu_pitch_prev - imu_pitchi;
        }
        else if (imu_pitchi >= 0 && imu_pitch_prev <= 0) {
            imu_pitch_accum -=  180 - imu_pitchi;
            imu_pitch_accum += -180 - imu_pitch_prev;
        }
        else if (imu_pitchi <= 0 && imu_pitch_prev >= 0) {
            imu_pitch_accum -= -180 - imu_pitchi;
            imu_pitch_accum +=  180 - imu_pitch_prev;
        }
    }
    imu_pitch_prev = imu_pitchi;
}

void spin_reset()
{
    spin_has_home = false;
    spin_cnt = 0;
    imu_pitch_accum = 0;
}

void imu_showSpinDebug()
{
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    M5Lcd.printf("%0.1f , %0.1f", imu_roll, imu_pitch);
    gui_blankRestOfLine();
    M5Lcd.println();
    gui_setCursorNextLine();
    M5Lcd.printf("%d", imu_pitch_accum);
    gui_blankRestOfLine();
    M5Lcd.println();
    gui_setCursorNextLine();
    M5Lcd.printf("spin %d", spin_cnt);
    gui_blankRestOfLine();
}

/*
note: the calculations here sucks, not anywhere near good enough for most robotics applications, but we just need a generalized angle
*/
