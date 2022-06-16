#include "AlphaFairy.h"
#include <Arduino.h>
#include <stdbool.h>

#define PIN_BTN_SIDE 39
#define PIN_BTN_BIG 37

volatile uint32_t btnSide_downTime = 0;
volatile uint32_t btnBig_downTime = 0;
volatile bool btnSide_flag = false;
volatile bool btnBig_flag  = false;

void IRAM_ATTR btnSide_isr() {
    btnSide_flag = true;
    btnSide_downTime = millis();
}

void IRAM_ATTR btnBig_isr() {
    btnBig_flag = true;
    btnBig_downTime = millis();
}

void btns_init()
{
    pinMode(PIN_BTN_SIDE, INPUT_PULLUP);
    attachInterrupt(PIN_BTN_SIDE, btnSide_isr, FALLING);
    pinMode(PIN_BTN_BIG, INPUT_PULLUP);
    attachInterrupt(PIN_BTN_BIG, btnBig_isr, FALLING);
}

bool btnSide_hasPressed(bool clr) {
    if (btnSide_flag)
    {
        if ((millis() - btnSide_downTime) > BTN_DEBOUNCE)
        {
            if (btnSide_isPressed())
            {
                if (clr) {
                    btnSide_flag = false;
                }
                return true;
            }
            else
            {
                btnSide_flag = false;
            }
        }
    }
    return false;
}

bool btnBig_hasPressed(bool clr) {
    if (btnBig_flag)
    {
        if ((millis() - btnBig_downTime) > BTN_DEBOUNCE)
        {
            if (btnBig_isPressed())
            {
                if (clr) {
                    btnBig_flag = false;
                }
                return true;
            }
            else
            {
                btnBig_flag = false;
            }
        }
    }
    return false;
}

bool btnSide_isPressed() {
    return (digitalRead(PIN_BTN_SIDE) == LOW) && (millis() - btnSide_downTime) > BTN_DEBOUNCE;
}

bool btnBig_isPressed() {
    return (digitalRead(PIN_BTN_BIG) == LOW) && (millis() - btnBig_downTime) > BTN_DEBOUNCE;
}

void btns_clear() {
    btnSide_flag = false;
    btnBig_flag  = false;
}
