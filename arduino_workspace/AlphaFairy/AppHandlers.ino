#include "AlphaFairy.h"

#define APP_DOES_NOTHING

void remotes_shutter(void* mip)
{
    menuitem_t* menuitm = (menuitem_t*)mip;
    int time_delay = 0;
    if (menuitm->id == MENUITEM_REMOTESHUTTER_NOW) {
        time_delay = 0;
    }
    else if (menuitm->id == MENUITEM_REMOTESHUTTER_2S) {
        time_delay = 2;
    }
    else if (menuitm->id == MENUITEM_REMOTESHUTTER_5S) {
        time_delay = 5;
    }
    else if (menuitm->id == MENUITEM_REMOTESHUTTER_10S) {
        time_delay = 10;
    }

    if (camera.isOperating() == false)
    {
        // no camera connected via wifi so use infrared if possible
        if (config_settings.infrared_enabled && time_delay <= 2) {
            if (time_delay == 2) {
                dbg_ser.println("IR - shoot 2s");
                SonyCamIr_Shoot2S();
            }
            else {
                dbg_ser.println("IR - shoot");
                SonyCamIr_Shoot();
            }
            return;
        }
        else {
            // show user that the camera isn't connected
            dbg_ser.println("remotes_shutter but no camera connected");
            app_waitAllReleaseConnecting(BTN_DEBOUNCE);
            return;
        }
    }

#ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
#endif

    bool starting_mf = camera.is_manuallyfocused();

    if (starting_mf == false) {
        camera.cmd_AutoFocus(true);
    }

    uint32_t tstart = millis();
    uint32_t now = tstart;
    uint32_t tdiff;
    bool quit = false;
    while (((tdiff = ((now = millis()) - tstart)) < (time_delay * 1000)) && camera.isOperating()) {
        if (app_poll()) {
            gui_drawVerticalDots(0, 20, -1, 3, time_delay, tdiff / 1000, false, TFT_GREEN, TFT_RED);
        }
        if (btnSide_hasPressed(true)) {
            quit = true;
            break;
        }
    }

    if (quit) {
        if (starting_mf == false) {
            camera.cmd_AutoFocus(false);
        }
    }

    tstart = millis();
    if (camera.is_focused)
    {
        camera.cmd_Shoot(config_settings.shutter_press_time_ms);
    }
    else if (btnBig_isPressed())
    {
        camera.cmd_Shutter(true);
        while ((btnBig_isPressed() || (starting_mf == false && camera.is_focused == false && ((now = millis()) - tstart) < 5000)) && camera.isOperating())
        {
            app_poll();
        }
        if (camera.is_focused) {
            camera.wait_while_busy(config_settings.shutter_press_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        }
        camera.cmd_Shutter(false);
    }

    if (starting_mf == false) {
        camera.cmd_AutoFocus(false);
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void focus_stack(void* mip)
{
    if (camera.isOperating() == false) {
        // show user that the camera isn't connected
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    menuitem_t* menuitm = (menuitem_t*)mip;

    int step_size = SONYALPHA_FOCUSSTEP_FARTHER_SMALL;
    if (menuitm->id == MENUITEM_FOCUSSTACK_FAR_1) {
        step_size = SONYALPHA_FOCUSSTEP_FARTHER_SMALL;
    }
    else if (menuitm->id == MENUITEM_FOCUSSTACK_FAR_2) {
        step_size = SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM;
    }
    else if (menuitm->id == MENUITEM_FOCUSSTACK_FAR_3) {
        step_size = SONYALPHA_FOCUSSTEP_FARTHER_LARGE;
    }
    else if (menuitm->id == MENUITEM_FOCUSSTACK_NEAR_1) {
        step_size = SONYALPHA_FOCUSSTEP_CLOSER_SMALL;
    }
    else if (menuitm->id == MENUITEM_FOCUSSTACK_NEAR_2) {
        step_size = SONYALPHA_FOCUSSTEP_CLOSER_MEDIUM;
    }
    else if (menuitm->id == MENUITEM_FOCUSSTACK_NEAR_3) {
        step_size = SONYALPHA_FOCUSSTEP_CLOSER_LARGE;
    }

    bool starting_mf = camera.is_manuallyfocused();

    if (starting_mf == false && camera.isOperating())
    {
        // obviously we need to be in manual focus mode in order to do focus stacking
        camera.cmd_ManualFocusMode(true);
    }

    // TODO:
    // returning the camera to original focus point will be hard
    // there is an indicator but I can't be sure when it is updated
    // and it is in the range 0 to 100 only, some fine movements won't register as a change

    int starting_focus_dist = config_settings.manual_focus_return != 0 ? 0 : -1;
    int steps_done = 0;
    if (starting_focus_dist >= 0)
    {
        // we want to be able to return the focus to the starting point
        // but we started off in auto-focus mode
        // there's a chance that we haven't gotten the correct focus distance yet, it might be in memory but not correct
        // there's no way to solve this, even if a new dev-properties packet arrives, we don't know if the contents contain outdated information
        // so the best thing to do is just do a wait while polling
        if (starting_mf == false) {
            app_sleep(300, true);
        }

        if (camera.has_property(SONYALPHA_PROPCODE_ManualFocusDist)) {
            starting_focus_dist = camera.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        }
        else {
            starting_focus_dist = -1;
        }
    }

    // configuring the shutter_press_time_ms to be 0 means hold the button down as the focus is stepped
    // this will work when the camera is in continuous shooting mode
    // but I can't guarantee that the photos are one-step-one-frame
    bool is_contshoot = false;
    if (config_settings.shutter_press_time_ms == 0 && camera.is_continuousshooting() && camera.isOperating())
    {
        is_contshoot = true;
        camera.cmd_Shutter(true);
    }

    int dot_idx = 0;

    while (camera.isOperating()) // I wish we had try-catches
    {
        app_poll();
        gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, step_size > 0, TFT_GREEN, TFT_RED);
        if (is_contshoot == false)
        {
            camera.cmd_Shoot(config_settings.shutter_press_time_ms);
        }
        camera.cmd_ManualFocusStep(step_size);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        steps_done++;
        dot_idx++;

        if (btnBig_isPressed() == false) {
            // check button release here so at least one photo is captured
            break;
        }
    }

    uint32_t t_end_1 = millis();

    if (is_contshoot && camera.isOperating())
    {
        camera.cmd_Shutter(false);
    }

    if (starting_focus_dist >= 0)
    {
        // this loop is what restores the focus point back to the starting focus point
        int x = camera.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        do
        {
            app_poll();
            gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, step_size < 0, TFT_GREEN, TFT_RED);
            camera.cmd_ManualFocusStep(step_size * -1);
            steps_done--;
            camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
            x = camera.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
            dot_idx++;
        }
        while (
                true // ignore me
                && ((x > starting_focus_dist && step_size > 0) || (x < starting_focus_dist && step_size < 0)) // soft limit on the distance reported
                && steps_done >= 0 // hard limit on the number of steps
                && camera.isOperating()
                );
    }

    if (starting_mf == false && camera.isOperating())
    {
        // if the camera was set to MF mode via PTP, then it will be stuck in MF mode even if the user toggle switches on the camera or lens!
        // so we MUST restore the setting via PTP
        camera.cmd_ManualFocusMode(false);
    }

    // this kinda imposes a minimum button release time
    uint32_t t_end_2 = millis();
    if ((t_end_2 - t_end_1) < 50) {
        app_sleep(50, true);
    }
}

void focus_9point(void* mip)
{
    if (camera.isOperating() == false) {
        dbg_ser.println("focus_9point but no camera connected");
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

#ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
#endif

    if (camera.is_spotfocus() == false) {
        // the camera must be in one of the many spot focus modes
        // we can't pick one for them
        // show user the error message
        gui_startPrint();
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.println("ERROR:");
        gui_setCursorNextLine();
        M5Lcd.println("not in spot auto-focus mode");
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }

    int dot_rad = 2;
    int dot_space = 25;
    int dot_y_start = M5Lcd.height() / 2;
    int dot_x_start = M5Lcd.width() / 2;
    // TODO:
    // the starting points of the dots need to be adjusted according to what the JPG looks like

    int x_cent = SONYALPHA_FOCUSPOINT_X_MAX / 2;
    int y_cent = SONYALPHA_FOCUSPOINT_Y_MAX / 2;
    int dist = config_settings.nine_point_dist;
    int x, y, dot_x, dot_y;

    int i;
    for (i = 0; camera.isOperating(); i++)
    {
        app_poll();
        // figure out which point to focus on
        switch (i)
        {
            case 0: x = x_cent;        y = y_cent;        dot_x = dot_x_start;             dot_y = dot_y_start;             break;
            case 1: x = x_cent + dist; y = y_cent;        dot_x = dot_x_start + dot_space; dot_y = dot_y_start;             break;
            case 2: x = x_cent - dist; y = y_cent;        dot_x = dot_x_start - dot_space; dot_y = dot_y_start;             break;
            case 3: x = x_cent;        y = y_cent + dist; dot_x = dot_x_start;             dot_y = dot_y_start + dot_space; break;
            case 4: x = x_cent;        y = y_cent - dist; dot_x = dot_x_start;             dot_y = dot_y_start - dot_space; break;
            case 5: x = x_cent + dist; y = y_cent + dist; dot_x = dot_x_start + dot_space; dot_y = dot_y_start + dot_space; break;
            case 6: x = x_cent + dist; y = y_cent - dist; dot_x = dot_x_start + dot_space; dot_y = dot_y_start - dot_space; break;
            case 7: x = x_cent - dist; y = y_cent + dist; dot_x = dot_x_start - dot_space; dot_y = dot_y_start + dot_space; break;
            case 8: x = x_cent - dist; y = y_cent - dist; dot_x = dot_x_start - dot_space; dot_y = dot_y_start - dot_space; break;
            // the first 9 points are fixed, after that, the points are random
            default:
                x = 20 + (rand() % (SONYALPHA_FOCUSPOINT_X_MAX - 20));
                y = 20 + (rand() % (SONYALPHA_FOCUSPOINT_Y_MAX - 20));
                break;
        }
        if (i < 9)
        {
            // show user which point was just set
            // this is just cosmetic
            M5Lcd.fillCircle(dot_x, dot_y, dot_rad, TFT_GREEN);
        }
        // set the point
        camera.cmd_FocusPointSet(x, y);
        camera.cmd_AutoFocus(true);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

        // wait for autofocus to lock onto something
        uint32_t tstart = millis();
        uint32_t now;
        uint32_t tlimit = (i < 9) ? 5000 : 2000;
        while (((now = millis()) - tstart) < 5000 && camera.is_focused == false && camera.isOperating())
        {
            app_poll();
        }

        // take the photo
        camera.cmd_Shoot(config_settings.shutter_press_time_ms);
        camera.cmd_AutoFocus(false);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

        if (btnBig_isPressed() == false) {
            // button check here means we get at least one photo
            break;
        }
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void wifi_info(void* mip)
{
    dbg_ser.println("wifi_info");

    gui_startPrint();

    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    M5Lcd.println("Wi-Fi SSID:");
    gui_setCursorNextLine();
    M5Lcd.println(NetMgr_getSSID());
    gui_setCursorNextLine();
    M5Lcd.println("Password:");
    gui_setCursorNextLine();
    M5Lcd.println(NetMgr_getPassword());
    uint32_t ip_addr;
    if ((ip_addr = NetMgr_getClient()) != 0) {
        gui_setCursorNextLine();
        M5Lcd.print("Cam IP: ");
        M5Lcd.println(IPAddress(ip_addr));
    }
    char* cam_name = NULL;
    if (camera.isOperating()) {
        cam_name = camera.getCameraName();
    }
    if (cam_name != NULL && strlen(cam_name) > 0) {
        gui_setCursorNextLine();
        M5Lcd.println(cam_name);
    }

    app_waitAnyPress();
}

void record_movie(void* mip)
{
    if (camera.isOperating() == false)
    {
        if (config_settings.infrared_enabled) {
            dbg_ser.println("IR - movie");
            SonyCamIr_Movie();
            return;
        }
        else {
            dbg_ser.println("record_movie but no camera connected");
            app_waitAllReleaseConnecting(BTN_DEBOUNCE);
            return;
        }
    }

#ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
#endif

    camera.cmd_MovieRecordToggle();
    app_waitAllRelease(BTN_DEBOUNCE);
}
