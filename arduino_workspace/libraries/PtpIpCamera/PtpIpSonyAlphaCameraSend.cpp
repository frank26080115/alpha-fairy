#include "PtpIpSonyAlphaCamera.h"
#include <Arduino.h>
#include "ptpip_utils.h"

bool PtpIpSonyAlphaCamera::cmd_AutoFocus(bool onoff)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_AutoFocus;
    uint8_t data[] = { onoff ? 2 : 1, 0 };
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceB, &propcode, 1, (uint8_t*)data, 2);
}

bool PtpIpSonyAlphaCamera::cmd_Shutter(bool openclose)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_Capture;
    uint8_t data[] = { openclose ? 2 : 1, 0 };
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceB, &propcode, 1, (uint8_t*)data, 2);
}

bool PtpIpSonyAlphaCamera::cmd_ManualFocusStep(int16_t step)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_ManualFocusStep;
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceB, &propcode, 1, (uint8_t*)&step, 2);
}

bool PtpIpSonyAlphaCamera::cmd_FocusPointSet(int16_t x, int16_t y)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_ManualFocusStep;
    int16_t data[] = {y, x};
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceB, &propcode, 1, (uint8_t*)data, sizeof(int16_t) * 2);
}

bool PtpIpSonyAlphaCamera::cmd_Shoot(int t) {
    if (t <= 0) {
        t = SONYCAM_DEFAULT_SHUTTER_TIME;
    }
    bool success = false;
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    success |= cmd_Shutter(true);
    // if continuous shooting is on, then don't wait too long here
    // if bulb mode is on, then this is the shutter speed
    wait_while_busy(t, DEFAULT_BUSY_TIMEOUT, NULL);
    success |= cmd_Shutter(false);
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    return success;
}

bool PtpIpSonyAlphaCamera::cmd_MovieRecord(bool onoff)
{
    bool already_on = is_movierecording();
    if ((already_on && onoff) || (!already_on && !onoff)) {
        return true;
    }
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_Movie;
    uint8_t data[] = { onoff ? 2 : 1, 0 };
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceB, &propcode, 1, (uint8_t*)data, 2);
}

bool PtpIpSonyAlphaCamera::cmd_MovieRecordToggle(bool onoff)
{
    bool already_on = is_movierecording();
    return cmd_MovieRecord(!already_on);
}

bool PtpIpSonyAlphaCamera::cmd_ManualFocusMode(bool onoff)
{
    bool already_on = is_manuallyfocused();
    if ((already_on && onoff) || (!already_on && !onoff)) {
        return true;
    }
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_ManualFocusMode;
    uint8_t data[] = { onoff ? 2 : 1, 0 };
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceB, &propcode, 1, (uint8_t*)data, 2);
}

bool PtpIpSonyAlphaCamera::cmd_ManualFocusToggle(bool onoff)
{
    bool already_on = is_manuallyfocused();
    return cmd_ManualFocusMode(!already_on);
}
