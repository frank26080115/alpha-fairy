#include "AlphaFairy.h"

#define PIN_CORNER_LED 10

#ifdef LEDBLINK_USE_PWM

#define LEDBLINK_PWM_CHAN 1 // channel 0 is occupied for buzzer
#define LEDBLINK_PWM_RESO 8
#define LEDBLINK_PWM_FREQ 5000

#endif

bool led_is_on = false;
uint8_t ledblink_mode = LEDMODE_NORMAL;

void ledblink_on()
{
    #ifndef LEDBLINK_USE_PWM
    if (config_settings.led_enabled != 0) {
        digitalWrite(PIN_CORNER_LED, LOW);
    }
    #else
    ledcWrite(LEDBLINK_PWM_CHAN, (config_settings.led_enabled != 0) ? LEDBLINK_PWM_DUTY_ON : LEDBLINK_PWM_DUTY_OFF);
    #endif
    led_is_on = true;
}

void ledblink_off()
{
    #ifndef LEDBLINK_USE_PWM
    digitalWrite(PIN_CORNER_LED, HIGH);
    #else
    ledcWrite(LEDBLINK_PWM_CHAN, LEDBLINK_PWM_DUTY_OFF);
    #endif
    led_is_on = false;
}

void ledblink_tog()
{
    if (led_is_on) {
        ledblink_off();
    }
    else {
        ledblink_on();
    }
}

void ledblink_init()
{
    pinMode(PIN_CORNER_LED, OUTPUT);
    #ifdef LEDBLINK_USE_PWM
    ledcSetup(LEDBLINK_PWM_CHAN, LEDBLINK_PWM_FREQ, LEDBLINK_PWM_RESO);
    ledcAttachPin(PIN_CORNER_LED, LEDBLINK_PWM_CHAN);
    #endif
    ledblink_off();
}

void ledblink_setMode(uint8_t x)
{
    ledblink_mode = x;
    if (x == LEDMODE_OFF) {
        // turn off the LED here
        // we don't want to do this in a loop
        // this way, OFF mode can still have the application control the LED
        ledblink_off();
    }
}

void ledblink_task()
{
    // blink the LED according to status (connected? movie recording? app-is-busy? etc)
    if (ledblink_mode == LEDMODE_ON)
    {
        ledblink_on();
    }
    else if (ledblink_mode != LEDMODE_OFF)
    {
        uint32_t now = millis();
        if (ledblink_mode == LEDMODE_BLINK) // application wants a rapid blink (for stuff like user held down the button)
        {
            now %= 400;
            if (now < 200) {
                ledblink_on();
            }
            else {
                ledblink_off();
            }
        }
        else // normal mode
        {
            if (camera.isOperating())
            {
                if (camera.is_movierecording())
                {
                    now %= 1500;
                    if (now < 100 || (now >= 200 && now <= 300) || (now >= 400 && now <= 500)) { // triple large blinks with decent pause
                        ledblink_on();
                    }
                    else {
                        ledblink_off();
                    }
                }
                else // connected but not recording a movie
                {
                    now %= 1000;
                    if (now < 20 || (now >= 60 && now <= 80)) { // double short blink with a decent pause
                        ledblink_on();
                    }
                    else {
                        ledblink_off();
                    }
                }
            }
            else // not connected
            {
                now %= 2000;
                if (now < 20) { // single short blink with a long pause
                    ledblink_on();
                }
                else {
                    ledblink_off();
                }
            }
        }
    }
}
