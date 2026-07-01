#include "AlphaFairy.h"
#include "FairyMenu.h"

void focus_pull(bool live, int bar_y)
{
    bool starting_mf = ptpcam.is_manuallyfocused();

    if (starting_mf == false && ptpcam.isOperating()) {
        // force into manual focus mode
        ptpcam.cmd_ManualFocusMode(true, false);
        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT);
    }

    int8_t step_size = focus_tiltToStepSize(gui_drawFocusPullState(bar_y));

    while (ptpcam.isOperating())
    {
        app_poll();

        if (live) {
            step_size = focus_tiltToStepSize(gui_drawFocusPullState(bar_y));
        }

        if (step_size != 0)
        {
            ptpcam.cmd_ManualFocusStep(step_size);
            ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT);
        }

        if (btnBig_isPressed() == false)
        {
            // checking last makes sure at least one focus step is done
            break;
        }
    }
    
    if (starting_mf == false && ptpcam.isOperating()) {
        // restore AF state
        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT);
        ptpcam.cmd_ManualFocusMode(false, false);
    }
}

class AppFocusPull : public FairyMenuItem
{
    public:
        AppFocusPull() : FairyMenuItem("/focus_pull.png")
        {
            _dir = 0;
        };

        virtual void on_eachFrame(void)
        {
            _dir = gui_drawFocusPullState(_bar_y);
        };

        virtual bool on_execute(void)
        {
            if (must_be_ptp() == false) {
                return false;
            }

            if (_dir == 0) {
                app_waitAllRelease();
                return false;
            }

            focus_pull(false, _bar_y);

            return false;
        };

    protected:
        const int16_t _bar_y = 108;
        int8_t _dir;
};

extern FairySubmenu menu_focus;
void setup_focuspull()
{
    static AppFocusPull app;
    menu_focus.install(&app);
}
