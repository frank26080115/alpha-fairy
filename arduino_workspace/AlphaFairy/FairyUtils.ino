#include "AlphaFairy.h"

void wifi_get_unique_ssid(char* tgt)
{
    uint8_t wifi_ap_mac[16];
    WiFi.macAddress(wifi_ap_mac);
    sprintf(tgt, WIFI_DEFAULT_SSID "-%u%u%u", wifi_ap_mac[0], wifi_ap_mac[1], wifi_ap_mac[2]);
}

extern void wifi_onConnect(void);
extern void wifi_onDisconnect(uint8_t, int);
int wifi_err_reason = 0;

void wifi_init()
{
    NetMgr_regCallback(wifi_onConnect, wifi_onDisconnect);
    wifiprofile_connect(config_settings.wifi_profile);
    httpsrv_init();
}

void wifi_onDisconnect(uint8_t x, int reason)
{
    if (x == WIFIDISCON_NORMAL)
    {
        Serial.printf("WiFi disconnected normal, reason %d\r\n", reason);
        if (httpcam.getState() != 0) {
            httpcam.begin(0); // this resets the forbidden flag so it can reconnect again later 
        }
    }
    else if (x == WIFIDISCON_AUTH_ERROR)
    {
        Serial.printf("WiFi disconnected error, reason %d\r\n", reason);
        wifi_err_reason = reason;
        critical_error("/wifi_error.png");
    }
    else if (x == WIFIDISCON_AUTH_FAIL)
    {
        Serial.printf("WiFi disconnected auth failed, reason %d\r\n", reason);
        wifi_err_reason = reason;
        force_wifi_config("/wifi_reject.png");
    }
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


void gui_showVal(int32_t x, uint16_t txtfmt, Print* printer)
{
    char str[16]; int i = 0;
    if ((txtfmt & TXTFMT_BOOL) != 0) {
        if (x == 0) {
            i += sprintf(&(str[i]), "NO");
        }
        else {
            i += sprintf(&(str[i]), "YES");
        }
    }
    else if ((txtfmt & TXTFMT_BULB) != 0) {
        if (x == 0) {
            // when bulb = 0, the shutter speed setting on the camera is used
            i += sprintf(&(str[i]), "(Tv)");
        }
        else {
            gui_formatSecondsTime(x, str, false);
        }
    }
    else if ((txtfmt & TXTFMT_TIMELONG) != 0) {
        gui_formatSecondsTime(x, str, true);
    }
    else if ((txtfmt & TXTFMT_TIME) != 0) {
        gui_formatSecondsTime(x, str, false);
    }
    else if ((txtfmt & TXTFMT_TIMEMS) != 0) {
        // if time is provided in milliseconds
        // print the time as usual (after calculating the whole seconds)
        x = x < 0 ? 0 : x;
        uint32_t tsec = x / 1000;
        uint32_t tsubsec = (x / 100) % 10; // get one decimal place
        gui_formatSecondsTime(tsec, str, false);
        // add one decimal place
        sprintf(&(str[strlen(str)]), ".%d", tsubsec);
    }
    else if ((txtfmt & TXTFMT_SHUTTER) != 0) {
        gui_formatShutterSpeed(x, str);
    }
    else if ((txtfmt & TXTFMT_ISO) != 0) {
        gui_formatISO(x, str);
    }
    else {
        i += sprintf(&(str[i]), "%d", x);
    }

    if ((txtfmt & TXTFMT_LCDBRITE) != 0) {
        M5.Axp.ScreenBreath(x);
    }

    if (printer != NULL) {
        printer->print(str);
    }
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
