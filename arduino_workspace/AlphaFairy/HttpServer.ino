#include "AlphaFairy.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <FS.h>
#include <SPIFFS.h>

/*
implement a simple web page interface for the user to change Wi-Fi configuration
*/

// this HTTP server uses
// https://github.com/me-no-dev/ESPAsyncWebServer/
// plenty of documentation and examples

bool http_is_active  = false;
bool http_has_client = false;
bool http_has_shown  = false;

#ifdef WIFI_ALL_MODES

void send_css(AsyncResponseStream* response);
void send_cur_wifi_settings(AsyncResponseStream* response);

const byte DNS_PORT = 53;
DNSServer* dnsServer;
AsyncWebServer* httpServer;

void send_css(AsyncResponseStream* response)
{
    response->println("<style>");
    response->println("body {");
    response->println("font-family: monospace;"); // more readable
    response->println("}");
    response->println("td {");
    response->println("margin: 4pt 4pt 4pt 4pt;");
    response->println("padding: 4pt 4pt 4pt 4pt;");
    response->println("width: auto;");
    response->println("}");
    response->println("td.col-left {");
    // minimal left column size
    response->println("width: 1%;");
    response->println("white-space: nowrap;");
    response->println("}");
    response->println("</style>");
}

void send_cur_wifi_settings(AsyncResponseStream* response)
{
    response->print("<table border='1' width='100%'><tr><td class='col-left'>SSID:</td><td class='col-right'>");
    if (strlen(config_settings.wifi_ssid) > 0) {
        response->print(config_settings.wifi_ssid);
        response->print("</td></tr>\r\n<tr><td class='col-left'>Password:</td><td class='col-right'>");
        response->print(config_settings.wifi_pass);
        response->print("</td></tr><tr><td class='col-left'>Mode:</td><td class='col-right'>");
        if (config_settings.wifi_opmode == WIFIOPMODE_STA) {
            response->print("Direct");
        }
        else {
            response->print("Access Point");
        }
        response->print("</td></tr></table></fieldset>");
    }
    else {
        #ifdef WIFI_AP_UNIQUE_NAME
        char wifi_ap_name[64];
        wifi_get_unique_ssid(wifi_ap_name);
        response->print(wifi_ap_name);
        #else
        response->print(WIFI_DEFAULT_SSID);
        #endif
        response->print("</td></tr>\r\n<tr><td class='col-left'>Password:</td><td class='col-right'>");
        response->print(WIFI_DEFAULT_PASS);
        response->print("</td></tr><tr><td class='col-left'>Mode:</td><td class='col-right'>Access Point</td></tr></table>");
    }
}

void httpsrv_init()
{
    if (NetMgr_getOpMode() != WIFIOPMODE_AP)
    {
        // switch mode if required
        wifi_init_ap(true);
    }

    http_is_active = true;

    httpServer = new AsyncWebServer(80);
    dnsServer = new DNSServer();
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

    httpServer->on("/", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        // main page, index page
        // show current settings plus show the form for changing the settings
        http_has_shown = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>Alpha Fairy Wi-Fi Config</title>");
        send_css(response);
        response->print("</head><body><h1>Alpha Fairy Wi-Fi Config</h1><br />\r\n");
        response->print("<fieldset><legend>Current Settings</legend>\r\n");
        send_cur_wifi_settings(response);
        response->print("</fieldset>\r\n");
        response->print("<fieldset><legend>New Settings</legend><form method='post' action='/setwifi'><table border='1' width='100%'>");
        response->print("<tr><td class='col-left'><label for='ssid'>SSID:</lable></td><td class='col-right'>");
        response->print("<input type='text' id='ssid' name='ssid' style='width:80%' value='");
        if (strlen(config_settings.wifi_ssid) > 0) {
            response->print(config_settings.wifi_ssid);
        }
        response->print("' /></td></tr>");
        response->print("<tr><td class='col-left'><label for='password'>Password:</label></td><td class='col-right'>");
        response->print("<input type='text' id='password' name='password' style='width:80%' value='");
        if (strlen(config_settings.wifi_ssid) > 0) {
            response->print(config_settings.wifi_pass);
        }
        response->print("' /></td></tr>");
        response->print("<tr><td class='col-left'><label for='opmode'>Mode:</label></td><td class='col-right'>");
        response->print("<select id='opmode' name='opmode'><option value='ap'>Access Point</option><option value='sta'");
        if (strlen(config_settings.wifi_ssid) > 0 && config_settings.wifi_opmode == WIFIOPMODE_STA) {
            response->print(" selected");
        }
        response->print(">Direct</option></select></td></tr>\r\n");
        response->print("<tr><td colspan='2'><input type='submit' value='Save' style='width:50%' /></td></tr></table></form></fieldset>\r\n");
        response->print("</body></html>\r\n");
        request->send(response);
    });

    httpServer->on("/setwifi", HTTP_POST, [] (AsyncWebServerRequest* request)
    {
        // this is the target of the form
        http_has_shown = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        if (request->hasParam("ssid", true) && request->hasParam("password", true) && request->hasParam("opmode", true)) {
            AsyncWebParameter* paramSsid   = request->getParam("ssid", true);
            AsyncWebParameter* paramPass   = request->getParam("password", true);
            AsyncWebParameter* paramOpMode = request->getParam("opmode", true);
            response = request->beginResponseStream("text/html");
            int32_t opmode = 0;
            if (paramOpMode->value() == "ap" || paramOpMode->value() == "AP") {
                opmode = WIFIOPMODE_AP;
            }
            else if (paramOpMode->value() == "sta" || paramOpMode->value() == "STA") {
                opmode = WIFIOPMODE_STA;
            }
            else {
                response->print("unknown op mode\r\n");
            }
            if (opmode != 0)
            {
                strncpy(config_settings.wifi_ssid, paramSsid->value().c_str(), 30);
                if (strlen(config_settings.wifi_ssid) > 0) {
                    strncpy(config_settings.wifi_pass, paramPass->value().c_str(), 30);
                }
                else {
                    // no password if default SSID is set
                    config_settings.wifi_pass[0] = 0;
                }
                config_settings.wifi_opmode = opmode;
                settings_save();
                response->print("<!DOCTYPE html><html><head><title>Alpha Fairy Wi-Fi Config</title>");
                send_css(response);
                response->print("</head><body><h1>Alpha Fairy Wi-Fi Config</h1><h2>New Settings Saved Successfully</h2><br />\r\n");
                send_cur_wifi_settings(response);
                // show a footer
                response->print("<br />reboot the remote to apply new settings<br /><a href='/'>go back to re-config</a></body></html>\r\n");
            }
        }
        else {
            response = request->beginResponseStream("text/html");
            response->print("missing param(s)\r\n");
        }
        request->send(response);
    });

    httpServer->onNotFound([] (AsyncWebServerRequest* request) {
        #if 0
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>Alpha Fairy Captive Portal</title></head><body>");
        response->print("<p>This is the Alpha Fairy captive portal page</p>");
        response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
        response->printf("<p>Try opening <a href='http://%s/'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
        response->print("</body></html>");
        request->send(response);
        #else
        request->redirect("/");
        #endif
    });

    httpServer->begin();
}

void httpsrv_task()
{
    dnsServer->processNextRequest();
    ledblink_task();
    NetMgr_task();
    imu.poll();
    cmdline.task();
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    btns_poll();
    #endif
}

menustate_t menustate_wificonfig;

void wifi_config(void* mip)
{
    bool redraw = true;
    menustate_t* m = &menustate_wificonfig;
    m->idx = 0;
    m->cnt = 5;

    btnAll_clrPressed();
    M5Lcd.fillScreen(TFT_WHITE);

    httpsrv_init();
    while (true)
    {
        httpsrv_task();

        if (btnSide_hasPressed())
        {
            m->idx = (m->idx + 1) % m->cnt;
            redraw = true;
            btnSide_clrPressed();
        }
        if (btnPwr_hasPressed())
        #if defined(USE_PWR_BTN_AS_BACK) && !defined(USE_PWR_BTN_AS_EXIT)
        {
            m->idx = (m->idx <= 0) ? m->cnt : (m->idx - 1);
            redraw = true;
            btnPwr_clrPressed();
        }
        #else
        {
            ESP.restart();
        }
        #endif

        // note: page index 4 is the current settings, which might change, so it's constantly redrawn without flickering

        if (redraw || (m->idx == 4))
        {
            #ifdef QUICK_HTTP_TEST
            m->idx = 4;
            #endif

            M5Lcd.drawPngFile(SPIFFS, "/wificfg_head.png", 0, 0);
            if (redraw) {
                M5Lcd.fillRect(0, 50, M5Lcd.width(), M5Lcd.height() - 12 - 50, TFT_WHITE);
            }
            gui_drawStatusBar(false);
            if (m->idx == 0 || m->idx == 4)
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
                M5Lcd.println((m->idx == 0) ? "use phone browser" : "current config:");
                M5Lcd.setCursor(left_margin, top_margin + line_space);
                M5Lcd.print("Wi-Fi SSID: ");
                M5Lcd.setCursor(left_margin + 8, top_margin + line_space + (line_space * 1));
                M5Lcd.print((m->idx == 0 || config_settings.wifi_ssid[0] == 0) ? NetMgr_getSSID() : config_settings.wifi_ssid); gui_blankRestOfLine();
                M5Lcd.setCursor(left_margin, top_margin + line_space + (line_space * 2));
                M5Lcd.print("password: ");
                M5Lcd.setCursor(left_margin + 8, top_margin + line_space + (line_space * 3));
                M5Lcd.print((m->idx == 0 || config_settings.wifi_ssid[0] == 0) ? NetMgr_getPassword() : config_settings.wifi_pass); gui_blankRestOfLine();
                M5Lcd.setCursor(left_margin, top_margin + line_space + (line_space * 4));
                M5Lcd.print((m->idx == 0) ? "URL:   " : "mode:   ");
                M5Lcd.setCursor(left_margin + 8, top_margin + line_space + (line_space * 5));
                if (m->idx == 0) {
                    M5Lcd.print("http://");
                    M5Lcd.print(WiFi.softAPIP());
                    M5Lcd.print("/");
                }
                else {
                    if (config_settings.wifi_opmode == 2) {
                        M5Lcd.print("direct          ");
                    }
                    else {
                        M5Lcd.print("access point   ");
                    }
                }
                M5Lcd.setRotation(0);
            }
            else if (m->idx == 1 || m->idx == 2)
            {
                // draw a header quickly first
                M5Lcd.drawPngFile(SPIFFS, m->idx == 1 ? "/wificfg_login.png" : "/wificfg_url.png", 0, 0);
                char qrstr[64];
                uint32_t width = 124;
                uint32_t x = (M5Lcd.width()  - width + 1) / 2;
                uint32_t y = (M5Lcd.height() - width + 1) / 2;
                y += 20;
                // generate the QR code
                if (m->idx == 1) {
                    sprintf(qrstr, "WIFI:T:WPA;S:%s;P:%s;;", NetMgr_getSSID(), NetMgr_getPassword());
                }
                else if (m->idx == 2) {
                    sprintf(qrstr, "http://%s/", WiFi.softAPIP());
                }
                M5Lcd.qrcode(qrstr, x, y, width, 7);
            }
            else if (m->idx == 3)
            {
                M5Lcd.drawPngFile(SPIFFS, "/wificfg_frst.png", 0, 0);
            }

            if (redraw) {
                btnSide_clrPressed();
            }
            redraw = false;
        }

        if (btnBig_hasPressed())
        {
            btnBig_clrPressed();

            if (m->idx == 3) { // factory reset activated
                settings_default();
                settings_save();
                M5Lcd.drawPngFile(SPIFFS, "/wificfg_frstdone.png", 0, 0);
                delay(1000);
                ESP.restart();
            }
        }
        
    }
}

#endif // WIFI_ALL_MODES
