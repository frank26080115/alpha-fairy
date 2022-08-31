#include "AlphaFairy.h"

#define QIKRMT_ROLL_SPAN 60
#define QIKRMT_HYSTER    3
#define QIKRMT_FPULL_Y   198
enum
{
    QIKRMTBTN_IDLE,
    QIKRMTBTN_PRESSED_LOCKING_WAIT,
    QIKRMTBTN_PRESSED_UNLOCKING_WAIT,
};

enum
{
    QIKRMTIMU_FREE,
    QIKRMTIMU_FREE_TEMP,
    QIKRMTIMU_LOCKED,
};

uint8_t qikrmt_imuState = QIKRMTIMU_LOCKED;
uint8_t qikrmt_col = 0;
uint8_t qikrmt_row = 0;
int8_t  qikrmt_col_prev = -1;
int8_t  qikrmt_row_prev = -1;
int8_t  qikrmt_imustate_prev = -1;
int     qikrmt_roll_center = 0;

void qikrmt_task(bool freeze_row)
{
    int ang;

    uint32_t now = millis();
    static uint32_t btn_down_time = 0;
    if (btnSide_hasPressed())
    {
        btn_down_time = millis();
        btnSide_clrPressed();
        if (qikrmt_imuState == QIKRMTIMU_LOCKED) {
            qikrmt_imuState = QIKRMTIMU_FREE;
        }
        else {
            qikrmt_imuState = QIKRMTIMU_LOCKED;
        }
    }
    else if (btnSide_isPressed() && btn_down_time > 0 && (qikrmt_imuState == QIKRMTIMU_FREE_TEMP || qikrmt_imuState == QIKRMTIMU_FREE))
    {
        if ((now - btn_down_time) > 1000)
        {
            qikrmt_imuState = QIKRMTIMU_FREE_TEMP;
        }
    }
    else if (btnSide_isPressed() == false)
    {
        if (qikrmt_imuState == QIKRMTIMU_FREE_TEMP) {
            qikrmt_imuState = QIKRMTIMU_LOCKED;
        }
    }

    if (qikrmt_imuState == QIKRMTIMU_FREE || qikrmt_imuState == QIKRMTIMU_FREE_TEMP)
    {
        if (freeze_row == false)
        {
            ang = imu.rolli;
            ang = ang > 90 ? 90 : (ang < -90 ?  -90 : ang);

            if (ang > qikrmt_roll_center + (QIKRMT_ROLL_SPAN / 2)) { // exceeded boundary, shift the center point
                qikrmt_roll_center = ang - (QIKRMT_ROLL_SPAN / 2);
                qikrmt_row = 2;
            }
            else if (ang < qikrmt_roll_center - (QIKRMT_ROLL_SPAN / 2)) { // exceeded boundary, shift the center point
                qikrmt_roll_center = ang + (QIKRMT_ROLL_SPAN / 2);
                qikrmt_row = 0;
            }
            else if (qikrmt_row == 0 && ang >= qikrmt_roll_center - (QIKRMT_ROLL_SPAN / (3 * 2)) + QIKRMT_HYSTER)
            {
                qikrmt_row = 1;
            }
            else if (qikrmt_row == 1 && ang >= qikrmt_roll_center + (QIKRMT_ROLL_SPAN / (3 * 2)) + QIKRMT_HYSTER)
            {
                qikrmt_row = 2;
            }
            else if (qikrmt_row == 1 && ang <= qikrmt_roll_center - (QIKRMT_ROLL_SPAN / (3 * 2)) - QIKRMT_HYSTER)
            {
                qikrmt_row = 0;
            }
            else if (qikrmt_row == 2 && ang <= qikrmt_roll_center + (QIKRMT_ROLL_SPAN / (3 * 2)) - QIKRMT_HYSTER)
            {
                qikrmt_row = 1;
            }
        }

        ang = imu.pitchi;
        ang = ang > 90 ? 90 : (ang < -90 ?  -90 : ang);

        if (qikrmt_col == 0 && ang >= QIKRMT_HYSTER) {
            qikrmt_col = 1;
        }
        else if (qikrmt_col == 1 && ang <= -QIKRMT_HYSTER) {
            qikrmt_col = 0;
        }
    }

    if (qikrmt_col != qikrmt_col_prev || qikrmt_row != qikrmt_row_prev || qikrmt_imuState != qikrmt_imustate_prev || redraw_flag)
    {
        pwr_tick(true); // movement means don't turn off
        if (qikrmt_row_prev >= 0 && qikrmt_col_prev >= 0) {
            qikrmt_drawBox(qikrmt_row_prev, qikrmt_col_prev, TFT_WHITE);
        }
        qikrmt_drawBox(qikrmt_row, qikrmt_col, qikrmt_imuState == QIKRMTIMU_LOCKED ? TFT_BLACK : TFT_ORANGE);
        if (qikrmt_row != 2) {
            M5Lcd.fillRect(0, QIKRMT_FPULL_Y, M5Lcd.width(), 26, TFT_WHITE); // blank out the arrows for focus pull
        }
    }
    if (qikrmt_row == 2) {
        gui_drawFocusPullState(QIKRMT_FPULL_Y);
    }

    qikrmt_col_prev = qikrmt_col;
    qikrmt_row_prev = qikrmt_row;
    qikrmt_imustate_prev = qikrmt_imuState;
}

void qikrmt_drawBox(uint8_t row, uint8_t col, uint16_t colour)
{
    uint16_t linewidth = M5Lcd.width();
    uint16_t xstart = 1, xwidth = linewidth / 2;
    if (col == 1 && row != 2) {
        xstart = (linewidth / 2) + 1;
    }
    uint16_t boxheight = 50;
    uint16_t ystart = 44 + (boxheight * row);
    M5Lcd.drawRect(xstart    , ystart    , (row == 2 ? linewidth : (linewidth / 2)) - 2, boxheight    , colour);
    M5Lcd.drawRect(xstart + 1, ystart + 1, (row == 2 ? linewidth : (linewidth / 2)) - 4, boxheight - 2, colour);
}

void quick_remote(void* mip)
{
    qikrmt_imuState = QIKRMTIMU_LOCKED;
    qikrmt_col = 0;
    qikrmt_row = 0;
    qikrmt_col_prev = -1;
    qikrmt_row_prev = -1;
    qikrmt_imustate_prev = -1;
    qikrmt_roll_center = 0;
    redraw_flag = true;

    app_waitAllRelease(BTN_DEBOUNCE);

    while (true)
    {
        app_poll();

        if (redraw_flag) {
            M5Lcd.drawPngFile(SPIFFS, "/qikrmt_active.png", 0, 0);
        }

        qikrmt_task(false);
        gui_drawStatusBar(false);
        pwr_sleepCheck();

        redraw_flag = false;

        if (btnBig_hasPressed())
        {
            btnBig_clrPressed();
            if (qikrmt_row == 0 && qikrmt_col == 0) { // remote shutter
                remote_shutter(mip);
            }
            else if (qikrmt_row == 0 && qikrmt_col == 1) { // record movie
                record_movie(mip);
            }
            else if (qikrmt_row == 1) // zoom
            {
                bool can_do = true;
                if (fairycam.isOperating() == false)
                {
                    app_waitAllReleaseConnecting(BTN_DEBOUNCE);
                    can_do = false;
                }
                if (can_do)
                {
                    bool do_one = true;
                    while ((btnBig_isPressed() || do_one) && fairycam.isOperating())
                    {
                        do_one = false;
                        app_poll();
                        int8_t n = qikrmt_col == 0 ? -1 : +1;
                        if (ptpcam.isOperating())
                        {
                            if (n != 0) {
                                ptpcam.cmd_ZoomStep((n > 0) ? -1 : ((n < 0) ? +1 : 0)); // I am soooo sorry for this
                                ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                            }
                        }
                        if (httpcam.isOperating())
                        {
                            httpcam.cmd_ZoomStart(n);
                            httpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                        }
                    }
                    if (ptpcam.isOperating()) {
                        ptpcam.cmd_ZoomStep(0);
                    }
                    if (httpcam.isOperating()) {
                        httpcam.cmd_ZoomStop();
                    }
                }
            }
            else if (qikrmt_row == 2) // focus
            {
                bool can_do = true;
                if (ptpcam.isOperating() == false)
                {
                    if (httpcam.isOperating()) {
                        app_waitAllReleaseUnsupported(BTN_DEBOUNCE);
                        can_do = false;
                    }
                    else {
                        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
                        can_do = false;
                    }
                }
                if (can_do)
                {
                    bool starting_mf = ptpcam.is_manuallyfocused();

                    if (starting_mf == false && ptpcam.isOperating()) {
                        // force into manual focus mode
                        ptpcam.cmd_ManualFocusMode(true, false);
                        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                    }

                    bool do_one = true;

                    while ((btnBig_isPressed() || do_one) && ptpcam.isOperating())
                    {
                        do_one = false;

                        int8_t n = gui_drawFocusPullState(QIKRMT_FPULL_Y); // return is -3 to +3
                        app_poll();
                        // translate n into Sony's focus step sizes
                        if (n >= 0) {
                            n = (n ==  2) ?  3 : ((n ==  3) ?  7 : n);
                        }
                        else {
                            n = (n == -2) ? -3 : ((n == -3) ? -7 : n);
                        }
                        if (n != 0) {
                            ptpcam.cmd_ManualFocusStep(n);
                            ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                        }
                    }
                    
                    if (starting_mf == false && ptpcam.isOperating()) {
                        // restore AF state
                        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                        ptpcam.cmd_ManualFocusMode(false, false);
                    }
                }
            }
        }

        if (btnPwr_hasPressed())
        {
            // this will quit out of the quick remote mode
            btnPwr_clrPressed();
            break;
        }
    }

    // the only way to quit is the pwr button
}
