#include "AlphaFairy.h"
#include "FairyMenu.h"

class AppFocusStack : public FairyMenuItem
{
    public:
        AppFocusStack() : FairyMenuItem("/focusstack.png")
        {
            _speed = 3;
        };

        virtual void on_spin(int8_t x)
        {
            if (x < 0)
            {
                if (_speed > 1) {
                    _speed--;
                }
            }
            else if (x > 0)
            {
                if (_speed < 5) {
                    _speed++;
                }
            }
            draw_text();
        };

        virtual void on_eachFrame(void)
        {
            gui_drawSpinStatus(5, TFT_WHITE);
            FairyMenuItem::on_eachFrame();
        };

        virtual void on_redraw(void)
        {
            FairyMenuItem::on_redraw();
            draw_text();
        };

        virtual bool on_execute(void)
        {
            if (must_be_ptp() == false) {
                return false;
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

            int step_size, step_cnt, i;
            // pick a step size and step count based on the "speed" indicator
            switch (_speed)
            {
                case 1:
                    step_size = SONYALPHA_FOCUSSTEP_FARTHER_SMALL;
                    step_cnt = 1;
                    break;
                case 2:
                    step_size = SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM;
                    step_cnt = 1;
                    break;
                case 3:
                    step_size = SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM;
                    step_cnt = 3;
                    break;
                case 4:
                    step_size = SONYALPHA_FOCUSSTEP_FARTHER_MEDIUM;
                    step_cnt = 6;
                    break;
                case 5:
                    step_size = SONYALPHA_FOCUSSTEP_FARTHER_LARGE;
                    step_cnt = 1;
                    break;
            }

            int dot_idx = 0;

            // enforce minimum pause based on shutter speed
            uint32_t shutter_speed_time = shutter_to_millis(ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed));
            shutter_speed_time = shutter_speed_time < config_settings.shutter_press_time_ms ? config_settings.shutter_press_time_ms : shutter_speed_time;
            shutter_speed_time += config_settings.focus_pause_time_ms;

            while (ptpcam.isOperating()) // I wish we had try-catches
            {
                app_poll();
                gui_drawVerticalDots(0, 40, -1, 5, 5, dot_idx, step_size > 0, TFT_GREEN, TFT_RED);
                if (is_contshoot == false)
                {
                    uint32_t t = millis();
                    ptpcam.cmd_Shoot(config_settings.shutter_press_time_ms);

                    // enforce minimum pause based on shutter speed
                    while ((millis() - t) < shutter_speed_time) {
                        app_poll();
                    }
                }
                for (i = 0; i < step_cnt; i++)
                {
                    ptpcam.cmd_ManualFocusStep(step_size);
                    ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                    steps_done++;
                }
                dot_idx++;

                if (btnBig_isPressed() == false) {
                    // check button release here so at least one photo is captured
                    break;
                }
            }

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
                    gui_drawVerticalDots(0, 40, -1, 5, 5, dot_idx, step_size < 0, TFT_GREEN, TFT_RED);
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

            set_redraw();
            return false;
        };

    protected:
        uint8_t _speed;

        void draw_text(void)
        {
            gui_startMenuPrint();
            M5Lcd.setCursor(107, 78);
            M5Lcd.printf("%u ", _speed);
        };
};

class AppFocus9Point : public FairyMenuItem
{
    public:
        AppFocus9Point() : FairyMenuItem("/focus_9point.png")
        {
        };

        virtual void on_eachFrame(void)
        {
            gui_drawSpinStatus(5, TFT_WHITE);
            FairyMenuItem::on_eachFrame();
        };

        virtual void on_navTo(void)
        {
            uint32_t x = config_settings.nine_point_dist;
            x *= 100;
            x /= SONYALPHA_FOCUSPOINT_Y_MID;
            x = x >= 95 ? 95: (x <= 20 ? 20 : x);
            _dist = x;
            _dist_start = x;
            FairyMenuItem::on_navTo();
        };

        virtual void on_navOut(void)
        {
            save_if_needed();
            FairyMenuItem::on_navOut();
        };

        virtual void on_redraw(void)
        {
            FairyMenuItem::on_redraw();
            draw_text();
        };

        virtual void on_spin(int8_t x)
        {
            uint8_t pd = _dist;
            if (x > 0)
            {
                if (_dist <= 30) {
                    _dist = 40;
                }
                else if (_dist <= 50) {
                    _dist = 60;
                }
                else if (_dist <= 70) {
                    _dist = 80;
                }
                else if (_dist <= 90) {
                    _dist = 95;
                }
            }
            else if (x < 0)
            {
                if (_dist >= 90) {
                    _dist = 80;
                }
                else if (_dist >= 70) {
                    _dist = 60;
                }
                else if (_dist >= 50) {
                    _dist = 40;
                }
                else if (_dist >= 30) {
                    _dist = 20;
                }
            }
            if (_dist != pd) {
                draw_text();
            }
        };

        virtual bool on_execute(void)
        {
            save_if_needed();

            if (must_be_ptp() == false) {
                return false;
            }

            if (ptpcam.isOperating() && ptpcam.is_spotfocus() == false) {
                // the camera must be in one of the many spot focus modes
                // we can't pick one for them
                // show user the error message
                M5Lcd.drawPngFile(SPIFFS, "/9point_unable.png", 0, 0);
                set_redraw();
                app_waitAllRelease();
                return false;
            }

            int dot_rad = 5;
            int dot_space = 30;
            int dot_y_start = (M5Lcd.height() / 2) + 28;
            int dot_x_start = (M5Lcd.width() / 2);
            // TODO:
            // the starting points of the dots need to be adjusted according to what the JPG looks like

            int x_cent = SONYALPHA_FOCUSPOINT_X_MAX / 2;
            int y_cent = SONYALPHA_FOCUSPOINT_Y_MAX / 2;
            int hdist = _dist;
            int vdist = _dist;
            int x, y, dot_x, dot_y;

            hdist *= SONYALPHA_FOCUSPOINT_X_MID;
            hdist /= 100;
            vdist *= SONYALPHA_FOCUSPOINT_Y_MID;
            vdist /= 100;

            int i;
            for (i = 0; fairycam.isOperating(); i++)
            {
                app_poll();
                // figure out which point to focus on
                switch (i)
                {
                    case 0: x = x_cent;         y = y_cent;         dot_x = dot_x_start;             dot_y = dot_y_start;             break;
                    case 1: x = x_cent + hdist; y = y_cent;         dot_x = dot_x_start + dot_space; dot_y = dot_y_start;             break;
                    case 2: x = x_cent - hdist; y = y_cent;         dot_x = dot_x_start - dot_space; dot_y = dot_y_start;             break;
                    case 3: x = x_cent;         y = y_cent + vdist; dot_x = dot_x_start;             dot_y = dot_y_start + dot_space; break;
                    case 4: x = x_cent;         y = y_cent - vdist; dot_x = dot_x_start;             dot_y = dot_y_start - dot_space; break;
                    case 5: x = x_cent + hdist; y = y_cent + vdist; dot_x = dot_x_start + dot_space; dot_y = dot_y_start + dot_space; break;
                    case 6: x = x_cent + hdist; y = y_cent - vdist; dot_x = dot_x_start + dot_space; dot_y = dot_y_start - dot_space; break;
                    case 7: x = x_cent - hdist; y = y_cent + vdist; dot_x = dot_x_start - dot_space; dot_y = dot_y_start + dot_space; break;
                    case 8: x = x_cent - hdist; y = y_cent - vdist; dot_x = dot_x_start - dot_space; dot_y = dot_y_start - dot_space; break;
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
                fairycam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);

                if (btnBig_isPressed() == false) {
                    // button check here means we get at least one photo
                    break;
                }
            }

            set_redraw();
            return false;
        };

    protected:
        uint8_t _dist = 20;
        uint8_t _dist_start;

        void save_if_needed(void)
        {
            if (_dist_start != _dist) {
                uint32_t x = SONYALPHA_FOCUSPOINT_Y_MID;
                x *= _dist;
                x /= 100;
                config_settings.nine_point_dist = x;
                settings_save();
                _dist_start = _dist;
            }
        };

        void draw_text(void)
        {
            M5Lcd.setTextFont(2);
            M5Lcd.highlight(true);
            M5Lcd.setTextWrap(false);
            M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
            M5Lcd.setHighlightColor(TFT_WHITE);
            M5Lcd.setCursor(40, 194);
            M5Lcd.printf("spacing: %u%%", _dist);
            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), TFT_WHITE);
        };
};

extern FairySubmenu menu_focus;
void setup_focusstack()
{
    static AppFocusStack  app_fstack;
    static AppFocus9Point app_f9pt;
    menu_focus.install(&app_fstack);
    menu_focus.install(&app_f9pt);
}
