#include "SonyHttpCamera.h"

const char SonyHttpCamera::cmd_generic_fmt[]               = "{\"method\": \"%s\", \"params\": [], \"id\": %u, \"version\": \"1.0\"}";
const char SonyHttpCamera::cmd_generic_strparam_fmt[]      = "{\"method\": \"%s\", \"params\": [\"%s\"], \"id\": %u, \"version\": \"1.0\"}";
const char SonyHttpCamera::cmd_generic_strintparam_fmt[]   = "{\"method\": \"%s\", \"params\": [\"%u\"], \"id\": %u, \"version\": \"1.0\"}";
const char SonyHttpCamera::cmd_generic_intparam_fmt[]      = "{\"method\": \"%s\", \"params\": [%u], \"id\": %u, \"version\": \"1.0\"}";
const char SonyHttpCamera::cmd_zoom_fmt[]                  = "{\"method\": \"actZoom\", \"params\": [\"%s\",\"%s\"], \"id\": %u, \"version\": \"1.0\"}";

void SonyHttpCamera::cmd_prep(void)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    #ifdef SHCAM_USE_ASYNC
    request_prep("POST", service_url, "application/json", genericRequestCb, NULL);
    #endif
}

bool SonyHttpCamera::cmd_send(char* cmd, char* alt_url, bool callend)
{
    #ifdef SHCAM_USE_ASYNC
    httpreq->send(cmd);
    state |= 1;
    return true;
    #else
    dbgser_tx->printf("httpcam cmd %s\r\n", cmd);
    httpclient.begin(alt_url == NULL ? service_url : alt_url);
    httpclient.addHeader("Content-Type", "application/json");
    last_http_resp_code = httpclient.POST(cmd);
    bool success = last_http_resp_code == 200;
    http_content_len = httpclient.getSize();
    if (callend) {
        httpclient.end();
    }
    else {
        state |= 1;
    }
    return success;
    #endif
}

void SonyHttpCamera::cmd_Shoot(void)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_fmt, "actTakePicture", req_id);
    cmd_send(cmd_buffer);
    req_id++;
}

void SonyHttpCamera::cmd_MovieRecord(bool is_start)
{
    #ifdef SHCAM_NEED_ENTER_MOVIE_MODE
    if (is_start && shoot_mode != SHOOTMODE_MOVIE) {
        cmd_MovieMode(true);
    }
    #endif
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_fmt, is_start ? "startMovieRec" : "stopMovieRec", req_id);
    cmd_send(cmd_buffer);
    req_id++;
    is_movierecording_v = is_start;
    #ifdef SHCAM_NEED_ENTER_MOVIE_MODE
    if (is_start == false && shoot_mode == SHOOTMODE_MOVIE) {
        cmd_MovieMode(false);
    }
    #endif
}

void SonyHttpCamera::cmd_MovieRecordToggle(void)
{
    cmd_MovieRecord(is_movierecording_v == false);
}

#ifdef SHCAM_NEED_ENTER_MOVIE_MODE

void SonyHttpCamera::cmd_MovieMode(bool onoff)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setShootMode", onoff ? "movie" : "still", req_id);
    cmd_send(cmd_buffer);
    req_id++;
    shoot_mode = onoff ? SHOOTMODE_MOVIE : SHOOTMODE_STILLS;
}

#endif

void SonyHttpCamera::cmd_ZoomStart(int dir)
{
    if (zoom_state == 0)
    {
        if (dir != 0) {
            cmd_prep();
            sprintf(cmd_buffer, cmd_zoom_fmt, dir > 0 ? "in" : "out", "start", req_id);
            cmd_send(cmd_buffer);
            req_id++;
            zoom_time = millis();
        }
        else {
            cmd_ZoomStop();
        }
        zoom_state = dir;
    }
    else if ((zoom_state > 0 && dir < 0) || (zoom_state < 0 && dir > 0))
    {
        cmd_ZoomStop();
        cmd_ZoomStart(dir);
    }
    else if ((zoom_state * dir) > 0)
    {
        zoom_time = millis();
    }
    else if (dir == 0)
    {
        cmd_ZoomStop();
    }
}

void SonyHttpCamera::cmd_ZoomStop(void)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT * 5, NULL);
    cmd_prep();
    sprintf(cmd_buffer, cmd_zoom_fmt, zoom_state > 0 ? "in" : "out", "stop", req_id);
    cmd_send(cmd_buffer);
    req_id++;
    zoom_state = 0;
    zoom_time = 0;
}

void SonyHttpCamera::cmd_FocusPointSet16(int16_t x, int16_t y)
{
    float xx = x; float yy = y;
    xx *= 100; yy *= 100;
    xx /= 639; yy /= 480;
    cmd_FocusPointSetF(xx, yy);
}

void SonyHttpCamera::cmd_FocusPointSetF(float x, float y)
{
    cmd_prep();
    sprintf(cmd_buffer, "{\"method\": \"setTouchAFPosition\", \"params\": [%0.1f, %0.1f], \"id\": %u, \"version\": \"1.0\"}", x, y, req_id);
    cmd_send(cmd_buffer);
    req_id++;
}

void SonyHttpCamera::cmd_ShutterSpeedSetStr(char* s)
{
    cmd_prep();
    int slen = strlen(s);
    char* tmp = (char*)malloc(strlen(s) + 3);
    strcpy(tmp, s);
    if (s[slen - 1] == '"' && s[slen - 2] != '\\') {
        tmp[slen - 1] = '\\';
        tmp[slen - 0] = '"';
        tmp[slen + 1] = 0;
    }
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setShutterSpeed", tmp, req_id);
    free(tmp);
    cmd_send(cmd_buffer);
    req_id++;
}

void SonyHttpCamera::cmd_IsoSet(uint32_t x)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_strintparam_fmt, "setIsoSpeedRate", x, req_id);
    cmd_send(cmd_buffer);
    req_id++;
}

void SonyHttpCamera::cmd_IsoSetStr(char* s)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setIsoSpeedRate", s, req_id);
    cmd_send(cmd_buffer);
    req_id++;
}

void SonyHttpCamera::cmd_ManualFocusMode(bool onoff, bool precheck)
{
    if (precheck)
    {
        bool already_on = is_manuallyfocused_v;
        if ((already_on && onoff) || (!already_on && !onoff)) {
            return;
        }
    }
    cmd_prep();
    if (strlen(str_afmode) <= 0) {
        sprintf(str_afmode, "AF-C");
    }
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setFocusMode", onoff ? "MF" : str_afmode, req_id);
    cmd_send(cmd_buffer);
    is_manuallyfocused_v = onoff;
}

void SonyHttpCamera::cmd_ManualFocusToggle(bool onoff)
{
    cmd_ManualFocusMode(onoff == false, false);
}

void SonyHttpCamera::cmd_AutoFocus(bool onoff)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_fmt, onoff ? "actHalfPressShutter" : "cancelHalfPressShutter", req_id);
    cmd_send(cmd_buffer);
}
