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

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

void add_crossDomainHeaders(AsyncResponseStream* response);

void send_css(AsyncResponseStream* response);
bool send_wifi_settings(AsyncResponseStream* response, uint8_t idx, bool readonly);

static AsyncWebServerRequest* httpsrv_request  = NULL;

#if 0
void httpsrv_jpgStream(uint8_t* buff, uint32_t len);
void httpsrv_jpgDone(void);
void httpsrv_startJpgStream(AsyncWebServerRequest* request);
#endif

const byte DNS_PORT = 53;
DNSServer* dnsServer;
AsyncWebServer* httpServer = NULL;

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

bool send_wifi_settings(AsyncResponseStream* response, uint8_t idx, bool readonly)
{
    if (idx == 0) {
        readonly = true;
    }
    wifiprofile_t profile;
    bool ret = true;
    bool has_profile = wifiprofile_getProfile(idx, &profile);
    response->printf("<fieldset><legend>Wi-Fi Profile #%u%s</legend>\r\n", idx, has_profile ? "" : " (new)");
    if (readonly == false) {
        response->printf("<form action='/setwifi' method='post'><input id='profilenum' name='profilenum' type='hidden' value='%u' />\r\n", idx);
    }
    response->print("<table border='1' width='100%'>\r\n");

    response->printf("<tr><td class='col-left'><label for='ssid%u'>SSID:</label></td><td class='col-right'>", idx);
    char* sp;
    sp = profile.ssid;
    if (has_profile == false) {
        sp[0] = 0;
        ret = false;
    }
    if (readonly) {
        if (sp[0] == 0) {
            response->print("&nbsp;");
        }
        else {
            response->print(sp);
        }
    }
    else {
        response->printf("<input type='text' id='ssid%u' name='ssid' style='width:80%%' placeholder='%sWi-Fi SSID' maxlength='%u' value='%s' />", idx, has_profile ? "" : "New ", WIFI_STRING_LEN, sp);
    }
    response->print("</td></tr>\r\n");

    response->printf("<tr><td class='col-left'><label for='password%u'>Password:</label></td><td class='col-right'>", idx);
    sp = profile.password;
    if (has_profile == false || profile.ssid[0] == 0) {
        sp[0] = 0;
        ret = false;
    }
    if (readonly) {
        if (sp[0] == 0) {
            response->print("&nbsp;");
        }
        else {
            response->print(sp);
        }
    }
    else {
        response->printf("<input type='text' id='password%u' name='password' style='width:80%%' placeholder='%sWi-Fi password' maxlength='%u' value='%s' />", idx, has_profile ? "" : "New ", WIFI_STRING_LEN, sp);
    }
    response->print("</td></tr>\r\n");

    response->printf("<tr><td class='col-left'><label for='opmode%u'>Mode:</label></td><td class='col-right'>", idx);
    if (readonly) {
        if (profile.opmode == WIFIOPMODE_STA) {
            response->print("Direct");
        }
        else {
            response->print("Access Point");
        }
    }
    else {
        response->printf("<select id='opmode%u' name='opmode'><option value='ap'>Access Point</option><option value='sta'", idx);
        if (has_profile && profile.opmode == 2) {
        response->print(" selected");
        }
        response->print(">Direct</option></select>");
    }
    response->print("</td></tr>\r\n");

    response->printf("<tr><td class='col-left'><label for='guid%u'>GUID:</label></td><td class='col-right'>", idx);
    sp = profile.guid;
    if (has_profile == false || profile.ssid[0] == 0) {
        sp[0] = 0;
    }
    if (readonly) {
        if (sp[0] == 0) {
            response->print("&nbsp;");
        }
        else {
            response->print(sp);
        }
    }
    else {
        response->printf("<input type='text' id='guid%u' name='guid' style='width:80%%' maxlength='16' placeholder='GUID for PTP mode' value='%s' />", idx, sp);
    }
    response->print("</td></tr>\r\n");

    if (readonly == false) {
        response->printf("<tr><td colspan='2'><input type='submit' value='Save Profile #%u' style='min-width:50%%' /></td></tr></table></form>", idx);
    }
    else {
        response->printf("</table>", idx);
    }
    response->print("</fieldset>\r\n");
    return ret;
}

void httpsrv_init()
{
    if (httpServer != NULL) {
        return;
    }

    httpServer = new AsyncWebServer(80);
    dnsServer = new DNSServer();
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

    httpServer->on("/wificonfig", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        dbg_ser.println("http server wificonfig");

        // main page, index page
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        // show current settings plus show the form for changing the settings
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        response->print("<!DOCTYPE html><html><head><title>Alpha Fairy Wi-Fi Config</title>");
        send_css(response);
        response->print("</head><body><h1>Alpha Fairy Wi-Fi Config</h1><br />\r\n");

        int i;
        for (i = 0; i <= WIFIPROFILE_LIMIT; i++) {
            bool did = send_wifi_settings(response, i, false);
            response->print("<br />\r\n");
            if (did == false) {
                // this will only leave one blank entry
                break;
            }
        }

        response->print("</body></html>\r\n");
        request->send(response);
    });

    httpServer->on("/setwifi", HTTP_POST, [] (AsyncWebServerRequest* request)
    {
        dbg_ser.println("http server setwifi");

        // this is the target of the form
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        if (request->hasParam("profilenum", true) && request->hasParam("ssid", true) && request->hasParam("password", true) && request->hasParam("opmode", true)) {
            AsyncWebParameter* paramNum    = request->getParam("profilenum", true);
            AsyncWebParameter* paramSsid   = request->getParam("ssid"      , true);
            AsyncWebParameter* paramPass   = request->getParam("password"  , true);
            AsyncWebParameter* paramOpMode = request->getParam("opmode"    , true);
            AsyncWebParameter* paramGuid = NULL;
            if (request->hasParam("guid", true)) {
                paramGuid = request->getParam("guid", true);
            }

            response = request->beginResponseStream("text/html");
            int pnum = atoi(paramNum->value().c_str());

            if (pnum < 1 || pnum > WIFIPROFILE_LIMIT) {
                response->print("bad value for profile number\r\n");
                request->send(response);
                return;
            }

            wifiprofile_t profile;

            profile.opmode = 0;
            if (paramOpMode->value() == "ap" || paramOpMode->value() == "AP") {
                profile.opmode = WIFIOPMODE_AP;
            }
            else if (paramOpMode->value() == "sta" || paramOpMode->value() == "STA") {
                profile.opmode = WIFIOPMODE_STA;
            }
            else {
                response->print("unknown op mode\r\n");
                request->send(response);
                return;
            }

            response->print("<!DOCTYPE html><html><head><title>Alpha Fairy Wi-Fi Config</title>");
            send_css(response);
            response->print("</head><body><h1>Alpha Fairy Wi-Fi Config</h1>\r\n");

            strncpy(profile.ssid, paramSsid->value().c_str(), WIFI_STRING_LEN);
            if (strlen(profile.ssid) > 0)
            {
                strncpy(profile.password, paramPass->value().c_str(), WIFI_STRING_LEN);

                if (paramGuid != NULL) {
                    strncpy(profile.guid, paramGuid->value().c_str(), PTP_GUID_LEN);
                }
                else {
                    profile.guid[0] = 0;
                }

                wifiprofile_writeProfile(pnum, &profile);

                response->printf("<h2>Profile #%u Saved Successfully</h2><br />\r\n", pnum);
                send_wifi_settings(response, pnum, true);
            }
            else
            {
                wifiprofile_deleteProfile(pnum);
                response->printf("<h2>Profile #%u Deleted Successfully</h2><br />\r\n", pnum);
            }
            // show a footer
            response->print("<br />reboot the remote to apply new settings<br /><a href='/wificonfig'>go back to re-config</a></body></html>\r\n");
            redraw_flag = true;
        }
        else {
            response = request->beginResponseStream("text/html");
            response->print("missing param(s)\r\n");
        }
        request->send(response);
    });

    httpServer->on("/", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        #if 0
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        if (ptpcam.isOperating()) {
            response->printf("<html>camera: %s</html>", ptpcam.getCameraName());
            //response->printf("<html>camera: %s<br /><img src='/getpreview.jpg?a=b' /></html>", ptpcam.getCameraName());
        }
        else {
            response->printf("<html>no camera</html>");
        }
        request->send(response);
        #else
        dbg_ser.println("http server root redirect");
        request->redirect("/wificonfig");
        #endif
    });

    #if defined(HTTP_ENABLE_CMD_INTEFACE) && defined(HTTP_ON_BOOT)
    httpServer->on("/getip", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        response->printf("%s", WiFi.softAPIP().toString().c_str());
        if (ptpcam.isOperating()) {
            response->printf(";\r\n");
            response->printf("%s;\r\n", (IPAddress(ptpcam.getIp())).toString().c_str());
        }
        request->send(response);
    });
    #endif

    #if defined(HTTP_ENABLE_CMD_INTEFACE) && defined(HTTP_ON_BOOT)
    httpServer->on("/getstate", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        response->printf("%u;\r\n", ptpcam.getState());
        if (ptpcam.isOperating())
        {
            response->printf("%s;\r\n", ptpcam.getCameraName());
            int i;
            for (i = 0; i < ptpcam.properties_cnt; i++) {
                ptpipcam_prop_t* p = &(ptpcam.properties[i]);
                response->printf("0x%04X , 0x%02X , %d;\r\n", p->prop_code, p->data_type, p->value);
            }
        }
        request->send(response);
    });
    #endif

    #if 0
    #if defined(HTTP_ENABLE_CMD_INTEFACE) && defined(HTTP_ON_BOOT)
    httpServer->on("/getpreview.jpg", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        if (httpcam.isOperating() && httpcam.getLiveviewUrl() != NULL) {
            request->redirect(httpcam.getLiveviewUrl());
            return;
        }
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        //AsyncResponseStream* response = request->beginResponseStream("image/jpeg", 1024 * 8);
        //add_crossDomainHeaders(response);
        httpsrv_startJpgStream(request);
    });
    #endif
    #endif

    #if defined(HTTP_ENABLE_CMD_INTEFACE) && defined(HTTP_ON_BOOT)
    httpServer->on("/cmd", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        NetMgr_markClientPhoneHttp(request->client()->getRemoteAddress());
        http_is_active = true;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        add_crossDomainHeaders(response);
        if (ptpcam.isOperating() == false) {
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
                    ret = ptpcam.cmd_ManualFocusToggle(pv != 0);
                }
                else if (paramName->value() == "mfmode") {
                    ret = ptpcam.cmd_ManualFocusMode(pv != 0);
                }
                else if (paramName->value() == "movierec") {
                    ret = ptpcam.cmd_MovieRecord(pv != 0);
                }
                else if (paramName->value() == "movierectog") {
                    ret = ptpcam.cmd_MovieRecordToggle();
                }
                else if (paramName->value() == "shoot") {
                    ret = ptpcam.cmd_Shoot(pv);
                }
                else if (paramName->value() == "shutter") {
                    ret = ptpcam.cmd_Shutter(pv != 0);
                }
                else if (paramName->value() == "af") {
                    ret = ptpcam.cmd_AutoFocus(pv != 0);
                }
                else if (paramName->value() == "mfstep") {
                    ret = ptpcam.cmd_ManualFocusStep(pv);
                }
                else if (paramName->value() == "zoomstep") {
                    ret = ptpcam.cmd_ZoomStep(pv);
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
        ret = ptpcam.cmd_arb(atoi(paramOpcode->value().c_str()), atoi(paramPropcode->value().c_str()), payloadptr, payloadlen);
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
    #endif

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
        dbg_ser.println("http server 404 redirect");
        request->redirect("/");
        #endif
    });

    #if defined(HTTP_MOCKBTNS_ENABLE)
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

#if 0
void httpsrv_startJpgStream(AsyncWebServerRequest* request)
{

    char http_resp[256];
    httpsrv_request  = request;
    int i = sprintf(http_resp, "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: POST, GET, OPTIONS\r\n\r\n");
    httpsrv_request->client()->write((const char *)http_resp, (size_t)i);
    if (ptpcam.isOperating()) {
        #ifdef PTPIP_ENABLE_STREAMING
        ptpcam.get_jpg(httpsrv_jpgStream, httpsrv_jpgDone);
        #endif
    }
    else {
        httpsrv_jpgDone();
    }
}
#endif

void httpsrv_poll()
{
    if (dnsServer != NULL) {
        dnsServer->processNextRequest();
    }
}
