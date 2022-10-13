#include "AlphaFairy.h"
#include <M5DisplayExt.h>

void gui_drawVerticalDots(int x_offset, int y_margin, int y_offset, int dot_radius, int dot_cnt, int dot_idx, bool reverse, uint16_t back_color, uint16_t fore_color)
{
    // draws a line of vertical dots, with one dot that's highlighted
    // the calling function can move the highlighted dot to indicate progress/count-down/busy-status
    // defaults to being in the middle of the screen but the offsets can be defined
    int lcd_width  = M5Lcd.width();
    int lcd_height = M5Lcd.height();
    int x = (lcd_width / 2) + x_offset;
    int y_span = lcd_height - (2 * y_margin);
    int i;

    cpufreq_boost(); // drawing these dots almost always mean we are in a long execution

    if (reverse) {
        dot_idx %= dot_cnt;
        dot_idx = dot_cnt - dot_idx - 1;
    }

    for (i = 0; i < dot_cnt; i++)
    {
        int y = y_margin + y_offset + ((y_span * i) / (dot_cnt - 1));
        uint32_t color = (i == (dot_idx % dot_cnt)) ? fore_color : back_color;
        M5Lcd.fillCircle(x, y, dot_radius, color);
    }
}

void gui_startAppPrint()
{
    // setup for printing text for a specific application
    // all black screen, rotated landscape, white text
    M5Lcd.fillScreen(TFT_BLACK);
    M5Lcd.setRotation(1);
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(true);
    M5Lcd.setHighlightColor(TFT_BLACK); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
    M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
}

void gui_startMenuPrint()
{
    // setup for printing text on a menu screen
    // white text, large font
    M5Lcd.setTextFont(4);
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(false);
    M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    M5Lcd.setHighlightColor(TFT_WHITE);
}

void gui_drawConnecting(bool first)
{
    // this function will blink between a sequence of images that indicates that we are waiting for the camera to connect
    // this code can be tweaked for more animation frames if needed
    // right now it just goes between 0 and 1
    static char conn_filename[] = "/connecting0.png";
    static int last_idx = -1;
    static uint32_t t;
    if (first) {
        last_idx = -1;
        t = millis();
    }
    int cur_idx;
    uint32_t now = millis();
    uint32_t dt = now - t;
    dt %= 1400;
    cur_idx = dt / 700;
    if (cur_idx != last_idx) {
        last_idx = cur_idx;
        conn_filename[11] = '0' + cur_idx;
        cpufreq_boost();
        M5Lcd.setRotation(0);
        M5Lcd.drawPngFile(SPIFFS, conn_filename, 0, 0);
    }
    redraw_flag = true;
}

void gui_setCursorNextLine()
{
    // the new-line sequence only shifts Y and puts X back to zero
    // so this wrapper call restores X but uses the new Y
    M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
}

void gui_blankRestOfLine()
{
    uint32_t margin = (M5Lcd.getRotation() == 0) ? 8 : 65;
    uint32_t lim = M5Lcd.width() - margin;
    while (M5Lcd.getCursorX() < lim) {
        M5Lcd.print(" ");
    }
}

void gui_drawTopThickLine(uint16_t thickness, uint16_t colour)
{
    M5Lcd.fillRect(0, 0, M5Lcd.width(), thickness, colour);
}

void gui_showVal(int32_t x, uint32_t txtfmt, Print* printer)
{
    char str[64]; int i = 0;
    uint32_t txtfmt_masked = txtfmt & TXTFMT_BASEMASK;
    if (txtfmt_masked == TXTFMT_BOOL) {
        if (x == 0) {
            i += sprintf(&(str[i]), "NO");
        }
        else {
            i += sprintf(&(str[i]), "YES");
        }
    }
    else if (txtfmt_masked == TXTFMT_BULB) {
        if (x == 0) {
            // when bulb = 0, the shutter speed setting on the camera is used
            i += sprintf(&(str[i]), "(Tv)");
        }
        else {
            gui_formatSecondsTime(x, str, false);
        }
    }
    else if (txtfmt_masked == TXTFMT_TIMELONG) {
        gui_formatSecondsTime(x, str, true);
    }
    else if (txtfmt_masked == TXTFMT_TIME) {
        gui_formatSecondsTime(x, str, false);
    }
    else if (txtfmt_masked == TXTFMT_TIMEMS) {
        // if time is provided in milliseconds
        // print the time as usual (after calculating the whole seconds)
        x = x < 0 ? 0 : x;
        uint32_t tsec = x / 1000;
        uint32_t tsubsec = (x / 100) % 10; // get one decimal place
        gui_formatSecondsTime(tsec, str, false);
        // add one decimal place
        sprintf(&(str[strlen(str)]), ".%d", tsubsec);
    }
    else if (txtfmt_masked == TXTFMT_SHUTTER) {
        gui_formatShutterSpeed(x, str);
    }
    else if (txtfmt_masked == TXTFMT_ISO) {
        gui_formatISO(x, str);
    }
    else if (txtfmt_masked == TXTFMT_PROTOCOL) {
        if (x == ALLOWEDPROTOCOL_ALL) {
            i += sprintf(&(str[i]), "ALL");
        }
        else if (x == ALLOWEDPROTOCOL_PTP) {
            i += sprintf(&(str[i]), "PTP (newer)");
        }
        else if (x == ALLOWEDPROTOCOL_HTTP) {
            i += sprintf(&(str[i]), "HTTP (older)");
        }
    }
    else if (txtfmt_masked == TXTFMT_TRIGSRC) {
        if ((txtfmt & TXTFMT_SMALL) == 0)
        {
            if (x == TRIGSRC_ALL) {
                i += sprintf(&(str[i]), "all");
            }
            else if (x == TRIGSRC_MIC) {
                i += sprintf(&(str[i]), "mic");
            }
            else if (x == TRIGSRC_EXINPUT) {
                i += sprintf(&(str[i]), "ext-input");
            }
            else if (x == TRIGSRC_IMU) {
                i += sprintf(&(str[i]), "IMU");
            }
            #ifdef ENABLE_BUILD_LEPTON
            else if (x == TRIGSRC_THERMAL) {
                i += sprintf(&(str[i]), "FLIR");
            }
            #endif
        }
        else
        {
            if (x == TRIGSRC_ALL) {
                i += sprintf(&(str[i]), "ALL");
            }
            else if (x == TRIGSRC_MIC) {
                i += sprintf(&(str[i]), "MIC");
            }
            else if (x == TRIGSRC_EXINPUT) {
                i += sprintf(&(str[i]), "EXT");
            }
            else if (x == TRIGSRC_IMU) {
                i += sprintf(&(str[i]), "IMU");
            }
            #ifdef ENABLE_BUILD_LEPTON
            else if (x == TRIGSRC_THERMAL) {
                i += sprintf(&(str[i]), "FLIR");
            }
            #endif
        }
    }
    else if (txtfmt_masked == TXTFMT_TRIGACT) {
        if ((txtfmt & TXTFMT_SMALL) == 0)
        {
            if (x == TRIGACT_PHOTO) {
                i += sprintf(&(str[i]), "photo");
            }
            else if (x == TRIGACT_VIDEO) {
                i += sprintf(&(str[i]), "video");
            }
            else if (x == TRIGACT_INTERVAL) {
                i += sprintf(&(str[i]), "interval");
            }
        }
        else
        {
            if (x == TRIGACT_PHOTO) {
                i += sprintf(&(str[i]), "PIC");
            }
            else if (x == TRIGACT_VIDEO) {
                i += sprintf(&(str[i]), "VID");
            }
            else if (x == TRIGACT_INTERVAL) {
                i += sprintf(&(str[i]), "INTV");
            }
        }
    }
    else if (txtfmt_masked == TXTFMT_PINCFG) {
        if (x == PINCFG_NONE) {
            i += sprintf(&(str[i]), "none");
        }
        else {
            int pin = get_pinCfgGpio(x);
            if (pin >= 0) {
                i += sprintf(&(str[i]), "G%u", pin);
            }
            else {
                i += sprintf(&(str[i]), "G??");
            }
        }
    }
    else if (txtfmt_masked == TXTFMT_TALLEYLITE) {
        if (x == TALLYLITE_OFF) {
            i += sprintf(&(str[i]), "OFF");
        }
        else if (x == TALLYLITE_SCREEN) {
            i += sprintf(&(str[i]), "SCREEN");
        }
        else if (x == TALLYLITE_LED) {
            i += sprintf(&(str[i]), "LED");
        }
        else if (x == TALLYLITE_BOTH) {
            i += sprintf(&(str[i]), "BOTH");
        }
    }
    else if ((txtfmt & TXTFMT_DIVHUNDRED) != 0) {
        float xx = x;
        xx /= 100.0;
        i += sprintf(&(str[i]), "%0.2f", xx);
    }
    else {
        i += sprintf(&(str[i]), "%d", x);
    }

    if (((txtfmt & TXTFMT_ZEROOFF) != 0 && x == 0) || ((txtfmt & TXTFMT_NEGOFF) != 0 && x < 0)) {
        i = sprintf(&(str[0]), "OFF");
    }
    else if (((txtfmt & TXTFMT_ZEROINF) != 0 && x == 0) || ((txtfmt & TXTFMT_NEGINF) != 0 && x < 0)) {
        i = sprintf(&(str[0]), "inf.");
    }

    if (i > 0 && ((txtfmt & TXTFMT_ALLCAPS) != 0) || ((txtfmt & TXTFMT_ALLLOWER) != 0))
    {
        uint8_t j;
        for (j = 0; j < i; j++)
        {
            char c = str[j];
            if (c == 0) {
                break;
            }
            if ((txtfmt & TXTFMT_ALLCAPS) != 0)
            {
                if (c >= 'a' && c <= 'z') {
                    str[j] -= 'a';
                    str[j] += 'A';
                }
            }
            else if ((txtfmt & TXTFMT_ALLLOWER) != 0)
            {
                if (c >= 'A' && c <= 'A') {
                    str[j] += 'a' - 'A';
                }
            }
        }
    }

    if (txtfmt_masked == TXTFMT_LCDBRITE) {
        M5.Axp.ScreenBreath(x);
    }

    if (printer != NULL) {
        printer->print(str);
    }
}

int8_t gui_drawFocusPullState(int y)
{
    int8_t dir = imu_getFocusPull();
    if (y < -30) {
        return dir;
    }
    uint16_t pink = 0xFF3C;
    fpull_drawOneArrowLeft ( 5 +  0, y, dir <= -3 ? TFT_RED : pink);
    fpull_drawOneArrowLeft ( 5 + 20, y, dir <= -2 ? TFT_RED : pink);
    fpull_drawOneArrowLeft ( 5 + 40, y, dir <= -1 ? TFT_RED : pink);
    fpull_drawOneArrowRight(71 +  0, y, dir >=  1 ? TFT_RED : pink);
    fpull_drawOneArrowRight(71 + 20, y, dir >=  2 ? TFT_RED : pink);
    fpull_drawOneArrowRight(71 + 40, y, dir >=  3 ? TFT_RED : pink);
    return dir;
}

void gui_drawLevelBar(int32_t lvl1, int32_t lvl2, int32_t thresh1, int32_t thresh2)
{
    static TFT_eSprite* level_canvas = NULL;
    if (level_canvas == NULL) {
        level_canvas = new TFT_eSprite(&M5Lcd);
        level_canvas->createSprite(M5Lcd.width() - GENERAL_ICON_WIDTH, MICTRIG_LEVEL_MARGIN);
    }

    #define MICTRIG_LEVEL_BAR_HEIGHT   8
    #define MICTRIG_LEVEL_TRIG_HEIGHT 12

    int16_t ysplit = MICTRIG_LEVEL_TRIG_HEIGHT / 2;

    level_canvas->fillSprite(TFT_BLACK);
    if (lvl1 >= 0 && lvl2 < 0) {
        level_canvas->fillRect(0      , 0, lvl1, MICTRIG_LEVEL_BAR_HEIGHT    , TFT_RED  );
    }
    else if (lvl1 < 0 && lvl2 >= 0) {
        level_canvas->fillRect(0      , 0, lvl2, MICTRIG_LEVEL_BAR_HEIGHT    , TFT_RED  );
    }
    else if (lvl1 >= 0 && lvl2 >= 0) {
        level_canvas->fillRect(0      , 0     , lvl1, ysplit, TFT_RED  );
        level_canvas->fillRect(0      , ysplit, lvl2, ysplit, TFT_RED  );
    }
    if (thresh1 >= 0 && thresh2 < 0) {
        level_canvas->fillRect(thresh1, 0, 3   , MICTRIG_LEVEL_TRIG_HEIGHT, lvl2 > lvl1 ? TFT_DARKGREEN : TFT_GREEN);
    }
    else if (thresh2 >= 0 && thresh1 < 0) {
        level_canvas->fillRect(thresh2, 0, 3   , MICTRIG_LEVEL_TRIG_HEIGHT, lvl2 > lvl1 ? TFT_GREEN : TFT_DARKGREEN);
    }
    else if (thresh1 >= 0 && thresh2 >= 0) {
        level_canvas->fillRect(thresh1, 0     , 3   , ysplit, TFT_GREEN);
        level_canvas->fillRect(thresh2, ysplit, 3   , ysplit, TFT_GREEN);
    }
    level_canvas->pushSprite(0, 0);
}

void draw_borderRect(int16_t thickness, uint16_t colour)
{
    int16_t i;
    for (i = 0; i < thickness; i++)
    {
        M5Lcd.drawRect(0 + i, 0 + i, M5Lcd.width() - 1 - (i * 2), M5Lcd.height() - 1 - (i * 2), colour);
    }
}

void interval_drawTimerStart()
{
    //M5Lcd.drawPngFile(SPIFFS, "/timer_blank.png", M5Lcd.width() - GENERAL_ICON_WIDTH, M5Lcd.height() - GENERAL_ICON_WIDTH);
    M5Lcd.fillRect(M5Lcd.width() - GENERAL_ICON_WIDTH, M5Lcd.height() - GENERAL_ICON_WIDTH, GENERAL_ICON_WIDTH, GENERAL_ICON_WIDTH, TFT_BLACK);
}

void interval_drawTimerLine(int16_t cx, int16_t cy, int8_t i, uint16_t colour)
{
    float hand = 12.0;

    float ang = (-M_PI * 2.0f * (float)i) / CLOCK_ANG_DIV;
    float nx0 = hand * sin(ang);
    float ny0 = hand * cos(ang);
    float nx1 = 1.8 * sin(ang + (M_PI / 2.0f));
    float ny1 = 1.8 * cos(ang + (M_PI / 2.0f));
    float nx2 = 1.8 * sin(ang - (M_PI / 2.0f));
    float ny2 = 1.8 * cos(ang - (M_PI / 2.0f));

    M5Lcd.fillTriangle(
        cx + lround(nx0), cy + lround(ny0),
        cx + lround(nx1), cy + lround(ny1),
        cx + lround(nx2), cy + lround(ny2),
        colour);
}

void interval_drawTimerCircle(int16_t cx, int16_t cy)
{
    // draws a thick circle
    M5Lcd.drawCircle(cx, cy, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx, cy, 19, TFT_WHITE);
    M5Lcd.drawCircle(cx, cy + 1, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx, cy - 1, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx + 1, cy, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx - 1, cy, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx + 1, cy + 1, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx - 1, cy + 1, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx + 1, cy - 1, 18, TFT_WHITE);
    M5Lcd.drawCircle(cx - 1, cy - 1, 18, TFT_WHITE);
}

void fpull_drawOneArrowLeft(int16_t x, int16_t y, uint16_t colour)
{
    M5Lcd.fillTriangle(x    , y + 11, x + 11    , y + 11, x + 8     , y     , colour);
    M5Lcd.fillTriangle(x    , y + 11, x + 11    , y + 11, x + 8 + 11, y     , colour);
    M5Lcd.fillTriangle(x + 8, y     , x + 8 + 11, y     , x         , y + 11, colour);
    M5Lcd.fillTriangle(x + 8, y     , x + 8 + 11, y     , x + 11    , y + 11, colour);

    M5Lcd.fillTriangle(x    , y + 12, x + 11    , y + 11, x + 8     , y + 24, colour);
    M5Lcd.fillTriangle(x    , y + 12, x + 11    , y + 11, x + 8 + 11, y + 24, colour);
    M5Lcd.fillTriangle(x + 8, y + 24, x + 8 + 11, y + 24, x         , y + 12, colour);
    M5Lcd.fillTriangle(x + 8, y + 24, x + 8 + 11, y + 24, x + 11    , y + 12, colour);
}

void fpull_drawOneArrowRight(int16_t x, int16_t y, uint16_t colour)
{
    M5Lcd.fillTriangle(x    , y     , x + 11    , y     , x + 8     , y + 11, colour);
    M5Lcd.fillTriangle(x    , y     , x + 11    , y     , x + 11 + 8, y + 11, colour);
    M5Lcd.fillTriangle(x + 8, y + 11, x + 11 + 8, y + 11, x         , y     , colour);
    M5Lcd.fillTriangle(x + 8, y + 11, x + 11 + 8, y + 11, x + 11    , y     , colour);

    M5Lcd.fillTriangle(x    , y + 24, x + 11    , y + 24, x + 8     , y + 12, colour);
    M5Lcd.fillTriangle(x    , y + 24, x + 11    , y + 24, x + 11 + 8, y + 12, colour);
    M5Lcd.fillTriangle(x + 8, y + 12, x + 11 + 8, y + 12, x         , y + 24, colour);
    M5Lcd.fillTriangle(x + 8, y + 12, x + 11 + 8, y + 12, x + 11    , y + 24, colour);
}
