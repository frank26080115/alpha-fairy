#include "AlphaFairy.h"
#include "FairyMenu.h"

extern int32_t infoscr_reqShutter;

class AppShutterStep : public FairyMenuItem
{
    public:
        AppShutterStep() : FairyMenuItem("/shutter_step.png")
        {
        };

        virtual void on_eachFrame(void)
        {
            gui_drawSpinStatus(5, TFT_WHITE);
            FairyMenuItem::on_eachFrame();
        };

        virtual void on_navTo(void)
        {
            _step_cnt = config_settings.shutter_speed_step_cnt;
            FairyMenuItem::on_navTo();
        };

        virtual void on_navOut(void)
        {
            save_if_needed();
            FairyMenuItem::on_navOut();
        };

        virtual void on_spin(int8_t x)
        {
            int prev = _step_cnt;
            _step_cnt = _step_cnt <= 0 ? 1 : _step_cnt; // enforce a minimum
            if (x > 0)
            {
                if (_step_cnt <= 9) {
                    _step_cnt++;
                }
            }
            else if (x < 0)
            {
                if (_step_cnt > 1) {
                    _step_cnt--;
                }
            }
            if (prev != _step_cnt) {
                draw_text();
            }
        };

        virtual void on_redraw(void)
        {
            FairyMenuItem::on_redraw();
            draw_text();
        };

        virtual bool on_execute(void)
        {
            if (must_be_connected() == false) {
                return false;
            }

            uint32_t expo_mode = fairycam.get_exposureMode();
            if (expo_mode != SONYALPHA_EXPOMODE_M && expo_mode != SONYALPHA_EXPOMODE_S)
            {
                M5Lcd.drawPngFile(SPIFFS, "/tvstep_unable.png", 0, 0);
                app_waitAllRelease();
                set_redraw();
                return false;
            }

            save_if_needed();

            bool toggle_button = imu.rolli < -60; // hold the device upside down

            uint32_t t, now;

            bool starting_mf = fairycam.is_manuallyfocused();

            if (starting_mf == false && fairycam.isOperating())
            {
                // use manual focus mode to avoid the complexity of waiting for autofocus
                // assume the user has obtained focus already
                fairycam.cmd_ManualFocusMode(true, false);
            }

            _step_cnt = _step_cnt <= 0 ? 1 : _step_cnt; // enforce a minimum

            uint32_t cur_ss;

            // get the current shutter speed
            if (ptpcam.isOperating()) {
                cur_ss = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
            }
            else if (httpcam.isOperating()) {
                cur_ss = httpcam.get_shutterspd_32();
            }
            infoscr_reqShutter = cur_ss;

            uint32_t shutter_ms = shutter_to_millis(cur_ss);

            int dot_idx = 0;

            do
            {
                app_poll();
                if (fairycam.isOperating() == false) {
                    break;
                }

                fairycam.cmd_Shutter(true);
                t = millis(); now = t;
                while (((now = millis()) - t) < shutter_ms && (now - t) < config_settings.shutter_press_time_ms) {
                    ptpcam.poll();
                    httpcam.poll();
                    cpufreq_boost();
                }
                fairycam.cmd_Shutter(false);
                // wait the amount of time that the shutter speed says we need to pause for
                while (((now = millis()) - t) < shutter_ms && fairycam.isOperating() && btnBig_isPressed()) {
                    ptpcam.poll();
                    httpcam.poll();
                    cpufreq_boost();
                }

                gui_drawVerticalDots(0, 40, -1, 5, 5, dot_idx, false, TFT_GREEN, TFT_RED);
                dot_idx++;

                fairycam.wait_while_saving(config_settings.shutter_step_time_ms, 500, DEFAULT_SAVE_TIMEOUT);

                infoscr_changeVal(EDITITEM_SHUTTER, -_step_cnt);

                // calculate the next shutter speed
                if (ptpcam.isOperating()) {
                    cur_ss = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
                }
                else if (httpcam.isOperating()) {
                    cur_ss = httpcam.get_shutterspd_32();
                }
                shutter_ms = shutter_to_millis(cur_ss);
                dbg_ser.printf("shutter_step next %u\r\n", shutter_ms);

                if (btnBig_isPressed() == false && toggle_button == false) {
                    // check button release here so at least one photo is captured
                    break;
                }
                else if (toggle_button) {
                    // in toggle mode, the button might actually be released already, but we still run until the next button press
                    if (btnAny_hasPressed()) {
                        btnAny_clrPressed();
                        break;
                    }
                }
            }
            while (fairycam.isOperating());

            if (starting_mf == false && fairycam.isOperating()) {
                // restore AF state
                fairycam.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT);
                fairycam.cmd_ManualFocusMode(true, false);
            }

            set_redraw();
            return false;
        };

    protected:
        int8_t _step_cnt;

        void draw_text(void)
        {
            gui_startMenuPrint();
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(55, 200);
            M5Lcd.printf("%u ", _step_cnt);
        };

        void save_if_needed(void)
        {
            if (_step_cnt != config_settings.shutter_speed_step_cnt) {
                config_settings.shutter_speed_step_cnt = _step_cnt;
                settings_save();
            }
        };
};

extern FairySubmenu menu_focus;
void setup_shutterstep()
{
    static AppShutterStep app;
    menu_focus.install(&app);
}
