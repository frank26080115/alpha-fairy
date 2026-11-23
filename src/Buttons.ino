#include "AlphaFairy.h"
#include <Arduino.h>
#include <stdbool.h>
#include "button_pins.h"

/*
interrupts track both edges so short taps are captured between polls
each new press requires a stable release first; holds never repeat
*/

//#define BTNS_DEBUG

volatile uint32_t btnSide_downTime = 0;
volatile uint32_t btnBig_downTime  = 0;
static volatile uint32_t btnSide_upTime = 0;
static volatile uint32_t btnBig_upTime  = 0;
static volatile bool btnSide_down = false;
static volatile bool btnBig_down  = false;
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

#if defined(CONFIG_IDF_TARGET_ESP32S3)
// M5PM1 button registers, matching m5stack/M5PM1 (BTN_CFG_1/2 and BTN_STATUS).
// Use M5Unified's shared bus so this works with both older and newer M5Unified versions.
static constexpr uint8_t STICKS3_PM1_ADDRESS = 0x6E;
static constexpr uint32_t STICKS3_PM1_I2C_HZ = 100000;
static bool sticks3_power_button_ready = false;
static bool sticks3_power_button_sampled = false, sticks3_power_button_down = false;
#endif

void btnPwr_init(void)
{
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
    if (M5.getBoard() != m5::board_t::board_M5StickS3) return;
    sticks3_power_button_ready = false;
    sticks3_power_button_sampled = false;
    sticks3_power_button_down = false;
    uint8_t single_config, double_config;
    if (!M5.In_I2C.readRegister(STICKS3_PM1_ADDRESS, 0x49, &single_config, 1, STICKS3_PM1_I2C_HZ)
        || !M5.In_I2C.readRegister(STICKS3_PM1_ADDRESS, 0x4A, &double_config, 1, STICKS3_PM1_I2C_HZ)) {
        Serial.println("StickS3 Back: PMIC configuration read failed");
        return;
    }
    // Disable single-click reset; preserve timing and hardware download/recovery settings.
    single_config |= 0x01;
    double_config &= ~0x01; // Keep hardware double-click power-off enabled.
    if (!M5.In_I2C.writeRegister8(STICKS3_PM1_ADDRESS, 0x49, single_config, STICKS3_PM1_I2C_HZ)
        || !M5.In_I2C.writeRegister8(STICKS3_PM1_ADDRESS, 0x4A, double_config, STICKS3_PM1_I2C_HZ)) {
        Serial.println("StickS3 Back: PMIC configuration write failed");
        return;
    }
    // Discard the power-on press. BTN_STATUS bit 7 is a press latch, cleared by reading it.
    uint8_t status;
    if (M5.In_I2C.readRegister(STICKS3_PM1_ADDRESS, 0x48, &status, 1, STICKS3_PM1_I2C_HZ)) {
        sticks3_power_button_sampled = true;
        sticks3_power_button_down = (status & 0x01) != 0;
    }
    sticks3_power_button_ready = true;
    #endif
}

#if defined(CONFIG_IDF_TARGET_ESP32S3)
uint8_t btnPwr_readStickS3(void)
{
    if (!sticks3_power_button_ready) return 0;
    uint8_t status;
    if (!M5.In_I2C.readRegister(STICKS3_PM1_ADDRESS, 0x48, &status, 1, STICKS3_PM1_I2C_HZ)) return 0;
    bool down = (status & 0x01) != 0;
    bool was_down = sticks3_power_button_down;
    sticks3_power_button_down = down;
    if (!sticks3_power_button_sampled) {
        sticks3_power_button_sampled = true;
        return 0; // If the initial sample failed, do not turn the boot press into Back later.
    }
    // Report a new press once, or a complete tap missed between polls (latched bit 7).
    // No hold timer or repeat action. Hardware double-click shutdown remains independent.
    return !was_down && (down || (status & 0x80)) ? 2 : 0;
}
#endif

void IRAM_ATTR btnSide_isr()
{
    #if defined(ENABLE_LIGHT_SLEEP) && defined(ENABLE_LIGHT_SLEEP_GPIOWAKE)
    // Restore both edges after low-level GPIO wakeup.
    gpio_set_intr_type(GPIO_BTN_SIDE, GPIO_INTR_ANYEDGE);
    #endif

    bool down = digitalRead(PIN_BTN_SIDE) == LOW;
    // Ignore spurious/duplicate interrupts; trust the actual pin level.
    if (down == btnSide_down) return;
    btnSide_down = down;
    uint32_t now = millis();
    if (down) {
        if ((now - btnSide_upTime) >= BTN_DEBOUNCE) btnSide_cnt++;
        btnSide_downTime = now;
    }
    else {
        btnSide_upTime = now;
    }
}

void IRAM_ATTR btnBig_isr()
{
    #if defined(ENABLE_LIGHT_SLEEP) && defined(ENABLE_LIGHT_SLEEP_GPIOWAKE)
    gpio_set_intr_type(GPIO_BTN_BIG, GPIO_INTR_ANYEDGE);
    #endif

    bool down = digitalRead(PIN_BTN_BIG) == LOW;
    if (down == btnBig_down) return;
    btnBig_down = down;
    uint32_t now = millis();
    if (down) {
        if ((now - btnBig_upTime) >= BTN_DEBOUNCE) btnBig_cnt++;
        btnBig_downTime = now;
    }
    else {
        btnBig_upTime = now;
    }
}

void btns_init()
{
    pinMode(PIN_BTN_SIDE, INPUT_PULLUP);
    pinMode(PIN_BTN_BIG, INPUT_PULLUP);
    // A button held at boot is not a new press. Its release starts debounce.
    btnSide_down = digitalRead(PIN_BTN_SIDE) == LOW;
    btnBig_down = digitalRead(PIN_BTN_BIG) == LOW;
    btnSide_upTime = btnBig_upTime = millis();
    attachInterrupt(PIN_BTN_SIDE, btnSide_isr, CHANGE);
    attachInterrupt(PIN_BTN_BIG, btnBig_isr, CHANGE);
}

void btns_poll()
{
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    // Share the edge tracking with the ISR without processing an edge twice.
    noInterrupts();
    btnSide_isr();
    btnBig_isr();
    interrupts();
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
        cpufreq_boost();
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
        cpufreq_boost();
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
    bool x = false;
    x |= btnSide_hasPressed();
    x |= btnBig_hasPressed();
    return x;
}

void btnBoth_clrPressed() {
    btnSide_clrPressed();
    btnBig_clrPressed();
}

bool btnAny_hasPressed() {
    bool x = false;
    x |= btnSide_hasPressed();
    x |= btnBig_hasPressed();
    x |= btnPwr_hasPressed();
    x |= m5power_getButtonPress() != 0;
    return x;
}

void btnAny_clrPressed() {
    btnSide_clrPressed();
    btnBig_clrPressed();
    btnPwr_clrPressed();
}

void btnPwr_poll()
{
    uint32_t now = millis();
    static uint32_t btn_last_time = 0;
    if ((now - btn_last_time) > 100 || btn_last_time == 0) {
        btn_last_time = now;
        btnPwr_quickPoll();
    }
}

void btnPwr_quickPoll()
{
    uint8_t b = m5power_getButtonPress();
    if (b != 0) {
        btnPwr_cnt++;
        dbg_ser.printf("user pressed power button\r\n");
        pwr_tick(true);
        cpufreq_boost();
    }
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
