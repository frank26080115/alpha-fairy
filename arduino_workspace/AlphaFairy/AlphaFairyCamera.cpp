#include "AlphaFairyCamera.h"

uint32_t AlphaFairyCamera::getIp(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->getIp();
    }
    if (cam_http->isOperating()) {
        return cam_http->getIp();
    }
    return 0;
}

char* AlphaFairyCamera::getCameraName(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->getCameraName();
    }
    if (cam_http->isOperating()) {
        return cam_http->getCameraName();
    }
    return NULL;
}

void AlphaFairyCamera::wait_while_busy(uint32_t min_wait, uint32_t max_wait, volatile bool* exit_signal)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->wait_while_busy(min_wait, max_wait, exit_signal);
    }
    if (cam_http->isOperating()) {
        return cam_http->wait_while_busy(min_wait, max_wait, exit_signal);
    }
}

bool AlphaFairyCamera::is_movierecording(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->is_movierecording();
    }
    if (cam_http->isOperating()) {
        return cam_http->is_movierecording();
    }
    return false;
}

bool AlphaFairyCamera::is_manuallyfocused(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->is_manuallyfocused();
    }
    if (cam_http->isOperating()) {
        return cam_http->is_manuallyfocused();
    }
    return false;
}

bool AlphaFairyCamera::is_focused(void)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->is_focused;
    }
    if (cam_http->isOperating()) {
        return cam_http->is_focused;
    }
    return false;
}

bool AlphaFairyCamera::cmd_AutoFocus(bool onoff)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_AutoFocus(onoff);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_AutoFocus(onoff);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_Shutter(bool openclose)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_Shutter(openclose);
    }
    if (openclose && cam_http->isOperating()) {
        cam_http->cmd_Shoot();
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_FocusPointSet(int16_t x, int16_t y)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_FocusPointSet(x, y);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_FocusPointSet16(x, y);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_Shoot(int t)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_Shoot(t);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_Shoot();
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_MovieRecord(bool onoff)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_MovieRecord(onoff);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_MovieRecord(onoff);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_MovieRecordToggle()
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_MovieRecordToggle();
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_MovieRecordToggle();
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ManualFocusMode(bool onoff, bool precheck)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ManualFocusMode(onoff, precheck);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_ManualFocusMode(onoff, precheck);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_ManualFocusToggle(bool onoff)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_ManualFocusToggle(onoff);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_ManualFocusToggle(onoff);
        return true;
    }
    return false;
}

bool AlphaFairyCamera::cmd_IsoSet(uint32_t x)
{
    if (cam_ptp->isOperating()) {
        return cam_ptp->cmd_IsoSet(x);
    }
    if (cam_http->isOperating()) {
        cam_http->cmd_IsoSet(x);
        return true;
    }
    return false;
}

void AlphaFairyCamera::set_debugflags(uint32_t x)
{
    if (cam_ptp != NULL) {
        cam_ptp->set_debugflags(x);
    }
    if (cam_http != NULL) {
        cam_http->set_debugflags(x);
    }
}
