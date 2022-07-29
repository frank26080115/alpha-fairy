#include "AlphaFairy.h"
#include <M5DisplayExt.h>

bool guimenu_task(menustate_t* m)
{
    if (btnSide_hasPressed(true))
    {
        // side-button press means go to another menu item
        // go to the previous item if the angle of the device is "down"
        if (imu_angle == ANGLE_IS_DOWN && (m->flags & MENUFLAG_CAN_GO_DOWN) != 0) {
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
    }

    if (batt_need_recheck) {
        // if battery state changed, force a redraw
        batt_need_recheck = false;
        m->last_idx = -1;
    }

    if (m->last_idx != m->idx) { // prevent unnecessary re-draws
        guimenu_drawScreen(&(m->items[m->idx]));
        if ((m->flags & MENUFLAG_DRAW_PAGES) != 0) {
            guimenu_drawPages();
        }
        if (is_low_batt()) {
            battlow_draw(false);
        }
        m->last_idx = m->idx;
        app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
    }
    else if (imu_hasChange) { // prevent unneccessary re-draws
        if ((m->flags & MENUFLAG_DRAW_PAGES) != 0) {
            guimenu_drawPages();
        }
        imu_hasChange = false;
    }

    if (m->items[m->idx].id == MENUITEM_RECORDMOVIE) {
        // indicate a tally light on the screen
        gui_drawMovieRecStatus();
    }

    if (btnBig_hasPressed(true))
    {
        menuitem_t* menuitm = (menuitem_t*)&(m->items[m->idx]);
        if (menuitm->id == MENUITEM_BACK)
        {
            dbg_ser.printf("menu[%u] idx %u - %u return\r\n", m->id, m->idx, menuitm->id);
            return true;
        }
        dbg_ser.printf("menu[%u] idx %u - %u calling func\r\n", m->id, m->idx, menuitm->id);
        menuitm->func((void*)menuitm);
        m->last_idx = -1; // force redraw
        ledblink_setMode(LEDMODE_NORMAL);
        app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
    }

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
            }
        }

        return;
    }

    curmenu = &menustate_main;
}