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