#include "AlphaFairy.h"
#include "FairyMenu.h"

class AppFocusFrustration : public FairyMenuItem
{
    public:
        AppFocusFrustration() : FairyMenuItem("/focus_frust.png")
        {
        };

        virtual void on_navTo(void)
        {
            var_reset();
            FairyMenuItem::on_navTo();
        }

        virtual void on_eachFrame(void)
        {
            if (ptpcam.isOperating() == false) {
                var_reset();
                return;
            }
            if (ptpcam.has_property(SONYALPHA_PROPCODE_FocusFound) == false) {
                return;
            }

            cpufreq_boost();

            uint32_t x;

            /*
            track the card space and buffer space, reset the tap counter if the user actually takes a photo
            */
            uint32_t cardspace = 0;

            if (ptpcam.has_property(SONYALPHA_PROPCODE_ObjectInMemory))
            {
                x = ptpcam.get_property(SONYALPHA_PROPCODE_ObjectInMemory);
                if (x != _objinmem) {
                    // buffer space has changed
                    _cnt = 0;
                    _objinmem = x;
                    dbg_ser.printf("focusfrust obj in mem %u\r\n", x);
                }
            }

            if (ptpcam.has_property(SONYALPHA_PROPCODE_MemoryRemaining_Card1))
            {
                x = ptpcam.get_property(SONYALPHA_PROPCODE_MemoryRemaining_Card1);
                cardspace += x;
            }
            if (ptpcam.has_property(SONYALPHA_PROPCODE_MemoryRemaining_Card2))
            {
                x = ptpcam.get_property(SONYALPHA_PROPCODE_MemoryRemaining_Card2);
                cardspace += x;
            }
            if (cardspace != _cardrem) {
                // cardspace has changed
                _cnt = 0;
                _cardrem = cardspace;
                dbg_ser.printf("focusfrust card space %u\r\n", cardspace);
            }

            x = ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound);
            uint32_t now = millis();
            if (x != SONYALPHA_FOCUSSTATUS_NONE)
            {
                // shutter button half press
                if (_state_prev == false) {
                    _last_time_on = now;
                    pwr_tick(true);
                    dbg_ser.printf("focusfrust pressed %u\r\n", now);
                }
                _state_prev = true;
            }
            else
            {
                // shutter button released
                if (_state_prev)
                {
                    pwr_tick(true);
                    if ((now - _last_time_on) < 500 && (now - _last_time_off) < 1000)
                    {
                        // the tap was rapid enough
                        dbg_ser.printf("focusfrust released %u\r\n", now);
                        _cnt += 1;
                    }
                    else
                    {
                        _cnt = 0;
                    }
                    _last_time_off = now;
                }
                _state_prev = false;
            }

            if (_cnt >= 4)
            {
                dbg_ser.println("focusfrust cnt reached limit");
                M5Lcd.fillRect(0, 0, M5Lcd.width(), 12, TFT_RED);
                execute();
                _cnt = 0;
                M5Lcd.fillRect(0, 0, M5Lcd.width(), 12, TFT_WHITE);
            }
        };

        virtual bool on_execute(void)
        {
            if (must_be_connected() == false) {
                return false;
            }

            execute();

            return false;
        };

    protected:

        bool     _state_prev    = false;
        uint32_t _last_time_on  = 0;
        uint32_t _last_time_off = 0;
        uint8_t  _cnt           = 0;
        uint32_t _objinmem      = 0;
        uint32_t _cardrem       = 0;

        void var_reset(void)
        {
            _state_prev    = false;
            _last_time_on  = 0;
            _last_time_off = 0;
            _cnt = 0;
        };

        void execute(void)
        {
            cpufreq_boost();

            // obviously we need to be in manual focus mode in order to change focus
            bool starting_mf = fairycam.is_manuallyfocused();
            if (starting_mf == false && fairycam.isOperating()) {
                fairycam.cmd_ManualFocusMode(true, false);
            }

            int step_cnt = 0;

            ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT);
            for (int i = 0; i < 10 || btnBig_isPressed(); i++) {
                gui_drawVerticalDots(0, 40, -1, 5, 5, step_cnt++, false, TFT_GREEN, TFT_RED);
                ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_LARGE);
                ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT);
            }

            // return to AF mode
            if (starting_mf == false) {
                ptpcam.cmd_ManualFocusMode(false, false);
            }
            set_redraw();
        };
};

extern FairySubmenu menu_focus;
void setup_focusfrustration()
{
    static AppFocusFrustration app;
    menu_focus.install(&app);
}
