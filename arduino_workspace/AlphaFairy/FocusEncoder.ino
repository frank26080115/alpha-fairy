#include "AlphaFairy.h"
#include <FairyEncoder.h>

int32_t fenc_val;

void fenc_task()
{
    static bool prev_can_run = false;
    bool can_run = ptpcam.isOperating() && ptpcam.is_manuallyfocused();
    // we don't want to waste IO time if the camera is not connected or not in manual focus mode
    if (prev_can_run != can_run) {
        fenc_val = 0;
        fencoder.read(true);
        if (can_run) {
            pwr_tick(true);
            dbg_ser.println("focus knob can run");
        }
        else {
            dbg_ser.println("focus knob cannot run");
        }
    }
    prev_can_run = can_run;
    if (can_run == false) {
        return;
    }

    uint32_t now = millis();
    static uint32_t prev_execute_time = 0;
    static uint32_t prev_move_time = 0;
    static uint32_t wait_time = 0;

    fencoder.task(); // do the I2C IO operations required to read encoder

    int16_t d = fencoder.read(true); // get the relative step count
    if (d != 0) {
        pwr_tick(true);
        if (config_settings.fenc_multi == 0) {
            config_settings.fenc_multi = 1;
        }
        fenc_val += d * config_settings.fenc_multi;
        uint32_t tspan = now - prev_move_time;
        if (config_settings.fenc_multi > 1) {
            wait_time = tspan / (config_settings.fenc_multi * 2);
            if (wait_time > config_settings.focus_pause_time_ms) {
                wait_time = config_settings.focus_pause_time_ms;
            }
        }
        else {
            wait_time = 0;
        }
        prev_move_time = now;
    }
    else if (d == 0 && (fenc_val > 1 || fenc_val < -1) && (now - prev_move_time) > 500)
    {
        // stop the movement if knob stopped moving for a while
        fenc_val = 0;
    }

    if ((now - prev_execute_time) < wait_time && wait_time > 0) {
        return;
    }

    while (fenc_val > 1 || fenc_val < -1)
    {
        //dbg_ser.printf("focus knob %d\r\n", fenc_val);
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM * (fenc_val < 0 ? -1 : 1) * (config_settings.fenc_multi > 0 ? 1 : -1));
        fenc_val -= 2 * (fenc_val < 0 ? -1 : 1);
        prev_execute_time = now;
        if (wait_time != 0) { // do only one loop if a wait has been specified
            break;
        }
        if (millis() - now > 200) { // don't hang the thread
            break;
        }
    }
}

#if 0
void fenc_calib_sleep(uint32_t x)
{
    uint32_t tstart = millis();
    uint32_t now;
    while (((now = millis()) - tstart) < x) {
        ptpcam.task();
        yield();
    }
}

void fenc_calibrate()
{
    Serial.println("Manual Focus Calibration Start");
    ptpcam.set_debugflags(0);

    bool starting_mf = ptpcam.is_manuallyfocused();
    if (starting_mf == false)
    {
        ptpcam.cmd_ManualFocusMode(true, false);
    }

    uint32_t t = millis(), now = t;
    while (((now = millis()) - t) < 5000)
    {
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_LARGE);
        fenc_calib_sleep(config_settings.focus_pause_time_ms);
    }

    fenc_calib_sleep(config_settings.focus_pause_time_ms * 8);

    int32_t fdist_near = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
    Serial.printf("MF-Calib: done homing, dist 0x%08X == %d\r\n", fdist_near, fdist_near);

    t = millis();
    while (((now = millis()) - t) < 5000)
    {
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_FARTHER_LARGE);
        fenc_calib_sleep(config_settings.focus_pause_time_ms);
    }

    fenc_calib_sleep(config_settings.focus_pause_time_ms * 8);

    int32_t fdist_inf = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
    int32_t fdist_far;
    Serial.printf("MF-Calib: done homing infinity, dist 0x%08X == %d\r\n", fdist_inf, fdist_inf);

    while (true)
    {
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_SMALL);
        fenc_calib_sleep(config_settings.focus_pause_time_ms * 4);
        fdist_far = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_far != fdist_inf)
        {
            break;
        }
    }
    Serial.printf("MF-Calib: done homing far point, dist 0x%08X == %d\r\n", fdist_far, fdist_far);

    int32_t fdist_now, fdist_last = -1;
    #if 0
    int small_steps = 0;
    while (true)
    {
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_SMALL);
        small_steps++;
        fenc_calib_sleep(config_settings.focus_pause_time_ms * 4);
        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_now != fdist_last) {
            Serial.printf(".");
            fdist_last = fdist_now;
        }
        if (fdist_now == fdist_near)
        {
            break;
        }
    }

    Serial.printf("\r\nMF-Calib: small steps for full range %d\r\n", small_steps);
    #endif

    int medium_steps = 0;
    fdist_last = -1;
    while (true)
    {
        ptpcam.cmd_ManualFocusStep(
            #if 0
            SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM
            #else
            SONYALPHA_FOCUSSTEP_CLOSER_MEDIUM
            #endif
            );
        medium_steps++;
        fenc_calib_sleep(config_settings.focus_pause_time_ms * 4);
        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_now != fdist_last) {
            Serial.printf(".");
            fdist_last = fdist_now;
        }
        #if 0
        if (fdist_now == fdist_far || fdist_now == fdist_inf)
        #else
        if (fdist_now == fdist_near)
        #endif
        {
            break;
        }
    }

    Serial.printf("MF-Calib: medium steps for full range %d\r\n", medium_steps);

    int dly = config_settings.focus_pause_time_ms;
    int ideal_delay = dly;
    dly &= 0x7FFFFFFE;
    while (dly > 0)
    {
        int steps_taken = 0;
        fdist_last = -1;
        Serial.printf("MF-Calib: speed test start, medium steps, %s, delay = %d ... ", (dly % 2) != 0 ? "closer" : "farther", dly & 0x7FFFFFFE);
        while (steps_taken < medium_steps)
        {
            ptpcam.cmd_ManualFocusStep((dly % 2) != 0 ? SONYALPHA_FOCUSSTEP_CLOSER_MEDIUM : SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM);
            steps_taken++;
            fenc_calib_sleep(dly);
            fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
            if (fdist_now != fdist_last) {
                Serial.printf(".");
                fdist_last = fdist_now;
            }
        }

        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (((dly % 2) != 0 && (fdist_now > fdist_near)) || ((dly % 2) == 0 && (fdist_now < fdist_far)))
        {
            Serial.printf(" ... missed, cur %d -> tgt %d ... ", fdist_now, (dly % 2) != 0 ? fdist_near : fdist_far);
            int steps_missed = 0;
            fdist_last = -1;
            while (steps_missed < 100)
            {
                ptpcam.cmd_ManualFocusStep((dly % 2) != 0 ? SONYALPHA_FOCUSSTEP_CLOSER_MEDIUM : SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM);
                steps_missed++;
                fenc_calib_sleep(config_settings.focus_pause_time_ms * 4);
                fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
                fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
                if (fdist_now != fdist_last) {
                    Serial.printf(".");
                    fdist_last = fdist_now;
                }
                if (((dly % 2) != 0 && (fdist_now <= fdist_near)) || ((dly % 2) == 0 && (fdist_now >= fdist_far || fdist_now == fdist_inf)))
                {
                    break;
                }
            }

            Serial.printf(" ... extra steps = %d (vs %d)\r\n", steps_missed, medium_steps);
            if (steps_missed <= 2) {
                ideal_delay = dly;
            }
        }
        else
        {
            Serial.printf(" ... done and on-target, steps = %d\r\n", steps_taken);
            ideal_delay = dly;
        }

        if ((dly % 2) == 0)
        {
            dly -= 21;
        }
        else
        {
            dly -= 19;
        }
    }

    if (starting_mf == false)
    {
        ptpcam.cmd_ManualFocusMode(false, false);
    }

    Serial.printf("Manual Focus Calibration Done, ideal delay = %d\r\n", ideal_delay);
}
#endif