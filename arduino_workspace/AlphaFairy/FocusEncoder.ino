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

    if (config_settings.fenc_multi == 0) {
        config_settings.fenc_multi = 1;
    }
    int32_t absmulti = config_settings.fenc_multi < 0 ? -config_settings.fenc_multi : config_settings.fenc_multi;

    fencoder.task(); // do the I2C IO operations required to read encoder
    int16_t d = fencoder.read(true); // get the relative step count

    if (d != 0) {
        pwr_tick(true);
        fenc_val += d * config_settings.fenc_multi;
        uint32_t tspan = now - prev_move_time;
        if (absmulti > 1) {
            wait_time = tspan / (absmulti * 2);
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
        int16_t absval = fenc_val < 0 ? -fenc_val : fenc_val;
        int16_t large_step = config_settings.fenc_large * 2;
        //dbg_ser.printf("focus knob %d\r\n", fenc_val);

        ptpcam.cmd_ManualFocusStep(((absval >= large_step && large_step > 2) ? SONYALPHA_FOCUSSTEP_FARTHER_LARGE : SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM)  // large step size if needed, default medium step size
                                   * (fenc_val < 0 ? -1 : 1) * (config_settings.fenc_multi < 0 ? -1 : 1)                                                // account for direction
                                   );

        fenc_val -= ((absval >= large_step && large_step > 2) ? large_step : 2) * (fenc_val < 0 ? -1 : 1);

        prev_execute_time = now;
        if (wait_time != 0) { // do only one loop if a wait has been specified
            break;
        }
        if (millis() - now > 200) { // don't hang the thread
            break;
        }
    }
}

bool fenc_canOperate()
{
    return ptpcam.isOperating() && ptpcam.is_manuallyfocused() && fencoder.avail();
}

void fenc_calib_sleep(uint32_t x)
{
    uint32_t tstart = millis();
    uint32_t now;
    while (((now = millis()) - tstart) < x) {
        pwr_tick(true);
        ptpcam.task();
        yield();
    }
}

bool fenc_calibrate()
{
    #define FENC_CHECK_CALIB_FAILED() do { if (ptpcam.isOperating() == false) {                      Serial.printf("MF-Calib: FAILED\r\n"); return false; } } while (0)
    #define FENC_CHECK_BTN_QUIT()     do { if (btnPwr_hasPressed())           { btnPwr_clrPressed(); Serial.printf("MF-Calib: QUIT\r\n");   return false; } } while (0)
    #define FENC_DOT_TICK()           do { gui_drawVerticalDots(0, 40, -1, 5, 5, dot_idx++, false, TFT_GREEN, TFT_RED); } while (0)
    Serial.println("Manual Focus Calibration Start");
    ptpcam.set_debugflags(0);
    redraw_flag = true;
    FENC_CHECK_CALIB_FAILED();

    int32_t fdist_now;
    int dot_idx = 0;

    bool starting_mf = ptpcam.is_manuallyfocused();
    if (starting_mf == false)
    {
        ptpcam.cmd_ManualFocusMode(true, false);
    }

    bool do_one_more = true;
    uint32_t t = millis(), now = t;
    while (((now = millis()) - t) < 3000 || do_one_more)
    {
        pwr_tick(true);
        FENC_CHECK_CALIB_FAILED();
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_LARGE);
        fenc_calib_sleep(config_settings.focus_pause_time_ms);
        FENC_DOT_TICK();
        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_now <= 0)
        {
            if (do_one_more) {
                do_one_more = false;
            }
            else {
                break;
            }
        }
    }

    int32_t fdist_near = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
    Serial.printf("MF-Calib: done homing, dist 0x%08X == %d\r\n", fdist_near, fdist_near);

    ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_FARTHER_LARGE);
    fenc_calib_sleep(config_settings.focus_pause_time_ms * 8);
    int32_t fdist_lrg = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
    Serial.printf("MF-Calib: one large step, dist 0x%08X == %d\r\n", fdist_lrg);

    int med_steps = 0, med_extra = 0;
    while (true)
    {
        pwr_tick(true);
        FENC_CHECK_CALIB_FAILED();
        FENC_CHECK_BTN_QUIT();
        med_extra++;
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM);
        fenc_calib_sleep(config_settings.focus_pause_time_ms * 4);
        FENC_DOT_TICK();
        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_now > fdist_lrg) {
            break;
        }
    }

    Serial.printf("MF-Calib: %u extra medium steps, dist 0x%08X == %d\r\n", fdist_now);

    do_one_more = true;
    t = millis();
    while (((now = millis()) - t) < 3000 || do_one_more)
    {
        pwr_tick(true);
        FENC_CHECK_CALIB_FAILED();
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_LARGE);
        fenc_calib_sleep(config_settings.focus_pause_time_ms);
        FENC_DOT_TICK();
        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_now <= 0)
        {
            if (do_one_more) {
                do_one_more = false;
            }
            else {
                break;
            }
        }
    }

    Serial.printf("MF-Calib: home again\r\n");

    while (true)
    {
        pwr_tick(true);
        FENC_CHECK_CALIB_FAILED();
        FENC_CHECK_BTN_QUIT();
        med_steps++;
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM);
        fenc_calib_sleep(config_settings.focus_pause_time_ms * 4);
        FENC_DOT_TICK();
        fdist_now = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        if (fdist_now > fdist_lrg) {
            break;
        }
    }

    int delta_steps = med_steps - med_extra;
    Serial.printf("MF-Calib: %u medium steps equals one large step\r\n", delta_steps);
    config_settings.fenc_large = delta_steps;
    settings_save();

    if (starting_mf == false)
    {
        ptpcam.cmd_ManualFocusMode(false, false);
    }

    Serial.printf("Manual Focus Calibration Done\r\n");
    return true;
}

void focus_calib_write(uint16_t colour)
{
    M5Lcd.setCursor(10, 116);
    M5Lcd.setTextFont(4);
    M5Lcd.highlight(true);
    M5Lcd.setHighlightColor(TFT_WHITE);
    M5Lcd.setTextColor(colour, TFT_WHITE);
    M5Lcd.printf("%u", config_settings.fenc_large);
}

void focus_calib(void* mip)
{
    dbg_ser.println("focus_calib");

    if (ptpcam.isOperating() == false) {
        if (httpcam.isOperating()) {
            app_waitAllReleaseUnsupported(BTN_DEBOUNCE);
        }
        else {
            // show user that the camera isn't connected
            app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        }
        return;
    }

    M5Lcd.drawPngFile(SPIFFS, "/focus_calib.png", 0, 0);
    bool success = fenc_calibrate();
    M5Lcd.drawPngFile(SPIFFS, "/focus_calib.png", 0, 0);

    focus_calib_write(success ? TFT_BLACK : TFT_RED);
}
