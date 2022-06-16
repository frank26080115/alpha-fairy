#include "AlphaFairy.h"

#define PIN_CORNER_LED 10

bool led_is_on = false;
uint8_t ledblink_mode = LEDMODE_NORMAL;

void ledblink_on()
{
    digitalWrite(PIN_CORNER_LED, LOW);
    led_is_on = true;
}

void ledblink_off()
{
    digitalWrite(PIN_CORNER_LED, HIGH);
    led_is_on = false;
}

void ledblink_tog()
{
    digitalWrite(PIN_CORNER_LED, led_is_on ? HIGH : LOW);
    led_is_on = !led_is_on;
}

void ledblink_init()
{
    pinMode(PIN_CORNER_LED, OUTPUT);
    ledblink_off();
}

void ledblink_setMode(uint8_t x)
{
    ledblink_mode = x;
}

void ledblink_task()
{
    
    if (ledblink_mode == LEDMODE_ON)
    {
        ledblink_on();
    }
    else
    {
        uint32_t now = millis();
        if (ledblink_mode == LEDMODE_BLINK)
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
                    if (now < 100 || (now >= 200 && now <= 300) || (now >= 400 && now <= 500)) {
                        ledblink_on();
                    }
                    else {
                        ledblink_off();
                    }
                }
                else
                {
                    now %= 1000;
                    if (now < 20 || (now >= 60 && now <= 80)) {
                        ledblink_on();
                    }
                    else {
                        ledblink_off();
                    }
                }
            }
            else
            {
                now %= 2000;
                if (now < 20) {
                    ledblink_on();
                }
                else {
                    ledblink_off();
                }
            }
        }
    }
}
