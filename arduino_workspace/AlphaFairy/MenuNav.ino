#include "AlphaFairy.h"

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

    if (m->last_idx != m->idx) { // prevent unnecessary re-draws
        guimenu_drawScreen(&(m->items[m->idx]));
        m->last_idx = m->idx;
        app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
    }
    else if (imu_hasChange) { // prevent unneccessary re-draws
        guimenu_drawPages(); // this draws the scroll dots that indicate which page we are on
        imu_hasChange = false;
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
        if ((m->flags & MENUFLAG_RETURN_POP) != 0)
        {
            guimenu_drawScreen(&(m->items[m->idx])); // any sort of indicator while in the function need to be removed, so just draw the whole menu here again
            ledblink_setMode(LEDMODE_NORMAL);
            app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
        }
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