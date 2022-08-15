#include "SonyHttpCamera.h"

static const char cmd_generic_fmt[]               = "{\"method\": \"%s\", \"params\": [], \"id\": 1, \"version\": \"1.0\"}";
static const char cmd_generic_strparam_fmt[]      = "{\"method\": \"%s\", \"params\": [\"%s\"], \"id\": 1, \"version\": \"1.0\"}";
static const char cmd_generic_strintparam_fmt[]   = "{\"method\": \"%s\", \"params\": [\"%u\"], \"id\": 1, \"version\": \"1.0\"}";
static const char cmd_generic_intparam_fmt[]      = "{\"method\": \"%s\", \"params\": [%u], \"id\": 1, \"version\": \"1.0\"}";
static const char zoom_cmd_fmt[]                  = "{\"method\": \"actZoom\", \"params\": [\"%s\",\"%s\"], \"id\": 1, \"version\": \"1.0\"}";

void SonyHttpCamera::cmd_prep(void)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    request_prep();
    httpreq->onData(NULL, NULL);
    httpreq->onReadyStateChange(genericRequestCb, this);
    httpreq->open("POST", url_buffer);
}

void SonyHttpCamera::cmd_Shoot(void)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_fmt, "actTakePicture");
    httpreq->send(cmd_buffer);
}

void SonyHttpCamera::cmd_MovieRecord(bool is_start)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_fmt, is_start ? "startMovieRec" : "stopMovieRec");
    httpreq->send(cmd_buffer);
    is_movierecording_v = is_start;
}

void SonyHttpCamera::cmd_MovieRecordToggle(void)
{
    cmd_MovieRecord(is_movierecording_v == false);
}

void SonyHttpCamera::cmd_ZoomStart(int dir)
{
    if (zoom_state == 0)
    {
        if (dir != 0) {
            cmd_prep();
            sprintf(cmd_buffer, zoom_cmd_fmt, dir > 0 ? "in" : "out", "start");
            httpreq->send(cmd_buffer);
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
    sprintf(cmd_buffer, zoom_cmd_fmt, zoom_state > 0 ? "in" : "out", "stop");
    httpreq->send(cmd_buffer);
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
    sprintf(cmd_buffer, "{\"method\": \"setTouchAFPosition\", \"params\": [%0.1f, %0.1f], \"id\": 1, \"version\": \"1.0\"}", x, y);
    httpreq->send(cmd_buffer);
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
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setShutterSpeed", tmp);
    free(tmp);
    httpreq->send(cmd_buffer);
}

void SonyHttpCamera::cmd_IsoSet(uint32_t x)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_strintparam_fmt, "setIsoSpeedRate", x);
    httpreq->send(cmd_buffer);
}

void SonyHttpCamera::cmd_IsoSetStr(char* s)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setIsoSpeedRate", s);
    httpreq->send(cmd_buffer);
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
    sprintf(cmd_buffer, cmd_generic_strparam_fmt, "setFocusMode", onoff ? "MF" : str_afmode);
    httpreq->send(cmd_buffer);
    is_manuallyfocused_v = onoff;
}

void SonyHttpCamera::cmd_ManualFocusToggle(bool onoff)
{
    cmd_ManualFocusMode(onoff == false, false);
}

void SonyHttpCamera::cmd_AutoFocus(bool onoff)
{
    cmd_prep();
    sprintf(cmd_buffer, cmd_generic_fmt, onoff ? "actHalfPressShutter" : "cancelHalfPressShutter");
    httpreq->send(cmd_buffer);
}
