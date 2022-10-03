#include "AlphaFairy.h"

#ifdef ENABLE_BUILD_LEPTON
extern bool lepton_enable_poll;
#endif

void app_waitAllReleaseGfx(uint8_t waitgfx)
{
    btnAny_clrPressed();
    if (btnSide_isPressed() == false && btnBig_isPressed() == false)
    {
        return;
    }

    #ifdef ENABLE_BUILD_LEPTON
    lepton_enable_poll = false;
    #endif

    cpufreq_boost();

    if (waitgfx == WAITGFX_CONNECTING)
    {
        gui_drawConnecting(true);
    }
    else if (waitgfx == WAITGFX_UNSUPPORTED)
    {
        M5Lcd.drawPngFile(SPIFFS, "/unsupported.png", 0, 0);
    }

    uint32_t now = millis();
    uint32_t last_time = now;
    do
    {
        app_poll();

        if (waitgfx == WAITGFX_CONNECTING)
        {
            gui_drawConnecting(false);
        }

        if (btnSide_isPressed() || btnBig_isPressed()) {
            last_time = millis();
        }
    }
    while ((last_time - (now = millis())) < BTN_DEBOUNCE);

    redraw_flag = true;

    #ifdef ENABLE_BUILD_LEPTON
    lepton_enable_poll = true;
    #endif
}

void app_waitAllRelease()
{
    app_waitAllReleaseGfx(WAITGFX_NONE);
}

void app_waitAnyPress(bool can_sleep)
{
    #ifdef ENABLE_BUILD_LEPTON
    lepton_enable_poll = false;
    #endif
    while (true)
    {
        app_poll();
        if (can_sleep == false) {
            pwr_tick(true);
        }
        if (btnAny_hasPressed()) {
            break;
        }
    }
    btnAny_clrPressed();
    #ifdef ENABLE_BUILD_LEPTON
    lepton_enable_poll = true;
    #endif
}

void app_waitAllReleaseConnecting()
{
    app_waitAllReleaseGfx(WAITGFX_CONNECTING);
}

void app_waitAllReleaseUnsupported()
{
    app_waitAllReleaseGfx(WAITGFX_UNSUPPORTED);
}

void app_sleep(uint32_t x, bool forget_btns)
{
    uint32_t tstart = millis();
    uint32_t now;
    #ifdef ENABLE_BUILD_LEPTON
    lepton_enable_poll = false;
    #endif
    while (((now = millis()) - tstart) < x) {
        app_poll();
    }
    if (forget_btns) {
        btnAny_clrPressed();
    }
    #ifdef ENABLE_BUILD_LEPTON
    lepton_enable_poll = true;
    #endif
}

int8_t imu_getFocusPull()
{
    int n;
    int ang = lroundf(imu.getPitch());
    int aang = (ang < 0) ? (-ang) : (ang);
    if (aang >= 2) {
        if (ang > 0) {
            n = aang / 12;
        }
        else {
            n = aang / 7;
        }
        n = n > 3 ? 3 : n;
    }
    return (ang < 0) ? (-n) : (n);
}

int focus_tiltToStepSize(int8_t tilt)
{
    // translate tilt into Sony's focus step sizes
    int atilt = tilt < 0 ? -tilt : tilt;
    int n = (atilt ==  2) ?  SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM : ((atilt ==  3) ?  SONYALPHA_FOCUSSTEP_FARTHER_LARGE : n);
    return (tilt < 0) ? -n : n;
}

uint32_t shutter_to_millis(uint32_t x)
{
    uint16_t* p16 = (uint16_t*)&x;
    if (x == 0 || x == 0xFFFFFFFF) {
        return 0;
    }
    uint32_t nn = p16[1];
    uint32_t dd = p16[0];
    nn *= 1000;
    float n = nn;
    float d = dd;
    float y = d != 0 ? (n/d) : 0;
    return lroundf(y);
}

void gui_formatSecondsTime(int32_t x, char* str, bool shorten)
{
    // format time in 00:00:00 format when provided in seconds
    // optionally shortens to 0H:00 format when the time is very long
    int i = 0;
    if (x < 0) {
        // negative sign
        i += sprintf(&(str[i]), "-");
        x *= -1;
    }
    uint32_t mins = x / 60;
    uint32_t hrs = mins / 60;
    mins %= 60;
    uint32_t secs = x % 60;
    if (hrs > 0) {
        i += sprintf(&(str[i]), "%u", hrs);
        if (shorten) {
            // add a H just so we understand it's HH:MM instead of MM:SS
            i += sprintf(&(str[i]), "H");
        }
        i += sprintf(&(str[i]), ":");
        if (mins < 10) {
            i += sprintf(&(str[i]), "0");
        }
    }
    if (shorten && hrs > 0) {
        i += sprintf(&(str[i]), "%u", mins);
    }
    else {
        i += sprintf(&(str[i]), "%u:%02u", mins, secs);
    }
}

void gui_formatShutterSpeed(uint32_t x, char* str)
{
    uint16_t* p16 = (uint16_t*)&x;
    if (x == 0 || x == 0xFFFFFFFF) {
        sprintf(str, "BULB");
        return;
    }
    uint16_t nn = p16[1];
    uint16_t dd = p16[0];
    float n = nn;
    float d = dd;
    float y = d != 0 ? (n/d) : 0;
    sprintf(str, "%0.1f\"", y);
    if (y >= 4 || nn == dd || y == 2.0 || y == 3.0) {
        sprintf(str, "%u\"", lroundf(y));
        return;
    }
    if (y >= 0.35) {
        return;
    }
    if (nn >= 10 && dd > 10) {
        return;
    }
    if (nn == 1) {
        sprintf(str, "1/%u", dd);
    }
    return;
}

void gui_formatISO(uint32_t x, char* str)
{
    uint16_t* p16 = (uint16_t*)&x;
    if (x == 0) {
        sprintf(str, "???");
        return;
    }
    if (x == 0xFFFFFF) {
        sprintf(str, "AUTO");
        return;
    }
    x &= 0xFFFFFF;
    sprintf(str, "%u", x);
    return;
}

int file_readLine(File* f, char* tgt, int charlimit)
{
    int i = 0;
    if (f->available() <= 0) {
        return -1;
    }

    while (f->available() > 0) // until end of file
    {
        char c = f->read();
        
        if (c != '\r' && c != '\n' && c != '\0') // is not terminator
        {
            if (i < charlimit - 1) // if there is room in string buffer
            {
                // append char to string
                tgt[i] = c;
                i += 1;
                tgt[i] = 0;
            }
        }
        else // is terminator
        {
            // end the string
            tgt[i] = 0;
            if (i > 0) { // this trims the start of a line
                i += 1;
                break;
            }
        }
    }
    return i;
}

void dissolve_restart(uint16_t colour)
{
    uint32_t t = millis();
    cpufreq_boost();
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    while (btnBig_isPressed())
    {
        int x = rand() % M5Lcd.width();
        int y = rand() % M5Lcd.height();
        if ((millis() - t) < 5000)
        {
            M5Lcd.fillRect(x, y, 1, 1, colour);
        }
        else
        {
            M5Lcd.fillRect(x, y, 1, 1, TFT_BLACK);
            int32_t b = config_settings.lcd_brightness - (((millis() - t) - 5000) / 1250);
            M5.Axp.ScreenBreath(b);
        }
    }
    ESP.restart();
}

int32_t get_pinCfgGpio(int32_t x)
{
    #ifndef ENABLE_BUILD_LEPTON
    switch (x)
    {
        case PINCFG_G0 : return 0;
        case PINCFG_G25: return 25;
        case PINCFG_G26: return 26;
        case PINCFG_G36: return 36;
    }
    #endif
    return -1;
}

void safe_all_pins()
{
    #ifndef ENABLE_BUILD_LEPTON
    pinMode(0, INPUT);
    pinMode(25, INPUT);
    pinMode(26, INPUT);
    pinMode(36, INPUT);
    #endif
}
