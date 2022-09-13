#include "AlphaFairy.h"
#include <FairyKeyboard.h>

bool autoconnect_active = false;
int  autoconnect_status = 0;
extern volatile uint32_t btnPwr_cnt;

void autoconnect_poll()
{
    yield();
    imu.poll();
    cmdline.task();
    pwr_sleepCheck(); // this will only dim the screen, because pwr_tick is always being called
    if (imu.hasMajorMotion) {
        imu.hasMajorMotion = false;
        pwr_tick(true);
    }
    else {
        pwr_tick(false);
    }
    uint8_t b = M5.Axp.GetBtnPress();
    if (b != 0) {
        btnPwr_cnt++;
    }
}

void autoconnect(void* mip)
{
    autoconnect_active = true;
    autoconnect_status = 0;

    uint8_t result_code = 0;
    uint8_t result_profile = 0;
    wifiprofile_t profile;

    bool user_quit = false;

    int draw_idx = 0;
    uint32_t t = millis();
    uint32_t now = t;

    WiFi.disconnect(); // halt wifi activity for now

    btnAll_clrPressed();

    dbg_ser.println("autoconnect starting");

    while (true)
    {
        autoconnect_poll();

        if (((now = millis()) - t) > 333) // time for new animation frame
        {
            t = now;
            // rotate the icon's position in a loop
            int x, y;
            switch (draw_idx)
            {
                case 0: x = 71; y = 141; break;
                case 1: x =  8; y = 141; break;
                case 2: x =  8; y =  39; break;
                case 3: x = 71; y =  39; break;
            }
            draw_idx = (draw_idx + 1) % 4;

            M5Lcd.setRotation(0);
            M5Lcd.fillRect(0,  39, M5Lcd.width(), 62, TFT_WHITE); // remove old icons
            M5Lcd.fillRect(0, 141, M5Lcd.width(), 62, TFT_WHITE); // remove old icons
            M5Lcd.drawPngFile(SPIFFS, "/autoconn_icon.png", x, y);
            gui_drawStatusBar(false);
        }

        int scan_ret = WiFi.scanComplete();
        if (scan_ret > 0) // has results
        {
            dbg_ser.printf("autoconnect scan complete, %u results\r\n", scan_ret);
            int i, j;
            // first check if anything is broadcasting a SSID that we already have in database
            for (i = 1; i <= WIFIPROFILE_LIMIT; i++) // for all profiles
            {
                if (wifiprofile_getProfile(i, &profile)) // profile exists
                {
                    for (j = 0; j < scan_ret; j++) // for all scan results
                    {
                        if (strcmp(profile.ssid, WiFi.SSID(j).c_str()) == 0) // SSID matches
                        {
                            dbg_ser.printf("autoconnect found matching SSID[%u, %u]: %s\r\n", i, j, profile.ssid);
                            result_profile = i;
                            result_code = (strlen(profile.password) > 0) ? AUTOCONNRES_FOUND_EXISTING : AUTOCONNRES_FOUND_EXISTING_NEED_PASSWORD;
                            break;
                        }
                    }
                }
                else if (wifiprofile_isBlank(i))
                {
                    dbg_ser.printf("autoconnect reached end of database at %u\r\n", i);
                    // when WIFIPROFILE_LIMIT is large, we shouldn't waste time with the file system reads
                    break;
                }
            }

            if (result_code == AUTOCONNRES_NONE) // we didn't find anything in our existing database
            {
                dbg_ser.printf("autoconnect no existing database entry\r\n");

                // check if any entry has a SSID that looks like a Sony camera
                for (i = 0; i < scan_ret; i++)
                {
                    char* ssid_str = (char*)WiFi.SSID(i).c_str();
                    if (memcmp("DIRECT-", ssid_str, 7) == 0) // looks like a Sony camera
                    {
                        dbg_ser.printf("autoconnect new SSID: %s\r\n", ssid_str);
                        result_code = AUTOCONNRES_FOUND_NEW;
                        // use the data structure as a cache, we can save it into SPIFFS later quickly
                        strncpy(profile.ssid, ssid_str, WIFI_STRING_LEN);
                        profile.password[0] = 0;
                        profile.opmode = WIFIOPMODE_STA;
                        profile.guid[0] = 0;
                        break;
                    }
                    // TODO: check for compatible cameras only?
                    // TODO: handle multiple new cameras? currently only handles one new entity
                }
            }

            if (result_code == AUTOCONNRES_NONE)
            {
                // nothing found, restart the scan
                dbg_ser.printf("autoconnect re-scan\r\n");
                WiFi.scanNetworks(true, false, false);
            }
            else
            {
                // found something, we can quit this loop
                break;
            }
        }
        else if (scan_ret != WIFI_SCAN_RUNNING)
        {
            // either the scan isn't running or no results were found, (re)start the scan
            dbg_ser.printf("autoconnect scan start\r\n");
            WiFi.scanNetworks(true, false, false);
        }

        // maybe the user wants to cancel
        user_quit |= btnSide_hasPressed() || btnPwr_hasPressed();
        if (user_quit)
        {
            dbg_ser.printf("autoconnect user quit\r\n");
            result_code = AUTOCONNRES_QUIT;
            esp_wifi_scan_stop();
            WiFi.scanDelete();
            btnSide_clrPressed();
            btnPwr_clrPressed();
            break;
        }
    }

    if (user_quit || result_code == AUTOCONNRES_QUIT)
    {
        // user quit, go back to normal
        NetMgr_reset();
        wifiprofile_connect(config_settings.wifi_profile);
        goto all_done_exit;
    }

    // use this buffer as a cache for wifi_pswdPromptDrawCb()
    strncpy(NetMgr_getSSID(), profile.ssid, WIFI_STRING_LEN);

    if (result_code == AUTOCONNRES_FOUND_NEW)
    {
        // if the SSID does not exist in our database, then find a slot for it
        result_profile = 0;
        int k;
        for (k = 1; k <= WIFIPROFILE_LIMIT; k++)
        {
            if (wifiprofile_isBlank(k))
            {
                result_profile = k;
                dbg_ser.printf("autoconnect new camera blank spot %u\r\n", k);
                break;
            }
        }
    }

    if (result_profile != 0)
    {
        bool need_ask = (result_code == AUTOCONNRES_FOUND_EXISTING_NEED_PASSWORD || result_code == AUTOCONNRES_FOUND_NEW); // do not need to ask if the database already has entry
        bool can_save = result_code == AUTOCONNRES_FOUND_NEW;
        user_quit = wifi_newConnectOrPrompt(result_profile, &profile, need_ask, can_save);
        goto all_done_exit;
    }
    else
    {
        // result_profile is zero but the user didn't quit? the only reason should be that the database is now full, but could be another reason
        // the database is YUGE, so... I'm not going to handle this error gracefully
        wifi_err_reason = 0;
        critical_error("/wifi_error.png");
    }

    all_done_exit:
    dbg_ser.printf("autoconnect exiting function\r\n");
    app_waitAllRelease();
    autoconnect_active = false;
    redraw_flag = true;
    return;
}

bool wifi_newConnectOrPrompt(uint8_t profile_num, wifiprofile_t* profile, bool need_ask, bool can_save)
{
    bool user_quit = false;
    while (user_quit == false) // this loop will prompt the user for a password until a successful connection is made (or the user cancels)
    {
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
            if (btnAll_hasPressed())
            {
                // any key press means to not wait for connection
                // no timeout implemented
                btnAll_clrPressed();
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
                    dbg_ser.printf("autoconnect user wants give up\r\n");
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
