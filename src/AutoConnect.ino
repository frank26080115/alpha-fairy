#include "AlphaFairy.h"
#include <FairyKeyboard.h>

bool autoconnect_active = false;
int  autoconnect_status = 0;

extern bool airplane_mode;

void autoconnect_poll()
{
    yield();
    cpufreq_boost();
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
    btnPwr_quickPoll();
}

class AppAutoConnect : public FairyMenuItem
{
    public:
        AppAutoConnect() : FairyMenuItem("/main_auto.png")
        {
        };

        // hide this item when a camera is actually connected
        virtual bool can_navTo(void)
        {
            if (fairycam.isOperating()) {
                return false;
            }
            if (airplane_mode) {
                return false;
            }
            return FairyMenuItem::can_navTo();
        };

        virtual bool on_execute(void)
        {
            autoconnect_active = true;
            autoconnect_status = 0;

            uint8_t scan_failed_cnt = 0;
            uint8_t result_code = 0;
            uint8_t result_profile = 0;
            wifiprofile_t profile;

            bool first_loop = true;
            bool user_quit = false;

            int draw_idx = 0;
            uint32_t t = millis();
            uint32_t now = t;

            WiFi.mode(WIFI_STA);
            WiFi.disconnect(); // halt wifi activity for now

            btnAny_clrPressed();

            dbg_ser.println("autoconnect starting");

            int scan_ret;

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

                if (first_loop) {
                    scan_ret = 0;
                    first_loop = false;
                }
                else if (scan_ret <= 0) {
                    scan_ret = WiFi.scanComplete();
                }

                if (scan_ret > 0) // has results
                {
                    scan_failed_cnt = 0;
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
                            if (strlen(ssid_str) > 0) {
                                dbg_ser.printf(" [%u]scanned SSID: %s\r\n", i, ssid_str);
                            }
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
                        scan_ret = WiFi.scanNetworks(true, false, false);
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
                    dbg_ser.printf("autoconnect scan start (code %d)\r\n", scan_ret);
                    if (scan_ret == WIFI_SCAN_FAILED) {
                        scan_failed_cnt++;
                    }
                    scan_ret = WiFi.scanNetworks(true, false, false);
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

                if (scan_failed_cnt > 5) {
                    critical_error("/wifi_error.png");
                }
            }

            WiFi.scanDelete();

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

            if (result_code >= AUTOCONNRES_FOUND_EXISTING && user_quit == false)
            {
                FairySubmenu* p = dynamic_cast<FairySubmenu*>((FairySubmenu*)get_parent());
                p->rewind();
                if (result_profile != config_settings.wifi_profile) {
                    config_settings.wifi_profile = result_profile;
                    dbg_ser.printf("autoconnect saving profile %u\r\n", result_profile);
                    settings_save();
                }
            }

            dbg_ser.printf("autoconnect exiting function\r\n");
            app_waitAllRelease();
            autoconnect_active = false;
            redraw_flag = true;

            return false;
        };
};

extern FairySubmenu main_menu;
void setup_autoconnect()
{
    static AppAutoConnect app;
    main_menu.install(&app);
}
