#include "AlphaFairy.h"

#define APP_DOES_NOTHING
#define APPHDL_DEBUG_PRINTLN Serial.println
#define APPHDL_DEBUG_PRINTF Serial.printf

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
        if (config_settings.infrared_enabled && time_delay <= 2) {
            if (time_delay == 2) {
                APPHDL_DEBUG_PRINTLN("IR - shoot 2s");
                SonyCamIr_Shoot2S();
            }
            else {
                APPHDL_DEBUG_PRINTLN("IR - shoot");
                SonyCamIr_Shoot();
            }
            return;
        }
        else {
            APPHDL_DEBUG_PRINTLN("remotes_shutter but no camera connected");
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
    int starting_focus_dist = config_settings.manual_focus_return != 0 ? 0 : -1;
    if (starting_focus_dist >= 0) {
        if (camera.has_property(SONYALPHA_PROPCODE_ManualFocusDist)) {
            starting_focus_dist = camera.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        }
        else {
            starting_focus_dist = -1;
        }
    }

    if (starting_mf == false && camera.isOperating())
    {
        camera.cmd_ManualFocusMode(true);
    }

    if (config_settings.shutter_press_time_ms == 0 && camera.isOperating())
    {
        camera.cmd_Shutter(true);
    }

    int dot_idx = 0;

    while (btnBig_isPressed() && camera.isOperating())
    {
        gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, true, TFT_GREEN, TFT_RED);
        if (config_settings.shutter_press_time_ms != 0)
        {
            camera.cmd_Shoot(config_settings.shutter_press_time_ms);
        }
        camera.cmd_ManualFocusStep(step_size);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        dot_idx++;
    }

    uint32_t t_end_1 = millis();

    if (config_settings.shutter_press_time_ms == 0 && camera.isOperating())
    {
        camera.cmd_Shutter(false);
    }

    if (starting_focus_dist >= 0)
    {
        int x = camera.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        do
        {
            gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, true, TFT_GREEN, TFT_RED);
            camera.cmd_ManualFocusStep(step_size * -1);
            camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
            x = camera.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
            dot_idx++;
        }
        while (((x > starting_focus_dist && step_size > 0) || (x < starting_focus_dist && step_size < 0)) && camera.isOperating());
    }

    if (starting_mf == false && camera.isOperating())
    {
        camera.cmd_ManualFocusMode(false);
    }

    uint32_t t_end_2 = millis();
    if ((t_end_2 - t_end_1) < 100) {
        app_sleep(100, true);
    }
}

void focus_9point(void* mip)
{
    if (camera.isOperating() == false) {
        APPHDL_DEBUG_PRINTLN("focus_9point but no camera connected");
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

#ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
#endif

    if (camera.is_spotfocus() == false) {
        gui_startPrint();
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.println("ERROR:");
        M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
        M5Lcd.println("not in spot auto-focus mode");
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }

    int dot_rad = 2;
    int dot_space = 25;
    int dot_y_start = M5Lcd.height() / 2;
    int dot_x_start = M5Lcd.width() / 2;

    int x_cent = SONYALPHA_FOCUSPOINT_X_MAX / 2;
    int y_cent = SONYALPHA_FOCUSPOINT_Y_MAX / 2;
    int dist = config_settings.nine_point_dist;
    int x, y, dot_x, dot_y;

    int i;
    for (i = 0; btnBig_isPressed() && camera.isOperating(); i++)
    {
        app_poll();
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
            default:
                x = 20 + (rand() % (SONYALPHA_FOCUSPOINT_X_MAX - 20));
                y = 20 + (rand() % (SONYALPHA_FOCUSPOINT_Y_MAX - 20));
                break;
        }
        if (i < 9)
        {
            M5Lcd.fillCircle(dot_x, dot_y, dot_rad, TFT_GREEN);
        }
        camera.cmd_FocusPointSet(x, y);
        camera.cmd_AutoFocus(true);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        uint32_t tstart = millis();
        uint32_t now;
        uint32_t tlimit = (i < 9) ? 5000 : 2000;
        while (((now = millis()) - tstart) < 5000 && camera.is_focused == false && camera.isOperating())
        {
            app_poll();
        }
        camera.cmd_Shoot(config_settings.shutter_press_time_ms);
        camera.cmd_AutoFocus(false);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void wifi_info(void* mip)
{
    APPHDL_DEBUG_PRINTLN("wifi_info");

    gui_startPrint();

    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    M5Lcd.println("Wi-Fi SSID:");
    M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
    M5Lcd.println(NetMgr_getSSID());
    M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
    M5Lcd.println("Password:");
    M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
    M5Lcd.println(NetMgr_getPassword());
    uint32_t ip_addr;
    if ((ip_addr = NetMgr_getClient()) != 0) {
        M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
        M5Lcd.println("Client IP:");
        M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
        M5Lcd.println(IPAddress(ip_addr));
    }
    char* cam_name = NULL;
    if (camera.isOperating()) {
        cam_name = camera.getCameraName();
    }
    if (cam_name != NULL && strlen(cam_name) > 0) {
        M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
        M5Lcd.println(cam_name);
    }

    app_waitAnyPress();
}

void record_movie(void* mip)
{
    if (camera.isOperating() == false)
    {
        if (config_settings.infrared_enabled) {
            APPHDL_DEBUG_PRINTLN("IR - movie");
            SonyCamIr_Movie();
            return;
        }
        else {
            APPHDL_DEBUG_PRINTLN("record_movie but no camera connected");
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
