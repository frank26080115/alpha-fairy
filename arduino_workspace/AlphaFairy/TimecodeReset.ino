#include "AlphaFairy.h"
#include "FairyMenu.h"
#include <SonyCameraInfraredRemote.h>

class AppTimecodeReset : public FairyMenuItem
{
    public:
        AppTimecodeReset() : FairyMenuItem("/timecode_reset.png")
        {
        };

        virtual bool on_execute(void)
        {
            draw_borderRect(6, TFT_RED);
            SonyCamIr_SendRawBits(IR_CMD_RMT845_TCRESET, 20, 5);
            app_waitAllRelease();
            draw_borderRect(6, TFT_WHITE);
            set_redraw();
            return false;
        };
};

extern FairySubmenu menu_remote;
void setup_timecodeReset()
{
    static AppTimecodeReset app_timecodeReset;
    menu_remote.install(&app_timecodeReset);
}
