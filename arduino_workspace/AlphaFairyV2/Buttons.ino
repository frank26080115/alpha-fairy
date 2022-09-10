#include "AlphaFairy.h"
#include <Arduino.h>
#include <stdbool.h>

/*
interrupts are used to catch new button press events
a simple debounce algorithm is used
*/

#define PIN_BTN_SIDE     39
#define PIN_BTN_BIG      37
#define GPIO_BTN_SIDE    GPIO_NUM_39
#define GPIO_BTN_BIG     GPIO_NUM_37

//#define BTNS_DEBUG

volatile uint32_t btnSide_downTime = 0;
volatile uint32_t btnBig_downTime  = 0;
volatile uint32_t btnSide_clrTime  = 0;
volatile uint32_t btnBig_clrTime   = 0;
volatile uint32_t btnSide_cnt      = 0;
volatile uint32_t btnBig_cnt       = 0;
volatile uint32_t btnPwr_cnt       = 0;
volatile uint32_t btnSide_cnt_prev = 0;
volatile uint32_t btnBig_cnt_prev  = 0;
volatile uint32_t btnPwr_cnt_prev  = 0;

extern uint32_t pwr_last_tick;
extern uint32_t lcddim_last_tick;
extern void pwr_lcdUndim(void);

void IRAM_ATTR btnSide_isr()
{
    if (digitalRead(PIN_BTN_SIDE) != LOW) {
        // guard against ESP32 hardware bug 
        // https://github.com/espressif/arduino-esp32/issues/5055
        // https://github.com/espressif/esp-idf/commit/d890a516a1097f0a07788e203fdb1a82bb83520e
        return;
    }
    uint32_t now = millis();
    if ((now - btnSide_downTime) > BTN_DEBOUNCE) {
        btnSide_cnt++;
    }
    btnSide_downTime = now;
}

void IRAM_ATTR btnBig_isr()
{
    if (digitalRead(PIN_BTN_BIG) != LOW) {
        // guard against ESP32 hardware bug 
        // https://github.com/espressif/arduino-esp32/issues/5055
        // https://github.com/espressif/esp-idf/commit/d890a516a1097f0a07788e203fdb1a82bb83520e
        return;
    }
    uint32_t now = millis();
    if ((now - btnBig_downTime) > BTN_DEBOUNCE) {
        btnBig_cnt++;
    }
    btnBig_downTime = now;
}

void btns_init()
{
    pinMode(PIN_BTN_SIDE, INPUT_PULLUP);
    attachInterrupt(PIN_BTN_SIDE, btnSide_isr, FALLING);
    pinMode(PIN_BTN_BIG, INPUT_PULLUP);
    attachInterrupt(PIN_BTN_BIG, btnBig_isr, FALLING);
    //gpio_wakeup_enable(GPIO_BTN_SIDE, GPIO_INTR_LOW_LEVEL);
    //gpio_wakeup_enable(GPIO_BTN_BIG , GPIO_INTR_LOW_LEVEL);
}

void btns_poll()
{
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    // this function attempts to catch missed ISRs
    static char prev_side = HIGH;
    static char prev_big  = HIGH;
    char x;

    volatile uint32_t now = millis();

    if ((x = digitalRead(PIN_BTN_SIDE)) != prev_side) {
        if (x == LOW && btnSide_downTime == 0 && (now - btnSide_clrTime) > 100) {
            btnSide_cnt++;
            btnSide_downTime = now;
        }
        prev_side = x;
    }
    if ((x = digitalRead(PIN_BTN_BIG)) != prev_big) {
        if (x == LOW && btnBig_downTime == 0 && (now - btnBig_clrTime) > 100) {
            btnBig_cnt++;
            btnBig_downTime = now;
        }
        prev_big = x;
    }
    #endif
}

bool btnSide_hasPressed() {
    volatile bool x = btnSide_cnt_prev != btnSide_cnt;
    #ifdef BTNS_DEBUG
    static uint32_t rpt_time = 0;
    #endif
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    if (x == false) {
        btns_poll();
        x = btnSide_cnt_prev != btnSide_cnt;
    }
    #endif
    if (x)
    {
        #ifdef BTNS_DEBUG
        Serial.printf("btnSide_cnt %u %u\r\n", btnSide_cnt, btnSide_cnt_prev);
        rpt_time = 0;
        #endif
        return true;
    }
    #ifdef BTNS_DEBUG
    uint32_t now = millis();
    if ((now - rpt_time) > 500) {
        rpt_time = now;
        Serial.printf("not btnSide_cnt %u %u\r\n", btnSide_cnt, btnSide_cnt_prev);
    }
    #endif
    return false;
}

void btnSide_clrPressed() {
    uint32_t now = millis();
    #ifdef BTNS_DEBUG
    Serial.printf("clr btnSide_cnt %u %u\r\n", btnSide_cnt, btnSide_cnt_prev);
    #endif
    btnSide_cnt_prev = btnSide_cnt;
    //btnSide_downTime = 0;
    btnSide_clrTime  = now;
    pwr_last_tick    = now;
    lcddim_last_tick = now;
    pwr_lcdUndim();
}

bool btnBig_hasPressed() {
    volatile bool x = btnBig_cnt_prev != btnBig_cnt;
    #ifdef BTNS_DEBUG
    static uint32_t rpt_time = 0;
    #endif
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    if (x == false) {
        btns_poll();
        x = btnBig_cnt_prev != btnBig_cnt;
    }
    #endif
    if (x) {
        #ifdef BTNS_DEBUG
        Serial.printf("btnBig_cnt %u %u\r\n", btnBig_cnt, btnBig_cnt_prev);
        rpt_time = 0;
        #endif
        return true;
    }
    #ifdef BTNS_DEBUG
    uint32_t now = millis();
    if ((now - rpt_time) > 500) {
        rpt_time = now;
        Serial.printf("not btnBig_cnt %u %u\r\n", btnBig_cnt, btnBig_cnt_prev);
    }
    #endif
    return false;
}

void btnBig_clrPressed() {
    uint32_t now = millis();
    #ifdef BTNS_DEBUG
    Serial.printf("clr btnBig_cnt %u %u\r\n", btnBig_cnt, btnBig_cnt_prev);
    #endif
    btnBig_cnt_prev  = btnBig_cnt;
    //btnBig_downTime  = 0;
    btnBig_clrTime   = now;
    pwr_last_tick    = now;
    lcddim_last_tick = now;
    pwr_lcdUndim();
}

bool btnPwr_hasPressed() {
    volatile bool x = btnPwr_cnt != btnPwr_cnt_prev;
    return x;
}

void btnPwr_clrPressed() {
    uint32_t now = millis();
    btnPwr_cnt_prev = btnPwr_cnt;
    pwr_last_tick = now;
    lcddim_last_tick = now;
}

bool btnSide_isPressed() {
    return (digitalRead(PIN_BTN_SIDE) == LOW);
}

bool btnBig_isPressed() {
    return (digitalRead(PIN_BTN_BIG) == LOW);
}

bool btnBoth_hasPressed() {
    bool x;
    x |= btnSide_hasPressed();
    x |= btnBig_hasPressed();
    return x;
}

void btnBoth_clrPressed() {
    btnSide_clrPressed();
    btnBig_clrPressed();
}

bool btnAll_hasPressed() {
    bool x;
    x |= btnSide_hasPressed();
    x |= btnBig_hasPressed();
    x |= btnPwr_hasPressed();
    x |= M5.Axp.GetBtnPress() != 0;
    return x;
}

void btnAll_clrPressed() {
    btnSide_clrPressed();
    btnBig_clrPressed();
    btnPwr_clrPressed();
}

#if defined(HTTP_MOCKBTNS_ENABLE)

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
extern AsyncWebServer* httpServer;

void btn_installMockServer()
{
    httpServer->on("/btnbig", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        btnBig_cnt++;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        response->printf("ok");
        request->send(response);
    });

    httpServer->on("/btnside", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        btnSide_cnt++;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        response->printf("ok");
        request->send(response);
    });

    httpServer->on("/btnpwr", HTTP_GET, [] (AsyncWebServerRequest* request)
    {
        btnPwr_cnt++;
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        response->printf("ok");
        request->send(response);
    });
}

#endif
