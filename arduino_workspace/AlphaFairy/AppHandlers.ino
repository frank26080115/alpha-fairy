#include "AlphaFairy.h"

//#define APP_DOES_NOTHING // this is only used for testing purposes

uint8_t remoteshutter_delay = 2;

extern bool gui_microphoneActive;

void cam_shootQuick()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (ptpcam.isOperating()) {
        ptpcam.cmd_Shoot(config_settings.shutter_press_time_ms);
    }
    else if (httpcam.isOperating()) {
        httpcam.cmd_Shoot();
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
    if (ptpcam.isOperating()) {
        ptpcam.cmd_Shutter(true);
        if (gpio_time != 0) {
            #ifdef SHUTTER_GPIO_ACTIVE_HIGH
            digitalWrite(SHUTTER_GPIO, LOW);
            #endif
            pinMode(SHUTTER_GPIO, INPUT);
            gpio_time = 0;
        }
    }
    else if (httpcam.isOperating()) {
        httpcam.cmd_Shoot();
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
    if (ptpcam.isOperating()) {
        ptpcam.cmd_Shutter(false);
        if (gpio_time != 0) {
            #ifdef SHUTTER_GPIO_ACTIVE_HIGH
            digitalWrite(SHUTTER_GPIO, LOW);
            #endif
            pinMode(SHUTTER_GPIO, INPUT);
            gpio_time = 0;
        }
    }
    else if (httpcam.isOperating()) {
        // do nothing
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
    if (mip != NULL)
    {
        // determine time delay based on which menu item was used
        if (menuitm->id == MENUITEM_REMOTESHUTTER_NOW) {
            time_delay = 0;
        }
        else if (menuitm->id == MENUITEM_REMOTESHUTTER_DLY) {
            time_delay = remoteshutter_delay;
        }
    }

    if (fairycam.isOperating() == false)
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
                    if (btnSide_hasPressed()) {
                        quit = true;
                        break;
                    }
                }
                btnSide_clrPressed();
            }
            dbg_ser.println("shoot");
            //cam_shootQuickGpio();
            cam_shootOpen();
            tstart = millis();
            while (((tdiff = ((now = millis()) - tstart))) < config_settings.shutter_press_time_ms || btnBig_isPressed()) {
                app_poll();
            }
            cam_shootClose();
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

    bool starting_mf = fairycam.is_manuallyfocused();
    bool starting_mf_ignore = false;
    if (httpcam.isOperating() && httpcam.is_manuallyfocused() == SHCAM_FOCUSMODE_NONE) {
        starting_mf_ignore = true;
    }

    // start focusing at the beginning of the countdown
    if (starting_mf == false && starting_mf_ignore == false) {
        dbg_ser.printf("rmtshutter AF\r\n");
        fairycam.cmd_AutoFocus(true);
    }

    tstart = millis();
    now = tstart;
    quit = false;
    if (time_delay > 0)
    {
        dbg_ser.printf("rmtshutter wait delay %u... ", time_delay);
        // wait the countdown time
        while (((tdiff = ((now = millis()) - tstart)) < (time_delay * 1000)) && fairycam.isOperating()) {
            if (app_poll()) {
                if (time_delay > 2) {
                    gui_drawVerticalDots(0, 20, -1, 3, time_delay, tdiff / 1000, false, TFT_DARKGREEN, TFT_RED);
                }
            }
            if (btnSide_hasPressed()) {
                quit = true;
                break;
            }
        }
        btnSide_clrPressed();

        // if user cancelled
        if (quit) {
            dbg_ser.printf(" user cancelled\r\n");
            // end autofocus
            if (starting_mf == false && starting_mf_ignore == false) {
                fairycam.cmd_AutoFocus(false);
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
    if ((ptpcam.isOperating() && ptpcam.is_focused) || starting_mf)
    {
        // if the camera is focused or MF, then the shutter should immediately take the picture
        ptpcam.cmd_Shoot(config_settings.shutter_press_time_ms);
        dbg_ser.printf("rmtshutter shoot\r\n");
    }
    else if (btnBig_isPressed() || time_delay == 0)
    {
        // if the camera is not focused, we try to take the shot anyways if the user is holding the button
        if (ptpcam.isOperating())
        {
            ptpcam.cmd_Shutter(true);
            dbg_ser.printf("rmtshutter shutter open\r\n");
            while ((btnBig_isPressed() || (starting_mf == false && ptpcam.is_focused == false && ((now = millis()) - tstart) < 2000)) && ptpcam.isOperating())
            {
                app_poll();
            }
            if (ptpcam.is_focused) {
                dbg_ser.printf("rmtshutter got focus\r\n");
                ptpcam.wait_while_busy(config_settings.shutter_press_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
            }
            ptpcam.cmd_Shutter(false);
            dbg_ser.printf("rmtshutter shutter close\r\n");
        }
        else if (httpcam.isOperating())
        {
            httpcam.cmd_Shoot();
        }
    }
    else if (gui_microphoneActive == false)
    {
        // TODO: in this case, the camera didn't take a photo! what should we do?
        gui_drawVerticalDots(0, 20, -1, 3, time_delay > 2 ? time_delay : 5, 0, false, TFT_RED, TFT_RED);
        dbg_ser.printf("rmtshutter cannot shoot\r\n");
        fail_shown = true;
    }

    if (starting_mf == false && starting_mf_ignore == false) {
        fairycam.cmd_AutoFocus(false);
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

    if (ptpcam.isOperating() == false) {
        if (httpcam.isOperating()) {
            app_waitAllReleaseUnsupported(BTN_DEBOUNCE);
        }
        else {
            // show user that the camera isn't connected
            app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        }
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

    bool starting_mf = ptpcam.is_manuallyfocused();

    if (starting_mf == false && ptpcam.isOperating())
    {
        // obviously we need to be in manual focus mode in order to do focus stacking
        ptpcam.cmd_ManualFocusMode(true, false);
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

        if (ptpcam.has_property(SONYALPHA_PROPCODE_ManualFocusDist)) {
            starting_focus_dist = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        }
        else {
            starting_focus_dist = -1;
        }
    }

    // configuring the shutter_press_time_ms to be 0 means hold the button down as the focus is stepped
    // this will work when the camera is in continuous shooting mode
    // but I can't guarantee that the photos are one-step-one-frame
    bool is_contshoot = false;
    if (config_settings.shutter_press_time_ms == 0 && ptpcam.is_continuousshooting() && ptpcam.isOperating())
    {
        is_contshoot = true;
        ptpcam.cmd_Shutter(true);
    }

    int dot_idx = 0;

    while (ptpcam.isOperating()) // I wish we had try-catches
    {
        app_poll();
        gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, step_size > 0, TFT_GREEN, TFT_RED);
        if (is_contshoot == false)
        {
            ledblink_on();
            ptpcam.cmd_Shoot(config_settings.shutter_press_time_ms);
            ledblink_off();
        }
        ptpcam.cmd_ManualFocusStep(step_size);
        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        steps_done++;
        dot_idx++;

        if (btnBig_isPressed() == false) {
            // check button release here so at least one photo is captured
            break;
        }
    }

    uint32_t t_end_1 = millis();

    if (is_contshoot && ptpcam.isOperating())
    {
        ptpcam.cmd_Shutter(false);
    }

    if (starting_focus_dist >= 0)
    {
        // this loop is what restores the focus point back to the starting focus point
        int x = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
        do
        {
            app_poll();
            gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, step_size < 0, TFT_GREEN, TFT_RED);
            ptpcam.cmd_ManualFocusStep(step_size * -1);
            steps_done--;
            ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
            x = ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist);
            dot_idx++;
        }
        while (
                true // ignore me
                && ((x > starting_focus_dist && step_size > 0) || (x < starting_focus_dist && step_size < 0)) // soft limit on the distance reported
                && steps_done >= 0 // hard limit on the number of steps
                && ptpcam.isOperating()
                );
    }

    if (starting_mf == false && ptpcam.isOperating())
    {
        // if the camera was set to MF mode via PTP, then it will be stuck in MF mode even if the user toggle switches on the camera or lens!
        // so we MUST restore the setting via PTP
        ptpcam.cmd_ManualFocusMode(false, false);
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

    if (fairycam.isOperating() == false) {
        dbg_ser.println("focus_9point but no camera connected");
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    ledblink_setMode(LEDMODE_OFF);

#ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
#endif

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
    for (i = 0; fairycam.isOperating(); i++)
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
        fairycam.cmd_FocusPointSet(x, y);
        ledblink_on();
        if (fairycam.need_wait_af())
        {
            fairycam.cmd_AutoFocus(true);
            fairycam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

            if (httpcam.isOperating()) {
                httpcam.setPollDelay(0);
            }

            // wait for autofocus to lock onto something
            uint32_t tstart = millis();
            uint32_t now;
            uint32_t tlimit = (i < 9) ? 5000 : 2000;
            while (((now = millis()) - tstart) < 5000 && (fairycam.is_focused() == false && fairycam.isOperating()))
            {
                app_poll();
            }
        }
        // take the photo
        fairycam.cmd_Shoot(config_settings.shutter_press_time_ms);
        fairycam.cmd_AutoFocus(false);
        ledblink_off();
        fairycam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

        if (btnBig_isPressed() == false) {
            // button check here means we get at least one photo
            break;
        }
    }

    if (httpcam.isOperating()) {
        httpcam.setPollDelaySlow();
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void shutter_step(void* mip)
{
    uint32_t t, now;

    dbg_ser.println("shutter_step");

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

    bool starting_mf = fairycam.is_manuallyfocused();

    if (starting_mf == false && fairycam.isOperating())
    {
        // use manual focus mode to avoid the complexity of waiting for autofocus
        // assume the user has obtained focus already
        fairycam.cmd_ManualFocusMode(true, false);
    }

    config_settings.shutter_speed_step_cnt = config_settings.shutter_speed_step_cnt <= 0 ? 1 : config_settings.shutter_speed_step_cnt; // enforce a minimum

    uint32_t cur_ss;
    int cur_ss_idx;
    char next_ss[32] = {0};
    if (ptpcam.isOperating()) {
        cur_ss = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
    }
    else if (httpcam.isOperating()) {
        cur_ss = httpcam.get_shutterspd_32();
        cur_ss_idx = httpcam.get_shutterspd_idx();
    }

    uint32_t shutter_ms = shutter_to_millis(cur_ss);

    int dot_idx = 0;

    do
    {
        app_poll();
        if (fairycam.isOperating() == false) {
            break;
        }
        ledblink_on();

        fairycam.cmd_Shutter(true);
        t = millis(); now = t;
        while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
            ptpcam.poll();
            httpcam.poll();
        }
        fairycam.cmd_Shutter(false);
        while (((now = millis()) - t) < shutter_ms && fairycam.isOperating() && btnBig_isPressed()) {
            ptpcam.poll();
            httpcam.poll();
        }

        ledblink_off();
        gui_drawVerticalDots(0, 20, -1, 3, 5, dot_idx, false, TFT_GREEN, TFT_RED);
        dot_idx++;
        if (ptpcam.isOperating()) {
            cur_ss = ptpcam.get_property_enum(SONYALPHA_PROPCODE_ShutterSpeed, cur_ss, -config_settings.shutter_speed_step_cnt);
        }
        else if (httpcam.isOperating()) {
            cur_ss_idx -= config_settings.shutter_speed_step_cnt;
            if (cur_ss_idx <= 0) {
                cur_ss_idx = 0;
            }
            cur_ss = httpcam.get_another_shutterspd(cur_ss_idx, next_ss);
        }
        shutter_ms = shutter_to_millis(cur_ss);
        dbg_ser.printf("shutter_step next %u\r\n", shutter_ms);
        fairycam.wait_while_busy(config_settings.shutter_step_time_ms / 2, DEFAULT_BUSY_TIMEOUT, NULL);
        if (ptpcam.isOperating()) {
            ptpcam.cmd_ShutterSpeedSet32(cur_ss);
        }
        else if (httpcam.isOperating()) {
            httpcam.cmd_ShutterSpeedSetStr(next_ss);
        }
        fairycam.wait_while_busy(config_settings.shutter_step_time_ms / 2, DEFAULT_BUSY_TIMEOUT, NULL);
    }
    while (btnBig_isPressed() && ptpcam.isOperating());

    uint32_t t_end_1 = millis();

    if (starting_mf == false && fairycam.isOperating()) {
        // restore AF state
        fairycam.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        fairycam.cmd_ManualFocusMode(true, false);
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

    if (ptpcam.isOperating() == false) {
        if (httpcam.isOperating()) {
            app_waitAllReleaseUnsupported(BTN_DEBOUNCE);
        }
        else {
            // show user that the camera isn't connected
            app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        }
        return;
    }

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

    ledblink_setMode(LEDMODE_OFF);

    bool starting_mf = ptpcam.is_manuallyfocused();

    if (starting_mf == false && ptpcam.isOperating()) {
        // force into manual focus mode
        ptpcam.cmd_ManualFocusMode(true, false);
        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
    }

    bool do_one = true;

    while ((btnBig_isPressed() || do_one) && ptpcam.isOperating())
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
            ptpcam.cmd_ManualFocusStep(n);
            ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        }
    }

    if (starting_mf == false && ptpcam.isOperating()) {
        // restore AF state
        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
        ptpcam.cmd_ManualFocusMode(false, false);
    }
}

void zoom_pull(void* mip)
{
    dbg_ser.println("zoom_pull");

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

    bool do_one = true;

    while ((btnBig_isPressed() || do_one) && fairycam.isOperating())
    {
        do_one = false;

        app_poll();
        int8_t n = gui_drawFocusPullState(); // return is -3 to +3
        int8_t na = n < 0 ? (-n) : n;
        uint32_t dly = config_settings.focus_pause_time_ms; // the pause between steps is shorter for higher tilt
        if (na == 1) {
            dly += 200;
        }
        if (ptpcam.isOperating())
        {
            if (n != 0) {
                ptpcam.cmd_ZoomStep((n > 0) ? -1 : +1);
                ptpcam.wait_while_busy(dly, DEFAULT_BUSY_TIMEOUT, NULL);
            }
        }
        else if (httpcam.isOperating())
        {
            httpcam.cmd_ZoomStart(n);
            httpcam.wait_while_busy(dly, DEFAULT_BUSY_TIMEOUT, NULL);
        }
    }

    if (httpcam.isOperating()) {
        httpcam.cmd_ZoomStop();
    }
}

menustate_t menustate_wifiinfo;

void wifi_info(void* mip)
{
    dbg_ser.println("wifi_info");

    bool redraw = true; // always draw on entry
    bool first = true;  // this helps the button release
    bool conn_state = ptpcam.isOperating();
    menustate_t* m = &menustate_wifiinfo;
    m->idx = 0;
    m->cnt = 4;

    btnAll_clrPressed();
    M5Lcd.fillScreen(TFT_WHITE);

    while (true)
    {
        app_poll();
        pwr_tick();

        // redraw if connection changed
        redraw |= fairycam.isOperating() != conn_state;
        conn_state = fairycam.isOperating();

        if (btnBig_hasPressed())
        {
            btnAll_clrPressed();
            m->idx = (m->idx + 1) % m->cnt;
            redraw = true;
        }

        if (btnSide_hasPressed())
        {
            btnAll_clrPressed();
            #if defined(USE_PWR_BTN_AS_BACK) && !defined(USE_PWR_BTN_AS_EXIT)
            m->idx = (m->idx + 1) % m->cnt;
            #else
            m->idx++;
            if (m->idx >= m->cnt) {
                m->idx = 0;
                break;
            }
            #endif
            redraw = true;
        }

        if (btnPwr_hasPressed())
        {
            btnAll_clrPressed();
            break;
        }

        if (m->idx == 1 && fairycam.isOperating() == false) {
            m->idx = 2;
        }

        if (redraw)
        {
            M5Lcd.drawPngFile(SPIFFS, "/wifiinfo_head.png", 0, 0);
            M5Lcd.fillRect(0, 50, M5Lcd.width(), M5Lcd.height() - 16 - 50, TFT_WHITE);
            gui_drawStatusBar(false);
            if (m->idx == 0 || m->idx == 1 || m->idx >= m->cnt)
            {
                int line_space = 16;
                int top_margin = 7;
                int left_margin = 55;
                M5Lcd.setRotation(1);
                M5Lcd.highlight(true);
                M5Lcd.setTextWrap(true);
                M5Lcd.setHighlightColor(TFT_WHITE);
                M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
                M5Lcd.setCursor(left_margin, top_margin);
                M5Lcd.setTextFont(2);
                M5Lcd.print("Wi-Fi SSID: ");
                M5Lcd.setCursor(left_margin + 8, top_margin + (line_space * 1));
                M5Lcd.print(NetMgr_getSSID()); gui_blankRestOfLine();
                M5Lcd.setCursor(left_margin, top_margin + (line_space * 2));
                M5Lcd.print("password: ");
                M5Lcd.setCursor(left_margin + 8, top_margin + (line_space * 3));
                M5Lcd.print(NetMgr_getPassword()); gui_blankRestOfLine();
                M5Lcd.setCursor(left_margin, top_margin + (line_space * 4));
                if (m->idx != 1)
                {
                    M5Lcd.print("URL: ");
                    M5Lcd.setCursor(left_margin + 8, top_margin + (line_space * 5));
                    M5Lcd.print("http://");
                    M5Lcd.print(WiFi.softAPIP());
                    M5Lcd.print("/");
                }
                else if (m->idx == 1)
                {
                    M5Lcd.print("Camera: ");
                    M5Lcd.setCursor(left_margin + 8, top_margin + (line_space * 5));
                    M5Lcd.print(IPAddress(fairycam.getIp()));
                    char* cam_name = fairycam.getCameraName();
                    if (cam_name != NULL && strlen(cam_name) > 0) {
                        M5Lcd.setCursor(left_margin + 8, top_margin + (line_space * 6));
                        M5Lcd.print(cam_name);
                    }
                }
                M5Lcd.setRotation(0);
            }
            else if (m->idx == 2 || m->idx == 3)
            {
                // draw a header quickly first
                M5Lcd.drawPngFile(SPIFFS, m->idx == 2 ? "/wifiinfo_login.png" : "/wifiinfo_url.png", 0, 0);
                char qrstr[64];
                uint32_t width = 124;
                uint32_t x = (M5Lcd.width()  - width + 1) / 2;
                uint32_t y = (M5Lcd.height() - width + 1) / 2;
                y += 20;
                // generate the QR code
                if (m->idx == 2) {
                    sprintf(qrstr, "WIFI:T:WPA;S:%s;P:%s;;", NetMgr_getSSID(), NetMgr_getPassword());
                }
                else if (m->idx == 3) {
                    sprintf(qrstr, "http://%s/", WiFi.softAPIP());
                }
                M5Lcd.qrcode(qrstr, x, y, width, 7);
            }
            redraw = false;
            if (first) {
                app_waitAllRelease(BTN_DEBOUNCE);
            }
            first = false;
        }
        gui_drawStatusBar(false);
        app_sleep(BTN_DEBOUNCE, false); // this screen is way too jumpy with the button bounces
    }
    app_waitAllRelease(BTN_DEBOUNCE);
}

void record_movie(void* mip)
{
    dbg_ser.println("record_movie");

    ledblink_setMode(LEDMODE_OFF);

    if (fairycam.isOperating() == false)
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

    fairycam.cmd_MovieRecordToggle();

    // slight delay, for debouncing, and a chance for the movie recording status to change so we can indicate on-screen
    uint32_t t = millis();
    if ((millis() - t) < 300) {
        app_sleep(50, true);
        gui_drawMovieRecStatus();
    }
    app_waitAllRelease(BTN_DEBOUNCE);
}
