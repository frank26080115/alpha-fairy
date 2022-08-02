#include "AlphaFairy.h"

//#define APP_DOES_NOTHING // this is only used for testing purposes

uint8_t remoteshutter_delay = 2;
uint32_t dual_shutter_next = 0;
uint32_t dual_shutter_iso = 0;
uint32_t dual_shutter_last_tv = 0;
uint32_t dual_shutter_last_iso = 0;

void cam_shootQuick()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (camera.isOperating()) {
        camera.cmd_Shoot(config_settings.shutter_press_time_ms);
    }
    else if (config_settings.gpio_enabled) {
        cam_shootQuickGpio();
    }
    else if (config_settings.infrared_enabled) {
        SonyCamIr_Shoot();
    }
}

void cam_shootQuickGpio()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    #ifdef SHUTTER_GPIO_ACTIVE_HIGH
    pinMode(SHUTTER_GPIO, OUTPUT);
    digitalWrite(SHUTTER_GPIO, HIGH);
    app_sleep(config_settings.shutter_press_time_ms, false);
    digitalWrite(SHUTTER_GPIO, LOW);
    #else
    digitalWrite(SHUTTER_GPIO, LOW);
    pinMode(SHUTTER_GPIO, OUTPUT);
    digitalWrite(SHUTTER_GPIO, LOW);
    app_sleep(config_settings.shutter_press_time_ms, false);
    pinMode(SHUTTER_GPIO, INPUT);
    #endif
}

void cam_shootOpen()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (camera.isOperating()) {
        camera.cmd_Shutter(true);
        if (gpio_time != 0) {
            #ifdef SHUTTER_GPIO_ACTIVE_HIGH
            digitalWrite(SHUTTER_GPIO, LOW);
            #endif
            pinMode(SHUTTER_GPIO, INPUT);
            gpio_time = 0;
        }
    }
    else if (config_settings.gpio_enabled) {
        #ifdef SHUTTER_GPIO_ACTIVE_HIGH
        pinMode(SHUTTER_GPIO, OUTPUT);
        digitalWrite(SHUTTER_GPIO, HIGH);
        #else
        digitalWrite(SHUTTER_GPIO, LOW);
        pinMode(SHUTTER_GPIO, OUTPUT);
        digitalWrite(SHUTTER_GPIO, LOW);
        #endif
        gpio_time = millis();
    }
    else if (config_settings.infrared_enabled) {
        SonyCamIr_Shoot();
    }
}

void cam_shootClose()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (camera.isOperating()) {
        camera.cmd_Shutter(false);
        if (gpio_time != 0) {
            #ifdef SHUTTER_GPIO_ACTIVE_HIGH
            digitalWrite(SHUTTER_GPIO, LOW);
            #endif
            pinMode(SHUTTER_GPIO, INPUT);
            gpio_time = 0;
        }
    }
    else if (config_settings.gpio_enabled) {
        gpio_time = 0; // stop the timer
        #ifdef SHUTTER_GPIO_ACTIVE_HIGH
        digitalWrite(SHUTTER_GPIO, LOW);
        #else
        pinMode(SHUTTER_GPIO, INPUT);
        #endif
    }
    else if (config_settings.infrared_enabled) {
        // do nothing
    }
}

void remote_shutter(void* mip)
{
    uint32_t tstart, now, tdiff;
    bool quit = false;

    dbg_ser.println("remote_shutter");

    menuitem_t* menuitm = (menuitem_t*)mip;
    int time_delay = 0;
    // determine time delay based on which menu item was used
    if (menuitm->id == MENUITEM_REMOTESHUTTER_NOW) {
        time_delay = 0;
    }
    else if (menuitm->id == MENUITEM_REMOTESHUTTER_DLY) {
        time_delay = remoteshutter_delay;
    }

    if (camera.isOperating() == false)
    {
        bool can_still_shoot = false;
        // no camera connected via wifi so use infrared or GPIO if possible
        if (config_settings.infrared_enabled && time_delay <= 2)
        {
            ledblink_setMode(LEDMODE_OFF);
            ledblink_on();

            if (time_delay == 2) {
                dbg_ser.println("IR - shoot 2s");
                SonyCamIr_Shoot2S();
            }
            else {
                dbg_ser.println("IR - shoot");
                SonyCamIr_Shoot();
            }

            ledblink_off();

            can_still_shoot = true;
        }

        if (config_settings.gpio_enabled)
        {
            dbg_ser.print("GPIO rmt ");
            ledblink_setMode(LEDMODE_OFF);
            if (time_delay > 0)
            {
                dbg_ser.print("wait ");
                tstart = millis();
                now = tstart;
                quit = false;
                // wait the countdown time
                while (((tdiff = ((now = millis()) - tstart)) < (time_delay * 1000))) {
                    if (app_poll()) {
                        if (time_delay > 2) {
                            gui_drawVerticalDots(0, 20, -1, 3, time_delay, tdiff / 1000, false, TFT_GREEN, TFT_RED);
                        }
                    }
                    if (btnSide_hasPressed(true)) {
                        quit = true;
                        break;
                    }
                }
            }
            dbg_ser.println("shoot");
            cam_shootQuickGpio();
            can_still_shoot = true;
        }

        if (can_still_shoot == false) {
            // show user that the camera isn't connected
            dbg_ser.println("remote_shutter but no camera connected");
            app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        }
        return;
    }

    ledblink_setMode(LEDMODE_OFF);

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

    bool starting_mf = camera.is_manuallyfocused();

    // start focusing at the beginning of the countdown
    if (starting_mf == false) {
        dbg_ser.printf("rmtshutter AF\r\n");
        camera.cmd_AutoFocus(true);
    }

    tstart = millis();
    now = tstart;
    quit = false;
    if (time_delay > 0)
    {
        dbg_ser.printf("rmtshutter wait delay %u... ", time_delay);
        // wait the countdown time
        while (((tdiff = ((now = millis()) - tstart)) < (time_delay * 1000)) && camera.isOperating()) {
            if (app_poll()) {
                if (time_delay > 2) {
                    gui_drawVerticalDots(0, 20, -1, 3, time_delay, tdiff / 1000, false, TFT_DARKGREEN, TFT_RED);
                }
            }
            if (btnSide_hasPressed(true)) {
                quit = true;
                break;
            }
        }

        // if user cancelled
        if (quit) {
            dbg_ser.printf(" user cancelled\r\n");
            // end autofocus
            if (starting_mf == false) {
                camera.cmd_AutoFocus(false);
            }
            ledblink_off();
            app_waitAllRelease(BTN_DEBOUNCE);
            return;
        }
        else {
            dbg_ser.printf(" done\r\n");
        }
    }

    bool fail_shown = false;

    tstart = millis();
    ledblink_on();
    app_poll();
    if (camera.is_focused || starting_mf)
    {
        // if the camera is focused or MF, then the shutter should immediately take the picture
        camera.cmd_Shoot(config_settings.shutter_press_time_ms);
        dbg_ser.printf("rmtshutter shoot\r\n");
    }
    else if (btnBig_isPressed() || time_delay == 0)
    {
        // if the camera is not focused, we try to take the shot anyways if the user is holding the button
        camera.cmd_Shutter(true);
        dbg_ser.printf("rmtshutter shutter open\r\n");
        while ((btnBig_isPressed() || (starting_mf == false && camera.is_focused == false && ((now = millis()) - tstart) < 2000)) && camera.isOperating())
        {
            app_poll();
        }
        if (camera.is_focused) {
            dbg_ser.printf("rmtshutter got focus\r\n");
            camera.wait_while_busy(config_settings.shutter_press_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        }
        camera.cmd_Shutter(false);
        dbg_ser.printf("rmtshutter shutter close\r\n");
    }
    else
    {
        // TODO: in this case, the camera didn't take a photo! what should we do?
        gui_drawVerticalDots(0, 20, -1, 3, time_delay > 2 ? time_delay : 5, 0, false, TFT_RED, TFT_RED);
        dbg_ser.printf("rmtshutter cannot shoot\r\n");
        fail_shown = true;
    }

    if (starting_mf == false) {
        camera.cmd_AutoFocus(false);
        dbg_ser.printf("rmtshutter disable AF\r\n");
    }

    ledblink_off();

    if (fail_shown) {
        app_sleep(300, true);
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void focus_stack(void* mip)
{
    dbg_ser.println("focus_stack");

    if (camera.isOperating() == false) {
        // show user that the camera isn't connected
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    ledblink_setMode(LEDMODE_OFF);

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

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
            ledblink_on();
            camera.cmd_Shoot(config_settings.shutter_press_time_ms);
            ledblink_off();
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
    dbg_ser.println("focus_9point");

    if (camera.isOperating() == false) {
        dbg_ser.println("focus_9point but no camera connected");
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    ledblink_setMode(LEDMODE_OFF);

#ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
#endif

    if (camera.is_spotfocus() == false) {
        // the camera must be in one of the many spot focus modes
        // we can't pick one for them
        // show user the error message
        gui_startAppPrint();
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.println("ERROR:");
        gui_setCursorNextLine();
        M5Lcd.println("not in spot auto-focus mode");
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }

    int dot_rad = 5;
    int dot_space = 30;
    int dot_y_start = (M5Lcd.height() / 2) + 28;
    int dot_x_start = (M5Lcd.width() / 2);
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
        dbg_ser.printf("9point x %u y %u\r\n", x, y);
        camera.cmd_FocusPointSet(x, y);
        ledblink_on();
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
        ledblink_off();
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

        if (btnBig_isPressed() == false) {
            // button check here means we get at least one photo
            break;
        }
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void shutter_step(void* mip)
{
    uint32_t t, now;

    dbg_ser.println("shutter_step");

    if (camera.isOperating() == false) {
        // show user that the camera isn't connected
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

    ledblink_setMode(LEDMODE_OFF);

    bool starting_mf = camera.is_manuallyfocused();

    if (starting_mf == false && camera.isOperating())
    {
        // use manual focus mode to avoid the complexity of waiting for autofocus
        // assume the user has obtained focus already
        camera.cmd_ManualFocusMode(true);
    }

    config_settings.shutter_speed_step_cnt = config_settings.shutter_speed_step_cnt <= 0 ? 1 : config_settings.shutter_speed_step_cnt; // enforce a minimum

    uint32_t cur_ss = camera.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
    uint32_t shutter_ms = shutter_to_millis(cur_ss);

    int dot_idx = 0;

    do
    {
        app_poll();
        if (camera.isOperating() == false) {
            break;
        }
        ledblink_on();

        camera.cmd_Shutter(true);
        t = millis(); now = t;
        while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
            camera.poll();
        }
        camera.cmd_Shutter(false);
        while (((now = millis()) - t) < shutter_ms && camera.isOperating() && btnBig_isPressed()) {
            camera.poll();
        }

        ledblink_off();
        gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, false, TFT_GREEN, TFT_RED);
        dot_idx++;
        cur_ss = camera.get_property_enum(SONYALPHA_PROPCODE_ShutterSpeed, cur_ss, -config_settings.shutter_speed_step_cnt);
        shutter_ms = shutter_to_millis(cur_ss);
        dbg_ser.printf("shutter_step next %u\r\n", shutter_ms);
        camera.wait_while_busy(config_settings.shutter_step_time_ms / 2, DEFAULT_BUSY_TIMEOUT, NULL);
        camera.cmd_ShutterSpeedSet32(cur_ss);
        camera.wait_while_busy(config_settings.shutter_step_time_ms / 2, DEFAULT_BUSY_TIMEOUT, NULL);
    }
    while (btnBig_isPressed() && camera.isOperating());

    uint32_t t_end_1 = millis();

    if (starting_mf == false && camera.isOperating()) {
        // restore AF state
        camera.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        camera.cmd_ManualFocusMode(true);
    }

    // this kinda imposes a minimum button release time
    uint32_t t_end_2 = millis();
    if ((t_end_2 - t_end_1) < 50) {
        app_sleep(50, true);
    }
}

void focus_pull(void* mip)
{
    dbg_ser.println("focus_pull");

    if (camera.isOperating() == false) {
        // show user that the camera isn't connected
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

    ledblink_setMode(LEDMODE_OFF);

    bool starting_mf = camera.is_manuallyfocused();

    if (starting_mf == false && camera.isOperating()) {
        // force into manual focus mode
        camera.cmd_ManualFocusMode(true);
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
    }

    bool do_one = true;

    while ((btnBig_isPressed() || do_one) && camera.isOperating())
    {
        do_one = false;

        app_poll();
        int8_t n = gui_drawFocusPullState(); // return is -3 to +3
        // translate n into Sony's focus step sizes
        if (n >= 0) {
            n = (n ==  2) ?  3 : ((n ==  3) ?  7 : n);
        }
        else {
            n = (n == -2) ? -3 : ((n == -3) ? -7 : n);
        }
        if (n != 0) {
            camera.cmd_ManualFocusStep(n);
            camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        }
    }

    if (starting_mf == false && camera.isOperating()) {
        // restore AF state
        camera.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        camera.cmd_ManualFocusMode(false);
    }
}

void wifi_info(void* mip)
{
    dbg_ser.println("wifi_info");

    gui_startAppPrint();

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
    dbg_ser.println("record_movie");

    ledblink_setMode(LEDMODE_OFF);

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
    return;
    #endif

    camera.cmd_MovieRecordToggle();

    // slight delay, for debouncing, and a chance for the movie recording status to change so we can indicate on-screen
    uint32_t t = millis();
    if ((millis() - t) < 300) {
        app_sleep(50, true);
        gui_drawMovieRecStatus();
    }
    app_waitAllRelease(BTN_DEBOUNCE);
}

void dual_shutter(void* mip)
{
    dbg_ser.println("dual_shutter");

    if (camera.isOperating() == false) {
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
        if (camera.has_property(SONYALPHA_PROPCODE_ShutterSpeed) && camera.has_property(SONYALPHA_PROPCODE_ISO))
        {
            dual_shutter_next = camera.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
            dual_shutter_iso = camera.get_property(SONYALPHA_PROPCODE_ISO);
            dbg_ser.printf("dualshutter 0x%08X %u\r\n", dual_shutter_next, dual_shutter_iso);
        }
        else
        {
            dbg_ser.println("dualshutter no data from camera");
        }
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }

    // if (menuitm->id == MENUITEM_DUALSHUTTER_SHOOT)
    gui_drawTopThickLine(8, TFT_RED);
    dual_shutter_shoot(false, true, dual_shutter_last_tv, dual_shutter_last_iso);
    gui_drawTopThickLine(8, TFT_WHITE);

    app_waitAllRelease(BTN_DEBOUNCE);
}

uint32_t shutter_to_millis(uint32_t x)
{
    uint16_t* p16 = (uint16_t*)&x;
    if (x == 0 || x == 0xFFFFFFFF) {
        return 0;
    }
    uint32_t nn = p16[1];
    uint32_t dd = p16[0];
    nn *= 1000;
    float n = nn;
    float d = dd;
    float y = d != 0 ? (n/d) : 0;
    return lroundf(y);
}

void dual_shutter_shoot(bool already_focused, bool read_button, uint32_t restore_shutter, uint32_t restore_iso)
{
    bool starting_mf = false;
    bool need_restore_af = false;
    bool need_restore_ss = false;
    bool need_restore_iso = false;

    if (already_focused == false) {
        // only care about MF if we are not already focused
        starting_mf = camera.is_manuallyfocused();
    }

    if (already_focused == false && starting_mf == false) {
        // the camera won't actually take a photo if it's not focused (when shutter tries to open, it'll lag to focus)
        // so we turn on AF
        camera.cmd_AutoFocus(true);
        need_restore_af = true;
    }

    camera.cmd_Shutter(true);
    uint32_t t = millis(), now = t;
    uint32_t shutter_ms = shutter_to_millis(restore_shutter == 0 ? camera.get_property(SONYALPHA_PROPCODE_ShutterSpeed) : restore_shutter);
    // first wait is for minimum press time
    while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
        camera.poll();
    }
    // release button and wait for the rest of the time
    camera.cmd_Shutter(false);
    while (((now = millis()) - t) < shutter_ms && camera.isOperating()) {
        camera.poll();
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
    camera.cmd_ShutterSpeedSet32(dual_shutter_next);
    camera.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

    if (dual_shutter_iso != restore_iso) {
        // change ISO if required
        need_restore_iso = true;
        camera.cmd_IsoSet(dual_shutter_iso);
        camera.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
    }

    // start second shot
    camera.cmd_Shutter(true);
    t = millis(); now = t;
    shutter_ms = shutter_to_millis(dual_shutter_next);
    while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
        camera.poll();
    }
    camera.cmd_Shutter(false);
    while (((now = millis()) - t) < shutter_ms && camera.isOperating()) {
        camera.poll();
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
    if (restore_shutter != 0 && need_restore_ss) {
        uint32_t cur_ss;
        do {
            camera.cmd_ShutterSpeedSet32(restore_shutter);
            camera.wait_while_busy(100, DEFAULT_BUSY_TIMEOUT, NULL);
            cur_ss = camera.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
            if (cur_ss == restore_shutter) {
                break;
            }
        }
        while (((now = millis()) - t) < timeout || btnBig_isPressed());
    }
    t = millis(); now = t;
    if (need_restore_iso) {
        uint32_t cur_iso;
        do {
            camera.cmd_IsoSet(restore_iso);
            camera.wait_while_busy(100, DEFAULT_BUSY_TIMEOUT, NULL);
            cur_iso = camera.get_property(SONYALPHA_PROPCODE_ISO);
            if (cur_iso == need_restore_iso) {
                break;
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
            if (camera.get_property(SONYALPHA_PROPCODE_FocusFound) == SONYALPHA_FOCUSSTATUS_NONE) {
                break;
            }
        }
        while (((now = millis()) - t) < timeout);
    }
    if (need_restore_af) {
        camera.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        camera.cmd_AutoFocus(false);
    }
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
}
