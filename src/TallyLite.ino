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
    if (config_settings.tallylite == TALLYLITE_OFF) { // feature not used
        tallylite_ledOff();
        return;
    }

    if (fairycam.isOperating() == false) { // no way of getting status
        was_recording = false;
        tallylite_ledOff();
        return;
    }

    bool is_recording = fairycam.is_movierecording();
    if (is_recording && was_recording == false) // transitioning of recording state
    {
        if (config_settings.tallylite == TALLYLITE_LED || config_settings.tallylite == TALLYLITE_BOTH) {
            tallylite_ledOn();
        }
        if (config_settings.tallylite == TALLYLITE_SCREEN || config_settings.tallylite == TALLYLITE_BOTH) {
            tallylite_show();
        }
    }
    was_recording = fairycam.is_movierecording();
    if (was_recording == false)
    {
        tallylite_ledOff();
    }
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

bool tallylite_ledIsOn = false;

void tallylite_ledOn()
{
    pinMode(10, OUTPUT);
    digitalWrite(10, LOW);
    tallylite_ledIsOn = true;
}

void tallylite_ledOff()
{
    if (tallylite_ledIsOn)
    {
        pinMode(10, INPUT);
        digitalWrite(10, HIGH);
    }
    tallylite_ledIsOn = false;
}
