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

bool PtpIpSonyAlphaCamera::cmd_ShutterSpeedSet(int16_t numerator, int16_t denominator)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_ShutterSpeed;
    int16_t data[] = {denominator, numerator};
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceA, &propcode, 1, (uint8_t*)data, sizeof(int16_t) * 2);
}

bool PtpIpSonyAlphaCamera::cmd_ShutterSpeedSet32(uint32_t x)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_ShutterSpeed;
    return send_oper_req((uint32_t)SONYALPHA_OPCODE_SetControlDeviceA, &propcode, 1, (uint8_t*)&x, sizeof(uint32_t));
}

bool PtpIpSonyAlphaCamera::cmd_ShutterSpeedStep(int8_t dir) {
    if (dir == 0) {
        return false;
    }
    if (table_shutter_speed == NULL) {
        return false;
    }
    uint16_t prop_code = SONYALPHA_PROPCODE_ShutterSpeed;
    if (has_property(prop_code) == false) {
        return false;
    }
    uint32_t x = get_property(prop_code); // get current shutter speed
    int32_t i, j;
    uint32_t c = table_shutter_speed[0]; // the count is in the first table element
    // search all elements of the table
    for (i = 1; i <= c; i++)
    {
        uint32_t y = table_shutter_speed[i];
        if (y == x) // matches current shutter speed
        {
            j = i - dir; // find the adjacent entry
            // dir being positive should mean +exposure, but the table is flipped, so we use a subtraction
            // keep within limits of table
            if (j <= 1) {
                j = 1;
            }
            else if (j >= c) {
                j = c;
            }
            return cmd_ShutterSpeedSet32(table_shutter_speed[j]);
        }
    }
    return false;
}

bool PtpIpSonyAlphaCamera::cmd_FocusPointSet(int16_t x, int16_t y)
{
    wait_while_busy(0, DEFAULT_BUSY_TIMEOUT, NULL);
    uint32_t propcode = SONYALPHA_PROPCODE_FocusPointSet;
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

bool PtpIpSonyAlphaCamera::cmd_MovieRecordToggle()
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
