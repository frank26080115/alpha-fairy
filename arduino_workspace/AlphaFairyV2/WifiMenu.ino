#include "AlphaFairy.h"
#include "FairyMenu.h"

class PageHttpInfo : public FairyMenuItem
{
    public:
        PageHttpInfo() : FairyMenuItem("/wificfg_head.png", 0)
        {
        };

        virtual void on_navTo(void)
        {
            _prev_ip = 0;
            FairyMenuItem::on_navTo();
        };

        virtual void on_redraw(void)
        {
            //M5Lcd.fillRect(0, 50, M5Lcd.width(), M5Lcd.height() - 16 - 50, TFT_WHITE);
            M5Lcd.fillScreen(TFT_WHITE);
            FairyMenuItem::on_redraw(); // draw_mainImage();
            draw_text();
        };

        virtual bool check_redraw(void)
        {
            uint32_t cur_ip = (uint32_t)(IPAddress(WiFi.softAPIP()));
            if (cur_ip != _prev_ip)
            {
                _prev_ip = cur_ip;
                return true;
            }
            return false;
        };

    protected:

        uint32_t _prev_ip;

        void draw_text()
        {
            int line_space = 16;
            int top_margin = 7;
            int left_margin = 55;
            M5Lcd.setRotation(1);
            M5Lcd.highlight(true);
            M5Lcd.setTextWrap(true);
            M5Lcd.setHighlightColor(TFT_WHITE);
            M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
            M5Lcd.setCursor(left_margin, top_margin);
            M5Lcd.setTextFont(2);
            M5Lcd.println("use phone browser");
            M5Lcd.setCursor(left_margin, top_margin + line_space);
            M5Lcd.print("Wi-Fi SSID: ");
            M5Lcd.setCursor(left_margin + 8, top_margin + line_space + (line_space * 1));
            M5Lcd.print(NetMgr_getSSID()); gui_blankRestOfLine();
            M5Lcd.setCursor(left_margin, top_margin + line_space + (line_space * 2));
            M5Lcd.print("password: ");
            M5Lcd.setCursor(left_margin + 8, top_margin + line_space + (line_space * 3));
            M5Lcd.print(NetMgr_getPassword()); gui_blankRestOfLine();
            M5Lcd.setCursor(left_margin, top_margin + line_space + (line_space * 4));
            M5Lcd.print("URL:   ");
            M5Lcd.setCursor(left_margin + 8, top_margin + line_space + (line_space * 5));

            M5Lcd.print("http://");
            M5Lcd.print(WiFi.softAPIP());
            M5Lcd.print("/");
            M5Lcd.setRotation(0);
        };
};

class PageWifiQr : public FairyMenuItem
{
    public:
        PageWifiQr(bool is_wifi, const char* img_fname) : FairyMenuItem(img_fname, 0)
        {
            _is_wifi = is_wifi;
        };

        #if 0
        virtual bool can_navTo(void)
        {
            uint32_t cur_ip = (uint32_t)(IPAddress(WiFi.softAPIP()));
            return (cur_ip != 0);
        };
        #endif

        virtual bool check_redraw(void)
        {
            uint32_t cur_ip = (uint32_t)(IPAddress(WiFi.softAPIP()));
            if (cur_ip != _prev_ip)
            {
                _prev_ip = cur_ip;
                return true;
            }
            return false;
        };

        virtual void on_redraw(void)
        {
            FairyMenuItem::on_redraw();
            uint32_t cur_ip = (uint32_t)(IPAddress(WiFi.softAPIP()));
            if (cur_ip == 0) {
                return;
            }
            char qrstr[64];
            uint32_t width = 124;
            uint32_t x = (M5Lcd.width()  - width + 1) / 2;
            uint32_t y = (M5Lcd.height() - width + 1) / 2;
            y += 20;
            if (_is_wifi)
            {
                sprintf(qrstr, "WIFI:T:WPA;S:%s;P:%s;;", NetMgr_getSSID(), NetMgr_getPassword());
            }
            else
            {
                sprintf(qrstr, "http://%s/", WiFi.softAPIP());
            }
            M5Lcd.qrcode(qrstr, x, y, width, 7);
        };

        virtual void on_eachFrame(void)
        {
            pwr_tick(true); // keep light on for QR code
        };

    protected:
        bool _is_wifi;
        uint32_t _prev_ip;
};

class PageWifiSelectProfile : public FairyMenuItem
{
    public:
        PageWifiSelectProfile() : FairyMenuItem("/wificfg_selprofile.png", 0)
        {
        };

        virtual void on_navTo(void)
        {
            _profile_num = config_settings.wifi_profile;
            FairyMenuItem::on_navTo();
        };
        
        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_WHITE);
            FairyMenuItem::on_redraw();
            draw_text();
        };

        virtual void on_spin(int8_t x)
        {
            if (x > 0)
            {
                _profile_num = _profile_num + 1;
                if (wifiprofile_isBlank(_profile_num)) {
                    _profile_num = 0;
                }
            }
            else if (x < 0)
            {
                if (_profile_num <= 0)
                {
                    _profile_num = WIFIPROFILE_LIMIT;
                }
                else {
                    _profile_num -= 1;
                }
                while (wifiprofile_isBlank(_profile_num) && _profile_num > 0) {
                    _profile_num--;
                }
            }
        };

        virtual bool on_execute(void)
        {
            // press button to save, hold button to save-and-reboot
            config_settings.wifi_profile = _profile_num;
            settings_save();
            M5Lcd.drawPngFile(SPIFFS, "/wificfg_profilesave.png", 95, 53);
            uint32_t now, t = millis();
            while (((now = millis()) - t) < 2000 && btnBig_isPressed()) {
                app_poll();
                pwr_tick(true);
            }
            if (btnBig_isPressed())
            {
                esp_wifi_disconnect();
                esp_wifi_stop();
                esp_wifi_deinit();
                srand(t + lroundf(imu.accX) + lroundf(imu.accY) + lroundf(imu.accZ));
                while (btnBig_isPressed())
                {
                    int x = rand() % M5Lcd.width();
                    int y = rand() % M5Lcd.height();
                    M5Lcd.fillRect(x, y, 1, 1, TFT_WHITE);
                }
                ESP.restart();
            }
            return false;
        };

    protected:
        uint8_t _profile_num;
        void draw_text(void)
        {
            M5Lcd.setRotation(0);
            M5Lcd.highlight(true);
            M5Lcd.setTextWrap(true);
            M5Lcd.setHighlightColor(TFT_WHITE);
            M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
            M5Lcd.setTextFont(4);
            int top_margin = 94;
            int left_margin = 5;
            M5Lcd.setCursor((M5Lcd.width() / 2) - 5, top_margin);
            M5Lcd.println(_profile_num, DEC);
            M5Lcd.setCursor(5, M5Lcd.getCursorY());
            M5Lcd.setTextFont(2);
            wifiprofile_t wifi_profile;
            wifiprofile_getProfile(_profile_num, &wifi_profile);
            char* ssid_str = wifi_profile.ssid;
            if (memcmp("DIRECT-", ssid_str, 7) == 0) {
                ssid_str += 4;
                ssid_str[0] = '.';
                ssid_str[1] = '.';
                ssid_str[2] = '.';
            }
            M5Lcd.println(ssid_str);
        };
};

class PageFactoryReset : public FairyMenuItem
{
    public:
        PageFactoryReset() : FairyMenuItem("/wificfg_frst.png", 0)
        {
        };

        virtual bool on_execute(void)
        {
            settings_default();
            settings_save();
            wifiprofile_deleteAll();
            M5Lcd.drawPngFile(SPIFFS, "/wificfg_frstdone.png", 0, 0);
            delay(2000);
            app_waitAllRelease();
            ESP.restart();
            return false;
        };
};

class PageWifiInfo : public FairyMenuItem
{
    public:
        PageWifiInfo(bool show_cam) : FairyMenuItem("/wifiinfo_head.png", 0)
        {
            _show_cam = show_cam;
        };

        virtual void on_navTo(void)
        {
            _show_rssi = false;
            _prev_connected = fairycam.isOperating();
            FairyMenuItem::on_navTo();
        };

        virtual bool can_navTo(void)
        {
            if (_show_cam && fairycam.isOperating() == false) {
                return false;
            }
            return true;
        };

        virtual bool check_redraw(void)
        {
            bool connected = fairycam.isOperating();
            if (connected != _prev_connected)
            {
                _prev_connected = connected;
                return true;
            }
            return false;
        };

        virtual void on_redraw(void)
        {
            M5Lcd.fillScreen(TFT_WHITE);
            FairyMenuItem::on_redraw();

            //M5Lcd.setRotation(0);
            //M5Lcd.fillRect(0, 50, M5Lcd.width(), M5Lcd.height() - 16 - 50, TFT_WHITE);

            draw_text();
        };

        virtual void on_eachFrame(void)
        {
            int16_t pitch = imu.getPitch();
            if (pitch > 60 || pitch < -60 || _show_rssi) // show RSSI only if the device is at an upside down angle
            {
                int rssi;
                if (NetMgr_getRssi(fairycam.getIp(), &rssi)) {
                    M5Lcd.setRotation(1);
                    M5Lcd.setTextFont(2);
                    M5Lcd.setCursor(_left_margin, _top_margin);
                    M5Lcd.printf("Wi-Fi SSID: (RSSI %d)", rssi); gui_blankRestOfLine();
                    M5Lcd.setRotation(0);
                    _show_rssi = true;
                }
            }
        };

    protected:
        bool _show_cam;
        bool _show_rssi;
        bool _prev_connected;

        const int _line_space  = 16;
        const int _top_margin  = 7;
        const int _left_margin = 55;

        void draw_text(void)
        {
            M5Lcd.setRotation(1);
            M5Lcd.highlight(true);
            M5Lcd.setTextWrap(true);
            M5Lcd.setHighlightColor(TFT_WHITE);
            M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
            M5Lcd.setCursor(_left_margin, _top_margin);
            M5Lcd.setTextFont(2);

            if (_show_rssi == false) { // already overwritten, prevent collide
                M5Lcd.print("Wi-Fi SSID:"); gui_blankRestOfLine();
            }
            M5Lcd.setCursor(_left_margin + 8, _top_margin + (_line_space * 1));
            M5Lcd.print(NetMgr_getSSID()); gui_blankRestOfLine();
            M5Lcd.setCursor(_left_margin, _top_margin + (_line_space * 2));
            M5Lcd.print("password: ");
            M5Lcd.setCursor(_left_margin + 8, _top_margin + (_line_space * 3));
            M5Lcd.print(NetMgr_getPassword()); gui_blankRestOfLine();
            M5Lcd.setCursor(_left_margin, _top_margin + (_line_space * 4));

            if (_show_cam && fairycam.isOperating())
            {
                M5Lcd.print("Camera: ");
                M5Lcd.setCursor(_left_margin + 8, _top_margin + (_line_space * 5));
                M5Lcd.print(IPAddress(fairycam.getIp()));
                char* cam_name = fairycam.getCameraName();
                if (cam_name != NULL && strlen(cam_name) > 0) {
                    M5Lcd.setCursor(_left_margin + 8, _top_margin + (_line_space * 6));
                    M5Lcd.print(cam_name);
                }
            }
            else
            {
                M5Lcd.print("URL: ");
                M5Lcd.setCursor(_left_margin + 8, _top_margin + (_line_space * 5));
                M5Lcd.print("http://");
                M5Lcd.print(WiFi.softAPIP());
                M5Lcd.print("/");
            }

            M5Lcd.setRotation(0);
        };
};

class AppWifiConfig : public FairySubmenu
{
    public:
        AppWifiConfig() : FairySubmenu("/wifi_config.png", MENUITEM_WIFICONFIG)
        {
            install(new PageHttpInfo());
            install(new PageWifiQr(true , "/wificfg_login.png"));
            install(new PageWifiQr(false, "/wificfg_url.png"));
            install(new PageWifiSelectProfile());
            install(new PageFactoryReset());
            _already_running = false;
        };

        virtual bool on_execute(void)
        {
            bool ret = false;
            if (_already_running == false)
            {
                _already_running = true;
                if (NetMgr_getOpMode() == WIFIOPMODE_STA) {
                    wifiprofile_connect(0);
                }
                httpsrv_init();
                rewind();
                ret = FairySubmenu::on_execute();
                _already_running = false;
            }
            return ret;
        };

    protected:
        bool _already_running;
};

class AppWifiInfo : public FairySubmenu
{
    public:
        AppWifiInfo() : FairySubmenu("/wifi_config.png", MENUITEM_WIFICONFIG)
        {
            install(new PageWifiInfo(false));
            install(new PageWifiInfo(true));
            install(new PageWifiQr(true , "/wifiinfo_login.png"));
            install(new PageWifiQr(false, "/wifiinfo_url.png"));
        };
};

extern FairySubmenu menu_utils;
AppWifiConfig app_wificfg;
AppWifiInfo   app_wifinfo;
void setup_wifimenus()
{
    menu_utils.install(&app_wificfg);
    menu_utils.install(&app_wifinfo);
}

void run_wifi_cfg()
{
    app_wificfg.on_execute();
}
