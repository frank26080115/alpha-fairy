#include "AlphaFairy.h"

speed_t dual_shutter_next      = { 0, 0, "" };
speed_t dual_shutter_iso       = { 0, 0, "" };
speed_t dual_shutter_last_tv   = { 0, 0, "" };
speed_t dual_shutter_last_iso  = { 0, 0, "" };

void dual_shutter(void* mip)
{
    dbg_ser.println("dual_shutter");

    if (fairycam.isOperating() == false) {
        // show user that the camera isn't connected
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

    ledblink_setMode(LEDMODE_OFF);

    menuitem_t* menuitm = (menuitem_t*)mip;

    if (menuitm->id == MENUITEM_DUALSHUTTER_REG)
    {
        bool gotdata = false;
        if (ptpcam.isOperating())
        {
            if (ptpcam.has_property(SONYALPHA_PROPCODE_ShutterSpeed) && ptpcam.has_property(SONYALPHA_PROPCODE_ISO))
            {
                dual_shutter_next.flags = SPEEDTYPE_PTP;
                dual_shutter_iso.flags  = SPEEDTYPE_PTP;
                dual_shutter_next.u32 = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
                dual_shutter_iso.u32  = ptpcam.get_property(SONYALPHA_PROPCODE_ISO);
                dbg_ser.printf("dualshutter 0x%08X %u\r\n", dual_shutter_next, dual_shutter_iso);
                gotdata = true;
            }
        }
        else if (httpcam.isOperating())
        {
            if (strlen(httpcam.get_shutterspd_str()) > 0 && strlen(httpcam.get_iso_str()) > 0)
            {
                dual_shutter_next.flags = SPEEDTYPE_HTTP;
                dual_shutter_iso.flags  = SPEEDTYPE_HTTP;
                strcpy(dual_shutter_next.str, httpcam.get_shutterspd_str());
                strcpy(dual_shutter_iso.str,  httpcam.get_iso_str());
                gotdata = true;
            }
        }

        if (gotdata == false) {
            dbg_ser.println("dualshutter no data from camera");
        }

        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }

    // if (menuitm->id == MENUITEM_DUALSHUTTER_SHOOT)
    gui_drawTopThickLine(8, TFT_RED);
    dual_shutter_shoot(false, true, &dual_shutter_last_tv, &dual_shutter_last_iso);
    gui_drawTopThickLine(8, TFT_WHITE);

    app_waitAllRelease(BTN_DEBOUNCE);
}

void dual_shutter_shoot(bool already_focused, bool read_button, speed_t* restore_shutter, speed_t* restore_iso)
{
    bool starting_mf = false;
    bool need_restore_af = false;
    bool need_restore_ss = false;
    bool need_restore_iso = false;

    if (already_focused == false) {
        // only care about MF if we are not already focused
        starting_mf = fairycam.is_manuallyfocused();
    }

    if (already_focused == false && starting_mf == false) {
        // the camera won't actually take a photo if it's not focused (when shutter tries to open, it'll lag to focus)
        // so we turn on AF
        fairycam.cmd_AutoFocus(true);
        need_restore_af = true;
    }

    fairycam.cmd_Shutter(true);

    uint32_t t = millis(), now = t;
    uint32_t shutter_ms = 500;

    if (ptpcam.isOperating() && restore_shutter != NULL && restore_shutter->flags == SPEEDTYPE_PTP) {
        shutter_ms = shutter_to_millis(restore_shutter->u32 == 0 ? ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed) : restore_shutter->u32);
    }
    else if (httpcam.isOperating() && restore_shutter != NULL && restore_shutter->flags == SPEEDTYPE_HTTP) {
        shutter_ms = shutter_to_millis(parse_shutter_speed_str(restore_shutter->str));
    }

    // first wait is for minimum press time
    while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
        ptpcam.poll();
        httpcam.poll();
    }

    // release button and wait for the rest of the time
    fairycam.cmd_Shutter(false);

    while (((now = millis()) - t) < shutter_ms && fairycam.isOperating()) {
        ptpcam.poll();
        httpcam.poll();
        if (read_button) {
            if (btnBig_isPressed() == false) {
                break;
            }
        }
    }

    // opportunity for early pause
    if (read_button && btnBig_isPressed() == false) {
        goto last_step;
    }

    // set the shutter speed for second shot
    need_restore_ss = true;
    if (ptpcam.isOperating() && dual_shutter_next.flags == SPEEDTYPE_PTP) {
        ptpcam.cmd_ShutterSpeedSet32(dual_shutter_next.u32);
    }
    else if (httpcam.isOperating() && dual_shutter_next.flags == SPEEDTYPE_HTTP) {
        httpcam.cmd_ShutterSpeedSetStr(dual_shutter_next.str);
    }

    fairycam.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

    // change ISO if required
    if (ptpcam.isOperating() && restore_iso != NULL && dual_shutter_iso.flags == SPEEDTYPE_PTP && restore_iso->flags == SPEEDTYPE_PTP && dual_shutter_iso.u32 != restore_iso->u32) {
        need_restore_iso = true;
        ptpcam.cmd_IsoSet(dual_shutter_iso.u32);
    }
    else if (httpcam.isOperating() && restore_iso != NULL && dual_shutter_iso.flags == SPEEDTYPE_HTTP && restore_iso->flags == SPEEDTYPE_HTTP && strcmp(dual_shutter_iso.str, restore_iso->str) != 0) {
        need_restore_iso = true;
        httpcam.cmd_IsoSetStr(dual_shutter_iso.str);
    }
    if (need_restore_iso) {
        fairycam.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
    }

    // start second shot
    fairycam.cmd_Shutter(true);
    t = millis(); now = t;

    if (ptpcam.isOperating() && dual_shutter_next.flags == SPEEDTYPE_PTP) {
        shutter_ms = shutter_to_millis(dual_shutter_next.u32 == 0 ? ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed) : dual_shutter_next.u32);
    }
    else if (httpcam.isOperating() && dual_shutter_next.flags == SPEEDTYPE_HTTP) {
        shutter_ms = shutter_to_millis(parse_shutter_speed_str(dual_shutter_next.str));
    }

    while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
        ptpcam.poll();
        httpcam.poll();
    }
    fairycam.cmd_Shutter(false);
    while (((now = millis()) - t) < shutter_ms && ptpcam.isOperating()) {
        ptpcam.poll();
        httpcam.poll();
        if (read_button) {
            if (btnBig_isPressed() == false) {
                break;
            }
        }
    }

    last_step:
    // attempt to restore camera to original state if needed
    // this sends the commands over and over again until the change is effective
    t = millis(); now = t;
    uint32_t timeout = 800;
    if (need_restore_ss && restore_shutter != NULL)
    {
        uint32_t cur_ss;
        uint32_t compare_ss;
        if (ptpcam.isOperating() && restore_shutter->flags == SPEEDTYPE_PTP) {
            compare_ss = restore_shutter->u32;
        }
        else if (httpcam.isOperating() && restore_shutter->flags == SPEEDTYPE_HTTP) {
            compare_ss = parse_shutter_speed_str(restore_shutter->str);
        }
        do
        {
            if (ptpcam.isOperating() && restore_shutter->flags == SPEEDTYPE_PTP && restore_shutter->u32 != 0) {
                ptpcam.cmd_ShutterSpeedSet32(restore_shutter->u32);
                ptpcam.wait_while_busy(100, DEFAULT_BUSY_TIMEOUT, NULL);
                cur_ss = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
                if (cur_ss == compare_ss) {
                    break;
                }
            }
            else if (httpcam.isOperating() && restore_shutter->flags == SPEEDTYPE_HTTP && restore_shutter->str[0] != 0) {
                httpcam.cmd_ShutterSpeedSetStr(restore_shutter->str);
                httpcam.wait_while_busy(100, DEFAULT_BUSY_TIMEOUT, NULL);
                cur_ss = httpcam.get_shutterspd_32();
                if (cur_ss == compare_ss) {
                    break;
                }
            }
        }
        while (((now = millis()) - t) < timeout || btnBig_isPressed());
    }

    t = millis(); now = t;
    if (need_restore_iso && restore_iso != NULL) {
        uint32_t cur_iso;
        do
        {
            if (ptpcam.isOperating() && restore_iso->flags == SPEEDTYPE_PTP && restore_iso->u32 != 0) {
                ptpcam.cmd_IsoSet(restore_iso->u32);
                ptpcam.wait_while_busy(100, DEFAULT_BUSY_TIMEOUT, NULL);
                cur_iso = ptpcam.get_property(SONYALPHA_PROPCODE_ISO);
                if (cur_iso == restore_iso->u32) {
                    break;
                }
            }
            else if (httpcam.isOperating() && restore_iso->flags == SPEEDTYPE_HTTP && restore_iso->str[0] != 0) {
                httpcam.cmd_IsoSetStr(restore_iso->str);
                httpcam.wait_while_busy(100, DEFAULT_BUSY_TIMEOUT, NULL);
                char* cur_iso_str = httpcam.get_iso_str();
                if (strcmp(cur_iso_str, restore_iso->str) == 0) {
                    break;
                }
            }
        }
        while (((now = millis()) - t) < timeout || btnBig_isPressed());
    }

    if (already_focused) {
        // wait for user to let go of button
        t = millis(); now = t;
        do
        {
            app_poll();
            if (ptpcam.isOperating())
            {
                if (ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound) == SONYALPHA_FOCUSSTATUS_NONE) {
                    break;
                }
            }
            else if (httpcam.isOperating())
            {
                if (httpcam.is_focused == false) {
                    break;
                }
            }
        }
        while (((now = millis()) - t) < timeout);
    }
    if (need_restore_af) {
        fairycam.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        fairycam.cmd_AutoFocus(false);
    }
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
}
