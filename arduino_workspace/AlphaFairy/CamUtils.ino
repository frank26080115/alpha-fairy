#include "AlphaFairy.h"

void cam_shootQuick()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (ptpcam.isOperating()) {
        ptpcam.cmd_Shoot(config_settings.shutter_press_time_ms);
    }
    else if (httpcam.isOperating()) {
        httpcam.cmd_Shoot();
    }
    else
    {
        if (config_settings.pin_shutter != PINCFG_NONE && config_settings.pin_shutter != config_settings.pin_exinput) {
            cam_shootQuickGpio();
        }
        else if (config_settings.infrared_enabled) {
            SonyCamIr_Shoot();
        }
    }
}

void cam_shootQuickGpio()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    int32_t pin = get_pinCfgGpio(config_settings.pin_shutter);
    if (pin < 0) {
        return;
    }
    #ifdef SHUTTER_GPIO_ACTIVE_HIGH
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    app_sleep(config_settings.shutter_press_time_ms, false);
    digitalWrite(pin, LOW);
    #else
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    app_sleep(config_settings.shutter_press_time_ms, false);
    pinMode(pin, INPUT);
    #endif
}

void cam_shootOpen()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (ptpcam.isOperating())
    {
        ptpcam.cmd_Shutter(true);
        if (gpio_time != 0)
        {
            int32_t pin = get_pinCfgGpio(config_settings.pin_shutter);
            if (pin >= 0)
            {
                #ifdef SHUTTER_GPIO_ACTIVE_HIGH
                digitalWrite(pin, LOW);
                #endif
                pinMode(pin, INPUT);
            }
            gpio_time = 0;
        }
    }
    else if (httpcam.isOperating()) {
        httpcam.cmd_Shoot();
    }
    else
    {
        if (config_settings.pin_shutter != PINCFG_NONE && config_settings.pin_shutter != config_settings.pin_exinput)
        {
            int32_t pin = get_pinCfgGpio(config_settings.pin_shutter);
            if (pin >= 0)
            {
                #ifdef SHUTTER_GPIO_ACTIVE_HIGH
                pinMode(pin, OUTPUT);
                digitalWrite(pin, HIGH);
                #else
                digitalWrite(pin, LOW);
                pinMode(pin, OUTPUT);
                digitalWrite(pin, LOW);
                #endif
            }
            gpio_time = millis();
        }
        else if (config_settings.infrared_enabled) {
            SonyCamIr_Shoot();
        }
    }
}

void cam_shootClose()
{
    // convenience function for quickly taking a photo without complicated connectivity checks
    if (ptpcam.isOperating()) {
        ptpcam.cmd_Shutter(false);
        if (gpio_time != 0)
        {
            int32_t pin = get_pinCfgGpio(config_settings.pin_shutter);
            if (pin >= 0)
            {
                #ifdef SHUTTER_GPIO_ACTIVE_HIGH
                digitalWrite(pin, LOW);
                #endif
                pinMode(pin, INPUT);
            }
            gpio_time = 0;
        }
    }
    else if (httpcam.isOperating()) {
        // do nothing
    }
    else
    {
        if (config_settings.pin_shutter != PINCFG_NONE && config_settings.pin_shutter != config_settings.pin_exinput)
        {
            gpio_time = 0; // stop the timer
            int32_t pin = get_pinCfgGpio(config_settings.pin_shutter);
            if (pin >= 0)
            {
                #ifdef SHUTTER_GPIO_ACTIVE_HIGH
                digitalWrite(pin, LOW);
                #else
                pinMode(pin, INPUT);
                #endif
            }
        }
        else if (config_settings.infrared_enabled) {
            // do nothing
        }
    }
}

void cam_videoStart()
{
    // convenience function for quickly taking a video without complicated connectivity checks
    if (ptpcam.isOperating()) {
        ptpcam.cmd_MovieRecord(true);
    }
    else if (httpcam.isOperating()) {
        httpcam.cmd_MovieRecord(true);
    }
    else
    {
        if (config_settings.infrared_enabled) {
            SonyCamIr_Movie();
        }
    }
}

void cam_videoStop()
{
    // convenience function for quickly taking a video without complicated connectivity checks
    if (ptpcam.isOperating()) {
        ptpcam.cmd_MovieRecord(false);
    }
    else if (httpcam.isOperating()) {
        httpcam.cmd_MovieRecord(false);
    }
    else
    {
        if (config_settings.infrared_enabled) {
            SonyCamIr_Movie();
        }
    }
}
