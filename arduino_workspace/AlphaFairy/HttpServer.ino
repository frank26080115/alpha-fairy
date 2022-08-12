#include "AlphaFairy.h"
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

/*
implement a simple web page interface for the user to change Wi-Fi configuration
*/

// this HTTP server uses
// https://github.com/me-no-dev/ESPAsyncWebServer/
// plenty of documentation and examples

bool http_is_active = false;

#if defined(HTTP_SERVER_ENABLE) || defined(WIFI_ALL_MODES)

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

void add_crossDomainHeaders(AsyncResponseStream* response);

void send_css(AsyncResponseStream* response);
void send_cur_wifi_settings(AsyncResponseStream* response);

static AsyncWebServerRequest* httpsrv_request  = NULL;

void httpsrv_jpgStream(uint8_t* buff, uint32_t len);
void httpsrv_jpgDone(void);
void httpsrv_startJpgStream(AsyncWebServerRequest* request);

const byte DNS_PORT = 53;
DNSServer* dnsServer;
AsyncWebServer* httpServer;

void add_crossDomainHeaders(AsyncResponseStream* response)
{
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
}

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
    #if defined(WIFI_ALL_MODES)
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
    else
    #endif
    {
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
        #ifdef WIFI_ALL_MODES
        wifi_init_ap(true);
        #else
        wifi_init_ap(false);
        #endif
    }

    httpServer = new AsyncWebServer(80);
    dnsServer = new DNSServer();
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

#ifdef WIFI_ALL_MODES
    httpServer->on("/wificonfig", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        // main page, index page
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        // show current settings plus show the form for changing the settings
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
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
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
#endif

    httpServer->on("/", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        if (camera.isOperating()) {
            //response->printf("<html>camera: %s</html>", camera.getCameraName());
            response->printf("<html>camera: %s<br /><img src='/getpreview.jpg?a=b' /></html>", camera.getCameraName());
        }
        else {
            response->printf("<html>no camera</html>");
        }
        request->send(response);
    });

    httpServer->on("/getip", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        response->printf("%s", WiFi.softAPIP().toString().c_str());
        if (camera.isOperating()) {
            response->printf(";\r\n");
            response->printf("%s;\r\n", (IPAddress(camera.getIp())).toString().c_str());
        }
        request->send(response);
    });

    httpServer->on("/getstate", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        response->printf("%u;\r\n", camera.getState());
        if (camera.isOperating())
        {
            response->printf("%s;\r\n", camera.getCameraName());
            int i;
            for (i = 0; i < camera.properties_cnt; i++) {
                ptpipcam_prop_t* p = &(camera.properties[i]);
                response->printf("0x%04X , 0x%02X , %d;\r\n", p->prop_code, p->data_type, p->value);
            }
        }
        request->send(response);
    });

    #if 1
    httpServer->on("/getpreview.jpg", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        //AsyncResponseStream* response = request->beginResponseStream("image/jpeg", 1024 * 8);
        //add_crossDomainHeaders(response);
        httpsrv_startJpgStream(request);
    });
    #endif

    httpServer->on("/cmd", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        if (camera.isOperating() == false) {
            response->printf("disconnected");
            request->send(response);
            return;
        }
        bool ret;
        if (request->hasParam("name") == false)
        {
            AsyncWebParameter* paramName = request->getParam("name");
            if (request->hasParam("value"))
            {
                AsyncWebParameter* paramValue = request->getParam("value");
                int32_t pv = atoi(paramValue->value().c_str());
                if (paramName->value() == "mftoggle") {
                    ret = camera.cmd_ManualFocusToggle(pv != 0);
                }
                else if (paramName->value() == "mfmode") {
                    ret = camera.cmd_ManualFocusMode(pv != 0);
                }
                else if (paramName->value() == "movierec") {
                    ret = camera.cmd_MovieRecord(pv != 0);
                }
                else if (paramName->value() == "movierectog") {
                    ret = camera.cmd_MovieRecordToggle();
                }
                else if (paramName->value() == "shoot") {
                    ret = camera.cmd_Shoot(pv);
                }
                else if (paramName->value() == "shutter") {
                    ret = camera.cmd_Shutter(pv != 0);
                }
                else if (paramName->value() == "af") {
                    ret = camera.cmd_AutoFocus(pv != 0);
                }
                else if (paramName->value() == "mfstep") {
                    ret = camera.cmd_ManualFocusStep(pv);
                }
                else if (paramName->value() == "zoomstep") {
                    ret = camera.cmd_ZoomStep(pv);
                }
                else {
                    response->printf("error;\r\nunknown cmd");
                    request->send(response);
                    return;
                }
                if (ret) {
                    response->printf("success");
                    request->send(response);
                    return;
                }
                else {
                    response->printf("failed");
                    request->send(response);
                    return;
                }
            }
            else
            {
                response->printf("error;\r\nmissing opcode");
            }
            request->send(response);
            return;
        }

        if (request->hasParam("opcode") == false) {
            response->printf("error;\r\nmissing opcode");
            request->send(response);
            return;
        }
        AsyncWebParameter* paramOpcode = request->getParam("propcode");
        if (request->hasParam("opcode") == false) {
            response->printf("error;\r\nmissing propcode");
            request->send(response);
            return;
        }
        AsyncWebParameter* paramPropcode = request->getParam("propcode");
        uint32_t payloadcache;
        uint8_t* payloadptr = NULL;
        int32_t payloadlen = -1;
        if (request->hasParam("payload") && request->hasParam("payloadlen")) {
            AsyncWebParameter* paramPayload = request->getParam("payload");
            AsyncWebParameter* paramPayloadLen = request->getParam("payloadlen");
            payloadcache = atoi(paramPayload->value().c_str());
            payloadlen = atoi(paramPayloadLen->value().c_str());
            payloadptr = (uint8_t*)(&payloadcache);
        }
        else if (request->hasParam("payload"))
        {
            response->printf("error;\r\nmissing payloadlen");
            request->send(response);
            return;
        }
        ret = camera.cmd_arb(atoi(paramOpcode->value().c_str()), atoi(paramPropcode->value().c_str()), payloadptr, payloadlen);
        if (ret) {
            response->printf("success");
            request->send(response);
            return;
        }
        else {
            response->printf("failed");
            request->send(response);
            return;
        }
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

    #if defined(HTTP_SERVER_ENABLE) && defined(HTTP_MOCKBTNS_ENABLE)
    btn_installMockServer();
    #endif

    httpServer->begin();
}

void httpsrv_jpgStream(uint8_t* buff, uint32_t len)
{
    if (httpsrv_request == NULL) {
        return;
    }
    uint32_t now = millis(), t = now;
    while (httpsrv_request->client()->canSend() == false && ((now = millis()) - t) < 1000) {
        yield();
    }
    if (httpsrv_request->client()->canSend()) {
        httpsrv_request->client()->write((const char *)buff, (size_t)len);
    }
}

void httpsrv_jpgDone(void)
{
    if (httpsrv_request != NULL) {
        httpsrv_request->client()->close();
    }
    httpsrv_request = NULL;
}

void httpsrv_startJpgStream(AsyncWebServerRequest* request)
{
    char http_resp[256];
    httpsrv_request  = request;
    int i = sprintf(http_resp, "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: POST, GET, OPTIONS\r\n\r\n");
    httpsrv_request->client()->write((const char *)http_resp, (size_t)i);
    if (camera.isOperating()) {
        camera.get_jpg(httpsrv_jpgStream, httpsrv_jpgDone);
    }
    else {
        httpsrv_jpgDone();
    }
}

void httpsrv_poll()
{
    if (dnsServer != NULL) {
        dnsServer->processNextRequest();
    }
}

void httpsrv_task()
{
    httpsrv_poll();
    ledblink_task();
    NetMgr_task();
    imu.poll();
    cmdline.task();
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    btns_poll();
    #endif
}

#if defined(WIFI_ALL_MODES)

menustate_t menustate_wificonfig;

void wifi_config(void* mip)
{
    bool redraw = true;
    menustate_t* m = &menustate_wificonfig;
    m->idx = 0;
    m->cnt = 5;

    btnAll_clrPressed();
    M5Lcd.fillScreen(TFT_WHITE);

    while (true)
    {
        app_poll();
        pwr_tick();

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

        gui_drawStatusBar(false);

        if (redraw || (m->idx == 4))
        {
            #ifdef QUICK_HTTP_TEST
            m->idx = 4;
            #endif

            M5Lcd.drawPngFile(SPIFFS, "/wificfg_head.png", 0, 0);
            if (redraw) {
                M5Lcd.fillRect(0, 50, M5Lcd.width(), M5Lcd.height() - 16 - 50, TFT_WHITE);
            }
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

#endif  // WIFI_ALL_MODES or HTTP_SERVER_ENABLE
