#include "AlphaFairy.h"

extern void wifi_onConnect(void);
extern void wifi_onDisconnect(uint8_t, int);
int wifi_err_reason = 0;
extern bool autoconnect_active;
extern int autoconnect_status;
bool signal_wifiauthfailed = false;

void wifi_init()
{
    NetMgr_regCallback(wifi_onConnect, wifi_onDisconnect);
    wifiprofile_connect(config_settings.wifi_profile);
    #ifdef HTTP_ON_BOOT
    httpsrv_init();
    #endif
    if (config_settings.wifi_pwr != 0) {
        NetMgr_setWifiPower((wifi_power_t)wifipwr_table[config_settings.wifi_pwr]);
    }
}

void wifi_get_unique_ssid(char* tgt)
{
    uint8_t wifi_ap_mac[16];
    WiFi.macAddress(wifi_ap_mac);
    sprintf(tgt, WIFI_DEFAULT_SSID "-%u%u%u", wifi_ap_mac[0], wifi_ap_mac[1], wifi_ap_mac[2]);
}

void wifiprofile_getIdxFname(uint8_t idx, char* tgt)
{
    sprintf(tgt, "/wifipro_%u.txt", idx);
}

bool wifiprofile_getProfileRaw(uint8_t idx, char* ssid, char* password, uint8_t* opmode, char* guid)
{
    if (idx == 0)
    {
        #ifdef WIFI_AP_UNIQUE_NAME
        char wifi_ap_name[64];
        wifi_get_unique_ssid(ssid);
        strcpy(password, (char*)WIFI_DEFAULT_PASS);
        #else
        strcpy(ssid, (char*)WIFI_DEFAULT_SSID);
        strcpy(password, (char*)WIFI_DEFAULT_PASS);
        #endif
        *opmode = WIFIOPMODE_AP;
        if (guid != NULL) {
            ptpcam.generate_guid(guid);
        }
        return true;
    }

    char fname[32];
    char* tmp = (char*)fname;
    wifiprofile_getIdxFname(idx, fname);
    File f = SPIFFS.open(fname);
    if (!f) {
        ssid[0] = 0;
        password[0] = 0;
        *opmode = 0;
        if (guid != NULL) {
            guid[0] = 0;
        }
        return false;
    }

    int r;
    r = file_readLine(&f, ssid, WIFI_STRING_LEN + 2);
    if (r < 0) {
        ssid[0] = 0;
        password[0] = 0;
        *opmode = 0;
        f.close();
        return false;
    }
    r = file_readLine(&f, password, WIFI_STRING_LEN + 2);
    if (r < 0) {
        f.close();
        return false;
    }
    r = file_readLine(&f, tmp, WIFI_STRING_LEN + 2);
    if (r < 0) {
        f.close();
        return false;
    } else {
        if (tmp[0] == '2') {
            *opmode = WIFIOPMODE_STA;
        }
        else {
            *opmode = WIFIOPMODE_AP;
        }
    }
    if (guid != NULL) {
        r = file_readLine(&f, guid, 16 + 1);
        if (r < 0) {
            guid[0] = 0;
        }
    }
    f.close();
    return true;
}

bool wifiprofile_writeProfileRaw(uint8_t idx, char* ssid, char* password, uint8_t opmode, char* guid)
{
    if (idx == 0) {
        return false;
    }

    char fcontents[64];

    char fname[32];
    char* tmp = (char*)fname;
    wifiprofile_getIdxFname(idx, fname);

    File f = SPIFFS.open(fname, FILE_WRITE);
    if (!f) {
        return false;
    }

    f.println(ssid);
    f.println(password);
    f.println(opmode, DEC);
    if (guid != NULL) {
        if (guid[0] != 0) {
            f.println(guid);
        }
    }
    f.close();
    return true;
}

bool wifiprofile_getProfile(uint8_t idx, wifiprofile_t* p)
{
    return wifiprofile_getProfileRaw(idx, (char*)(p->ssid), (char*)(p->password), (uint8_t*)&(p->opmode), (char*)(p->guid));
}

bool wifiprofile_writeProfile(uint8_t idx, wifiprofile_t* p)
{
    return wifiprofile_writeProfileRaw(idx, (char*)(p->ssid), (char*)(p->password), p->opmode, (char*)(p->guid));
}

bool wifiprofile_connect(uint8_t idx)
{
    wifiprofile_t profile;
    if (wifiprofile_getProfile(idx, &profile))
    {
        if (idx != 0 && strlen(profile.ssid) > 0) {
            Serial.print("WiFi AP Name: ");
            Serial.println(profile.ssid);
            if (profile.opmode == WIFIOPMODE_STA) {
                NetMgr_beginSTA((char*)profile.ssid, (char*)profile.password);
            }
            else {
                NetMgr_beginAP((char*)profile.ssid, (char*)profile.password);
            }
            return true;
        }
    }

    // fallback to default settings
    #ifdef WIFI_AP_UNIQUE_NAME
        char wifi_ap_name[64];
        wifi_get_unique_ssid(wifi_ap_name);
        Serial.print("WiFi AP Name: ");
        Serial.println(wifi_ap_name);
        NetMgr_beginAP((char*)wifi_ap_name, (char*)WIFI_DEFAULT_PASS);
    #else
        NetMgr_beginAP((char*)WIFI_DEFAULT_SSID, (char*)WIFI_DEFAULT_PASS);
    #endif
    return false;
}

void wifiprofile_deleteAll()
{
    int i;
    for (i = 1; i <= WIFIPROFILE_LIMIT; i++)
    {
        wifiprofile_writeProfileRaw(i, (char*)"", (char*)"", 0, NULL);
    }
}

void wifiprofile_deleteProfile(uint8_t idx)
{
    if (idx <= 0 || idx > WIFIPROFILE_LIMIT) {
        return;
    }
    char fname[16];
    char fname2[16];
    wifiprofile_getIdxFname(idx, (char*)fname);
    SPIFFS.remove(fname);
    for (; idx < WIFIPROFILE_LIMIT; idx++)
    {
        wifiprofile_getIdxFname(idx, (char*)fname);
        wifiprofile_getIdxFname(idx + 1, (char*)fname2);
        if (SPIFFS.exists(fname2)) {
            SPIFFS.rename(fname, fname2);
        }
    }
}

bool wifiprofile_isBlank(uint8_t idx)
{
    wifiprofile_t p;
    if (wifiprofile_getProfile(idx, &p) == false) {
        return true;
    }
    if (p.ssid[0] == 0) {
        return true;
    }
    return false;
}

void wifiprofile_scanFill()
{
    uint32_t t = millis();
    Serial.printf("WiFi camera scan, start time %u\r\n", t);
    wifiprofile_t pscan, pfile;
    int n = WiFi.scanNetworks(false,false,false,100U);
    int i, j;
    for (i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        strcpy(pscan.ssid, ssid.c_str());
        if (memcmp("DIRECT-", pscan.ssid, 7) == 0)
        {
            Serial.print("WiFi scan found camera: ");
            Serial.print(pscan.ssid);
            bool found = false;
            for (j = 1; j <= WIFIPROFILE_LIMIT && found == false; j++)
            {
                bool hasfile = wifiprofile_getProfile(j, &pfile);
                if (hasfile)
                {
                    if (strcmp(pfile.ssid, pscan.ssid) == 0) {
                        found = true;
                        break;
                    }
                }
            }
            if (found == false)
            {
                for (j = 1; j <= WIFIPROFILE_LIMIT && found == false; j++)
                {
                    bool hasfile = wifiprofile_getProfile(j, &pfile);
                    if (hasfile == false || pfile.ssid[0] == 0)
                    {
                        wifiprofile_writeProfileRaw(j, pscan.ssid, (char*)"", WIFIOPMODE_STA, NULL);
                        Serial.printf("\t; wrote to profile #%u", j);
                        found = true;
                        break;
                    }
                }
            }
            Serial.println();
        }
    }
    uint32_t t2 = millis(), t3 = t2 - t;
    Serial.printf("Scan time %u\r\n", t3); // this includes all of the flash file reads
}

int wifiprofile_autoFind(wifiprofile_t* ptgt)
{
    uint32_t t = millis();
    Serial.printf("WiFi camera scan, start time %u\r\n", t);
    wifiprofile_t pscan, pfile;
    int n = WiFi.scanNetworks(false,false,false,100U);
    int i, j;
    for (i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(i);
        strcpy(pscan.ssid, ssid.c_str());
        if (memcmp("DIRECT-", pscan.ssid, 7) == 0)
        {
            Serial.print("WiFi scan found camera: ");
            Serial.print(pscan.ssid);
            for (j = 1; j <= WIFIPROFILE_LIMIT; j++)
            {
                bool hasfile = wifiprofile_getProfile(j, &pfile);
                if (hasfile)
                {
                    if (strcmp(pfile.ssid, pscan.ssid) == 0)
                    {
                        Serial.printf("\t; matches profile #%d\r\n", i);
                        if (ptgt != NULL) {
                            memcpy(ptgt, &pfile, sizeof(wifiprofile_t));
                        }
                        return i;
                    }
                }
            }
            Serial.println();
        }
    }
    uint32_t t2 = millis(), t3 = t2 - t;
    Serial.printf("Scan time %u\r\n", t3); // this includes all of the flash file reads
    return -1;
}

void force_wifi_config(const char* fp)
{
    prevent_status_bar_thread = true;

    pwr_tick(true);
    M5.Axp.GetBtnPress();
    uint32_t t = millis(), now = t;
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, fp, 0, 0);

    if (wifi_err_reason != 0)
    {
        M5Lcd.setTextFont(2);
        M5Lcd.highlight(true);
        M5Lcd.setTextWrap(true);
        M5Lcd.setHighlightColor(TFT_BLACK);
        M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Lcd.setCursor(5, M5Lcd.height() - 16);
        M5Lcd.printf("REASON: %d", wifi_err_reason);
    }

    while (true)
    {
        app_poll();
        pwr_sleepCheck();
        if (btnBoth_hasPressed()) {
            break;
        }
        if (M5.Axp.GetBtnPress() != 0) {
            show_poweroff();
            M5.Axp.PowerOff();
        }
    }
    btnBoth_clrPressed();

    run_wifi_cfg();
}

void wifi_pswdPromptDrawCb()
{
    M5Lcd.setTextWrap(false);
    M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    M5Lcd.setTextFont(2);
    M5Lcd.setCursor(5, 4);
    M5Lcd.print("SSID: ");
    M5Lcd.print(NetMgr_getSSID());
    M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), TFT_WHITE);
    M5Lcd.setCursor(5, 18);
    M5Lcd.print("Password:");
}

int wifi_promptForPassword(char* ssid, char* existingPassword, char* newPassword)
{
    static FairyKeyboard kbd(&M5Lcd);
    cpufreq_boost();

    M5Lcd.fillScreen(TFT_WHITE);
    kbd.register_redraw_cb(wifi_pswdPromptDrawCb);
    kbd.reset();

    if (existingPassword != NULL) {
        if (existingPassword[0] != 0) {
            kbd.set_str(existingPassword);
        }
    }

    while (true)
    {
        cpufreq_boost();
        autoconnect_poll();
        kbd.update(imu.roll, imu.pitch);
        if (btnSide_hasPressed())
        {
            kbd.toggleLowerCase();
            btnSide_clrPressed();
        }
        if (btnBig_hasPressed())
        {
            btnBig_clrPressed();
            if (kbd.click())
            {
                char* ns = kbd.get_str();
                if (strlen(ns) > 0)
                {
                    if (newPassword != NULL) {
                        strncpy(newPassword, ns, WIFI_STRING_LEN);
                    }
                    return WIFIPROMPTRET_DONE;
                }
                else
                {
                    return WIFIPROMPTRET_EMPTY;
                }
            }
        }
        if (btnPwr_hasPressed())
        {
            btnPwr_clrPressed();
            return WIFIPROMPTRET_CANCEL;
        }
    }
}

bool wifi_newConnectOrPrompt(uint8_t profile_num, wifiprofile_t* profile, bool need_ask, bool can_save)
{
    bool user_quit = false;
    while (user_quit == false) // this loop will prompt the user for a password until a successful connection is made (or the user cancels)
    {
        cpufreq_boost();

        if (need_ask)
        {
            dbg_ser.printf("autoconnect starting keyboard\r\n");
            int pret = wifi_promptForPassword((char*)profile->ssid, (char*)profile->password, (char*)profile->password);
            if (pret != WIFIPROMPTRET_CANCEL)
            {
                dbg_ser.printf("autoconnect keyboard done: %s\r\n", profile->password);

                wifiprofile_writeProfile(profile_num, profile);

                if (can_save && profile_num != config_settings.wifi_profile) { // only save once
                    config_settings.wifi_profile = profile_num;
                    settings_save();
                }
            }
            else
            {
                dbg_ser.printf("autoconnect user quit keyboard\r\n");
                user_quit = true;
                return true;
            }
        }

        // attempt connection
        NetMgr_reset();
        wifiprofile_connect(can_save ? config_settings.wifi_profile : profile_num);
        autoconnect_status = AUTOCONNSTS_NONE; // this variable will be set to something by the Wi-Fi callbacks

        // wait for connection (with an animation)
        dbg_ser.printf("autoconnect waiting for connection\r\n");
        gui_drawConnecting(true);
        while (autoconnect_status == AUTOCONNSTS_NONE)
        {
            gui_drawConnecting(false);
            app_poll();
            if (btnAny_hasPressed())
            {
                // any key press means to not wait for connection
                // no timeout implemented
                btnAny_clrPressed();
                user_quit = true;
                dbg_ser.printf("autoconnect waiting for connection user quit\r\n");
                return true;
            }
        }

        if (autoconnect_status == AUTOCONNSTS_CONNECTED)
        {
            // Wi-Fi is connected, cameras are not handshaken but we can quit this loop
            // this should end up in main menu after the return
            dbg_ser.printf("autoconnect wifi connect success\r\n");
            need_ask = false;
            return false;
        }
        else if (autoconnect_status == AUTOCONNSTS_FAILED)
        {
            dbg_ser.printf("autoconnect wifi failed\r\n");
            need_ask = true;
            WiFi.disconnect();
            NetMgr_reset();
            M5Lcd.drawPngFile(SPIFFS, "/wifi_reject.png", 0, 0);
            while (true)
            {
                autoconnect_poll();
                if (btnBoth_hasPressed())
                {
                    // normal button press means retry entering the password
                    dbg_ser.printf("autoconnect user wants retry\r\n");
                    btnBoth_clrPressed();
                    break;
                }
                if (btnPwr_hasPressed())
                {
                    // power button press means give up
                    dbg_ser.printf("autoconnect user wants give up (wifi_newConnectOrPrompt)\r\n");
                    btnPwr_clrPressed();
                    user_quit = true;
                    return true;
                }
            }
        }
        // user_quit is evaluated here, the end of the loop
    }
    return user_quit;
}
