#include "AlphaFairy.h"
#include <M5DisplayExt.h>

extern uint8_t remoteshutter_delay;
extern uint32_t dual_shutter_next, dual_shutter_iso;
extern uint32_t dual_shutter_last_tv, dual_shutter_last_iso;

bool guimenu_task(menustate_t* m)
{
    static uint32_t last_time = 0;
    uint32_t now = millis();

    if (btnSide_hasPressed())
    {
        // side-button press means go to another menu item
        // go to the previous item if the angle of the device is "down"
        if (imu.getTilt() == TILT_IS_DOWN && (m->flags & MENUFLAG_CAN_GO_DOWN) != 0) {
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
    else if (m->items[m->idx].id == MENUITEM_FOCUS_PULL) {
        gui_drawFocusPullState();
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
        if (dual_shutter_next == 0)
        {
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(0, 70);
            M5Lcd.printf("  ");
            M5Lcd.setCursor(5, 70);
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
            gui_showVal(dual_shutter_next, TXTFMT_SHUTTER, (Print*)&M5Lcd);
            gui_blankRestOfLine();
            M5Lcd.setCursor(10, 65 + 18);
            M5Lcd.printf("ISO ");
            gui_showVal(dual_shutter_iso, TXTFMT_ISO, (Print*)&M5Lcd);
            gui_blankRestOfLine();
        }

        if (m->items[m->idx].id == MENUITEM_DUALSHUTTER_SHOOT && camera.isOperating())
        {
            if (camera.has_property(SONYALPHA_PROPCODE_FocusFound) && camera.get_property(SONYALPHA_PROPCODE_FocusFound) == SONYALPHA_FOCUSSTATUS_FOCUSED)
            {
                // trigger via shutter half press
                gui_drawTopThickLine(8, TFT_RED); // indicate
                dual_shutter_shoot(true, false, dual_shutter_last_tv, dual_shutter_last_iso);
                gui_drawTopThickLine(8, TFT_WHITE);
            }
        }

        if (camera.isOperating() && camera.has_property(SONYALPHA_PROPCODE_ShutterSpeed) && camera.has_property(SONYALPHA_PROPCODE_ISO) && camera.has_property(SONYALPHA_PROPCODE_FocusFound) && camera.get_property(SONYALPHA_PROPCODE_FocusFound) == SONYALPHA_FOCUSSTATUS_NONE)
        {
            // remember last known setting
            uint32_t tv  = camera.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
            uint32_t iso = camera.get_property(SONYALPHA_PROPCODE_ISO);
            // only remember if it's not the same (workaround for the camera not responding to restore commands)
            dual_shutter_last_tv  = tv  != dual_shutter_next ?  tv : dual_shutter_last_tv;
            dual_shutter_last_iso = iso != dual_shutter_iso  ? iso : dual_shutter_last_iso;
        }
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

        sprites->unload_all();
        menuitm->func((void*)menuitm);
        sprites->unload_all();


        if (m->items[m->idx].id != MENUITEM_FOCUS_PULL) { // do not redraw items that require faster response
            m->last_idx = -1; // force redraw
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