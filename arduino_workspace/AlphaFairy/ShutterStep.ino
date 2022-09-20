#include "AlphaFairy.h"
#include "FairyMenu.h"

class AppShutterStep : public FairyMenuItem
{
    public:
        AppShutterStep() : FairyMenuItem("/shutter_step.png")
        {
        };

        virtual bool on_execute(void)
        {
            if (must_be_connected() == false) {
                return false;
            }

            uint32_t t, now;

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

            // get the current shutter speed
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

                // calculate the next shutter speed to change to
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

                // TODO: maybe wait here a bit longer? maybe we need to check for buffer state?

                // set the new shutter speed
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

            if (starting_mf == false && fairycam.isOperating()) {
                // restore AF state
                fairycam.wait_while_busy(config_settings.shutter_step_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
                fairycam.cmd_ManualFocusMode(true, false);
            }

            set_redraw();
            return false;
        };
};

extern FairySubmenu menu_focus;
void setup_shutterstep()
{
    static AppShutterStep app;
    menu_focus.install(&app);
}
