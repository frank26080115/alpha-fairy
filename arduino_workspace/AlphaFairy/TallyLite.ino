#include "AlphaFairy.h"

extern bool redraw_flag;
bool tallylite_nopoll = false; // used to prevent recursion in app_poll()
bool tallylite_enable = true;

void tallylite_task()
{
    // prevent recursion
    if (tallylite_nopoll || tallylite_enable == false) {
        return;
    }

    static bool was_recording = false;
    if (config_settings.tallylite == 0) { // feature not used
        return;
    }

    if (fairycam.isOperating() == false) { // no way of getting status
        was_recording = false;
        return;
    }

    bool is_recording = fairycam.is_movierecording();
    if (is_recording && was_recording == false) // transitioning of recording state
    {
        tallylite_show();
    }
    was_recording = fairycam.is_movierecording();
}

void tallylite_show()
{
    M5Lcd.fillScreen(TFT_RED); // red screen
    M5.Axp.ScreenBreath(12);   // super bright
    tallylite_nopoll = true;   // prevent recursion
    while (fairycam.isOperating() && fairycam.is_movierecording()) // quit if disconnected, or recording has stopped
    {
        app_poll();     // this is where recursion is not allowed
        fenc_task();    // allow focus pull while red screen
        pwr_tick(true); // prevent screen dimming and sleep
        if (btnAny_hasPressed())
        {
            // exit on button press
            btnAny_clrPressed();
            break;
        }
    }
    M5.Axp.ScreenBreath(config_settings.lcd_brightness); // normal screen brightness
    tallylite_nopoll = false;
    redraw_flag = true;
}
