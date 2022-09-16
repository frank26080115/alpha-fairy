#include "AlphaFairy.h"
#include <M5DisplayExt.h>

extern bool signal_wifiauthfailed;

extern uint8_t remoteshutter_delay;
extern speed_t dual_shutter_next, dual_shutter_iso;
extern speed_t dual_shutter_last_tv, dual_shutter_last_iso;

bool guimenu_task(menustate_t* m)
{
    static uint32_t last_time = 0;
    uint32_t now = millis();

    if (btnSide_hasPressed())
    {
        // side-button press means go to another menu item
        // go to the previous item if the angle of the device is "down"
        if (imu.getTilt() < 0 && (m->flags & MENUFLAG_CAN_GO_DOWN) != 0) {
            if (m->idx > 0) {
                m->idx -= 1;
                dbg_ser.printf("menu[%u] prev idx %u\r\n", m->id, m->idx);
            }
        }
        else { // go to the next item if the angle is not "down"
            if (m->idx < (m->cnt - 1)) {
                m->idx += 1;
            }
            else {
                m->idx = 0;
            }
            dbg_ser.printf("menu[%u] next idx %u\r\n", m->id, m->idx);
        }
        btnSide_clrPressed();
    }
    #if defined(USE_PWR_BTN_AS_BACK) && !defined(USE_PWR_BTN_AS_EXIT)
    if (btnPwr_hasPressed())
    {
        if (m->idx > 0) {
            m->idx -= 1;
        }
        else {
            m->idx = m->cnt - 1;
        }
        dbg_ser.printf("menu[%u] prev idx %u\r\n", m->id, m->idx);
        btnPwr_clrPressed();
    }
    #endif

    if (m->last_idx != m->idx && m->items[m->idx].id == MENUITEM_FOCUSFRUSTRATION) {
        focusfrust_reset();
    }

    #ifdef SHCAM_NEED_ENTER_MOVIE_MODE
    #if 0
    if (m->last_idx != m->idx && httpcam.isOperating())
    {
        if (m->items[m->idx].id == MENUITEM_RECORDMOVIE) {
            httpcam.cmd_MovieMode(true);
        }
        else if (m->items[m->last_idx].id == MENUITEM_RECORDMOVIE) {
            httpcam.cmd_MovieMode(false);
        }
    }
    #endif
    #endif

    if (signal_wifiauthfailed) {
        gui_handleWifiAuthFailed();
        signal_wifiauthfailed = false;
    }

    if (m->last_idx != m->idx || redraw_flag) { // prevent unnecessary re-draws
        redraw_flag = false;

        if (m->items[m->idx].id == MENUITEM_REMOTESHUTTER_DLY && m->items[m->last_idx].id == MENUITEM_REMOTESHUTTER_NOW) {
            #if 0
            // this transition needs a better indication, so blank the whole screen quickly first
            M5Lcd.fillScreen(TFT_WHITE);
            #else
            // this transition needs a better indication, so fade the screen quickly first
            for (int16_t y = 0; y < M5Lcd.height() - 20; y += 2) {
                M5Lcd.drawFastHLine(0, y, M5Lcd.width(), TFT_WHITE);
            }
            #endif
        }

        guimenu_drawScreen(&(m->items[m->idx]));
        if ((m->flags & MENUFLAG_DRAW_PAGES) != 0) {
            guimenu_drawPages();
        }
        gui_drawStatusBar(false);
        m->last_idx = m->idx; // prevent unnecessary re-draws
        app_sleep(1, true); // kinda sorta a debounce and rate limit, don't think I need this here
        // note: above call clears buttons
        imu.resetSpin();
    }
    else if (imu.hasChange) { // prevent unneccessary re-draws
        if ((m->flags & MENUFLAG_DRAW_PAGES) != 0) {
            guimenu_drawPages();
        }
        imu.hasChange = false;
    }

    // periodically update just the status bar, in case of changes happening while idle
    if ((now - last_time) > 1000) {
        last_time = now;
        gui_drawStatusBar(false);
    }

    // draw live updates to the screen

    if (m->items[m->idx].id == MENUITEM_RECORDMOVIE) {
        // indicate a tally light on the screen
        gui_drawMovieRecStatus();
    }
    else if ((m->items[m->idx].id == MENUITEM_FOCUS_PULL && httpcam.isOperating() == false) || m->items[m->idx].id == MENUITEM_ZOOM_PULL) {
        gui_drawFocusPullState(108);
    }
    else if (m->items[m->idx].id == MENUITEM_REMOTESHUTTER_DLY)
    {
        // change the time delay for the remote shutter by spinning
        if (imu.getSpin() > 0) {
            if (remoteshutter_delay == 2) {
                remoteshutter_delay = 5;
            }
            else if (remoteshutter_delay == 5) {
                remoteshutter_delay = 10;
            }
            imu.resetSpin();
        }
        else if (imu.getSpin() < 0) {
            if (remoteshutter_delay == 10) {
                remoteshutter_delay = 5;
            }
            else if (remoteshutter_delay == 5) {
                remoteshutter_delay = 2;
            }
            imu.resetSpin();
        }
        gui_startMenuPrint();
        M5Lcd.setCursor(80, 188);
        M5Lcd.printf("%us   ", remoteshutter_delay);
    }
    else if (m->items[m->idx].id == MENUITEM_DUALSHUTTER_REG || m->items[m->idx].id == MENUITEM_DUALSHUTTER_SHOOT)
    {
        gui_startMenuPrint();
        if (dual_shutter_next.flags == SPEEDTYPE_NONE)
        {
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(0, 70);
            M5Lcd.printf("  ");
            M5Lcd.setCursor(6, 70);
            M5Lcd.printf(" NOT  SET");
            gui_blankRestOfLine();
        }
        else
        {
            M5Lcd.setTextFont(2);
            M5Lcd.setCursor(0, 65);
            M5Lcd.printf("  ");
            M5Lcd.setCursor(10, 65);
            M5Lcd.printf("Tv ");
            if (dual_shutter_next.flags == SPEEDTYPE_PTP) {
                gui_showVal(dual_shutter_next.u32, TXTFMT_SHUTTER, (Print*)&M5Lcd);
            }
            else {
                M5Lcd.print(dual_shutter_next.str);
            }
            gui_blankRestOfLine();
            M5Lcd.setCursor(10, 65 + 18);
            M5Lcd.printf("ISO ");
            if (dual_shutter_iso.flags == SPEEDTYPE_PTP) {
                gui_showVal(dual_shutter_iso.u32, TXTFMT_ISO, (Print*)&M5Lcd);
            }
            else {
                M5Lcd.print(dual_shutter_iso.str);
            }
            gui_blankRestOfLine();
        }

        if (m->items[m->idx].id == MENUITEM_DUALSHUTTER_SHOOT)
        {
            if ((ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_FocusFound) && ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound) == SONYALPHA_FOCUSSTATUS_FOCUSED) || (httpcam.isOperating() && httpcam.is_focused))
            {
                // trigger via shutter half press
                gui_drawTopThickLine(8, TFT_RED); // indicate
                dual_shutter_shoot(true, false, &dual_shutter_last_tv, &dual_shutter_last_iso);
                gui_drawTopThickLine(8, TFT_WHITE);
            }
        }

        if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ShutterSpeed) && ptpcam.has_property(SONYALPHA_PROPCODE_ISO) && ptpcam.has_property(SONYALPHA_PROPCODE_FocusFound) && ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound) == SONYALPHA_FOCUSSTATUS_NONE)
        {
            // remember last known setting
            uint32_t tv  = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
            uint32_t iso = ptpcam.get_property(SONYALPHA_PROPCODE_ISO);
            // only remember if it's not the same (workaround for the camera not responding to restore commands)
            dual_shutter_last_tv.u32    = (tv  != dual_shutter_next.u32 || dual_shutter_next.flags != SPEEDTYPE_PTP) ?  tv : dual_shutter_last_tv.u32;
            dual_shutter_last_iso.u32   = (iso != dual_shutter_iso.u32  || dual_shutter_iso.flags  != SPEEDTYPE_PTP) ? iso : dual_shutter_last_iso.u32;
            dual_shutter_last_tv.flags  = SPEEDTYPE_PTP;
            dual_shutter_last_iso.flags = SPEEDTYPE_PTP;
        }
    }
    else if (m->items[m->idx].id == MENUITEM_FOCUSFRUSTRATION)
    {
        focusfrust_task();
    }

    if (btnBig_hasPressed())
    {
        menuitem_t* menuitm = (menuitem_t*)&(m->items[m->idx]);
        if (menuitm->id == MENUITEM_BACK)
        {
            btnBig_clrPressed();
            dbg_ser.printf("menu[%u] idx %u - %u return\r\n", m->id, m->idx, menuitm->id);
            return true;
        }
        dbg_ser.printf("menu[%u] idx %u - %u calling func\r\n", m->id, m->idx, menuitm->id);
        btnBig_clrPressed();

        if (menuitm->func != NULL) {
            #ifdef USE_SPRITE_MANAGER
            sprites->unload_all();
            #endif
            menuitm->func((void*)menuitm);
            #ifdef USE_SPRITE_MANAGER
            sprites->unload_all();
            #endif
        }

        if (m->items[m->idx].id != MENUITEM_FOCUS_PULL) { // do not redraw items that require faster response
            m->last_idx = -1; // force redraw
        }

        if (m->items[m->idx].id == MENUITEM_FENCCALIB) {
            m->last_idx = m->idx;
            redraw_flag = false;
        }

        ledblink_setMode(LEDMODE_NORMAL);
        app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
        btnBig_clrPressed();
    }
    #if defined(USE_PWR_BTN_AS_EXIT) && !defined(USE_PWR_BTN_AS_BACK)
    if (btnPwr_hasPressed())
    {
        dbg_ser.printf("menu[%u] idx %u return from pwr btn\r\n", m->id, m->idx);
        btnPwr_clrPressed();
        return true;
    }
    #endif

    return false;
}

void submenu_enter(void* mip)
{
    menuitem_t* menuitm = (menuitem_t*)mip;
    menustate_t* m = NULL;
    if (menuitm->id == MENUITEM_REMOTE) {
        m = &menustate_remote;
    }
    else if (menuitm->id == MENUITEM_FOCUS) {
        m = &menustate_focus;
    }
    else if (menuitm->id == MENUITEM_UTILS) {
        m = &menustate_utils;
    }
    else if (menuitm->id == MENUITEM_MAIN) {
        m = &menustate_main;
    }

    if (m != NULL)
    {
        curmenu = m;
        m->idx = 0;
        m->last_idx = -1; // force redraw
        if (m->items != NULL) {
            guimenu_drawScreen(&(m->items[m->idx])); // draw here before button release
        }
        app_waitAllRelease(BTN_DEBOUNCE);
        while (true)
        {
            if (app_poll())
            {
                tallylite_task();

                if (guimenu_task(m))
                {
                    return;
                }
                pwr_sleepCheck();
            }
        }

        return;
    }

    curmenu = &menustate_main;
}

extern bool autoconnect_active;

void gui_handleWifiAuthFailed()
{
    autoconnect_active = true;
    bool user_quit = false;

    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, "/wifi_reject.png", 0, 0);
    while (true)
    {
        autoconnect_poll();
        if (btnBoth_hasPressed())
        {
            // normal button press means retry entering the password
            dbg_ser.printf("autoconnect user wants retry\r\n");
            btnBoth_clrPressed();
            break;
        }
        if (btnPwr_hasPressed())
        {
            // power button press means give up
            dbg_ser.printf("autoconnect user wants give up\r\n");
            btnPwr_clrPressed();
            user_quit = true;
        }
    }

    if (user_quit == false)
    {
        uint8_t profile_num = config_settings.wifi_profile;
        wifiprofile_t profile;
        bool got_profile = wifiprofile_getProfile(profile_num, &profile); // this can't possibly fail
        if (wifi_newConnectOrPrompt(profile_num, &profile, true, false)) {
            // user cancel
        }
        else {
            // user did not cancel
        }
    }
    autoconnect_active = false;
    redraw_flag = true;
}
