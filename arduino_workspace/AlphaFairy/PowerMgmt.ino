#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>

#include "esp_pm.h"
#include "esp32/pm.h"

uint8_t batt_status = BATTSTAT_NONE;

float batt_vbus  = -1;
float batt_vbatt = -1;
float batt_ibatt = -1;
float batt_ibatt_max = -1;

extern bool http_is_active;
extern bool autoconnect_active;

uint32_t pwr_last_tick = 0;
uint32_t lcddim_last_tick = 0;
bool lcd_backlight_dim = false;
bool prevent_status_bar_thread = false;

void gui_drawStatusBar(bool is_black)
{
    #ifdef DISABLE_STATUS_BAR
    return;
    #endif

    static uint8_t li = 0;
    static uint32_t batt_last_time = 0;
    uint32_t now;
    static const char* txt_white = "_white";
    static const char* txt_black = "_black";
    static const char* txt_prefix = "/status_";
    static const char* txt_suffix = ".png";
    char fpath[64];

    if (((now = millis()) - batt_last_time) > 300 || batt_last_time == 0)
    {
        // only do once in a while, these are I2C transactions, could cause slowdowns
        // I'll admit that file reading, decoding, and LCD transactions are also very slow
        batt_last_time = now;
        if (li == 0 || batt_vbus < 0) {
            batt_vbus  = M5.Axp.GetVBusVoltage();
        }
        if (li == 1 || batt_vbatt < 0) {
            batt_vbatt = M5.Axp.GetBatVoltage();
        }
        if (li == 2 || batt_ibatt < 0) {
            batt_ibatt = M5.Axp.GetBatCurrent();
        }
        li = (li + 1) % 3;

        // the PMIC gives us a lot of data to use
        if (batt_vbus > 3) // check if USB power is available
        {
            batt_status = BATTSTAT_CHARGING;
            if (batt_vbatt > 4.1 && batt_ibatt >= 0)
            {
                // high vbatt and no in-flow current means battery is full
                // but there's a case when there's a constant trickle into the battery that never stops (I don't know why)
                // so we track what the maximum in-flow current is and see if it drops
                batt_ibatt_max = (batt_ibatt > batt_ibatt_max) ? batt_ibatt : batt_ibatt_max;

                if (batt_ibatt <= 20) {
                    batt_status = BATTSTAT_FULL;
                }
                else {
                    if (batt_ibatt < (batt_ibatt_max * 0.8)) {
                        batt_status = BATTSTAT_FULL;
                    }
                }
            }
            else if (batt_vbatt < 3.7) {
                batt_status = BATTSTAT_CHARGING_LOW;
            }
        }
        else
        {
            // not charging, check if battery is low

            if (batt_status == BATTSTAT_LOW) {
                // already low, see if it magically recharged itself
                if (batt_vbatt > 3.6) {
                    batt_status = BATTSTAT_NONE;
                }
            }
            else {
                // just became low
                if (batt_vbatt < 3.5) {
                    batt_status = BATTSTAT_LOW;
                }
                else if (batt_vbatt < 4.1) {
                    batt_status = BATTSTAT_NONE;
                }
            }
        }
    }

    if (prevent_status_bar_thread) {
        return;
    }

    if (batt_status == BATTSTAT_LOW && batt_vbatt < 3.05) {
        pwr_shutdown();
    }

    static uint32_t max_x = 0;
    uint32_t icon_width  = 32;
    uint32_t icon_height = 14; // should be 12, but 14 looks better
    uint32_t x = 0;
    uint32_t y = M5Lcd.height() - icon_height;

    // draw required status icons from left to right

    if (batt_status != BATTSTAT_NONE) {
        if (batt_status == BATTSTAT_LOW) {
            sprintf(fpath, "%slowbatt%s%s", txt_prefix, is_black ? txt_black : txt_white, txt_suffix);
        }
        else if (batt_status == BATTSTAT_FULL) {
            sprintf(fpath, "%sfullbatt%s%s", txt_prefix, is_black ? txt_black : txt_white, txt_suffix);
        }
        else if (batt_status == BATTSTAT_CHARGING) {
            sprintf(fpath, "%scharging%s%s", txt_prefix, is_black ? txt_black : txt_white, txt_suffix);
        }
        else if (batt_status == BATTSTAT_CHARGING_LOW) {
            sprintf(fpath, "%schglow%s%s", txt_prefix, is_black ? txt_black : txt_white, txt_suffix);
        }

        #ifndef USE_SPRITE_MANAGER
        M5Lcd.drawPngFile(SPIFFS, fpath, x, y);
        #else
        sprites->draw(fpath, x, y, icon_width, 12);
        #endif

        x += icon_width;
    }

    if (fairycam.isOperating() == false && http_is_active == false && autoconnect_active == false)
    {
        if (ptpcam.isPairingWaiting()) {
            gui_prepStatusBarText(x, y, is_black);
            M5Lcd.print("PAIR");
            x += icon_width;
        }
        else {
            sprintf(fpath, "%snocam%s%s", txt_prefix, is_black ? txt_black : txt_white, txt_suffix);

            #ifndef USE_SPRITE_MANAGER
            M5Lcd.drawPngFile(SPIFFS, fpath, x, y);
            #else
            sprites->draw(fpath, x, y, icon_width, 12);
            #endif

            x += icon_width;
        }
    }

#ifndef DISABLE_POWER_SAVE
    if (config_settings.pwr_save_secs > 5 && (now - pwr_last_tick) > ((config_settings.pwr_save_secs - 5) * 1000))
    {
        // show a "ZZZ" status when we are close to going into automatic sleep mode
        gui_prepStatusBarText(x, y, is_black);
        M5Lcd.print("ZZZ");
        x += icon_width;
    }
#endif

    if (x > max_x) {
        // track the largest status bar we've made so we can clear it
        max_x = x;
    }

    if (x < max_x) {
        // clear the blank space of the status bar
        M5Lcd.fillRect(x, y, max_x - x, icon_height, is_black ? TFT_BLACK : TFT_WHITE);
    }
}

void gui_prepStatusBarText(int16_t x, int16_t y, bool is_black)
{
    M5Lcd.setCursor(x + 5, y + 2);
    M5Lcd.setTextFont(0);
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(false);
    if (is_black == false) {
        M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        M5Lcd.setHighlightColor(TFT_WHITE);
    }
    else {
        M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Lcd.setHighlightColor(TFT_BLACK);
    }
}

void pwr_lcdUndim()
{
    if (lcd_backlight_dim) {
        M5.Axp.ScreenBreath(config_settings.lcd_brightness);
        lcd_backlight_dim = false;
    }
}

void pwr_sleepCheck()
{
    uint32_t now = millis();

    if (config_settings.lcd_dim_secs != 0)
    {
        if ((now - lcddim_last_tick) > (config_settings.lcd_dim_secs * 1000))
        {
            // time to dim the LCD backlight
            if (lcd_backlight_dim == false) {
                M5.Axp.ScreenBreath(7);
                lcd_backlight_dim = true;
            }
        }
        else {
            pwr_lcdUndim();
        }
    }
    else
    {
        pwr_lcdUndim();
    }

    #ifdef DISABLE_POWER_SAVE
    return;
    #else

    // do not sleep if HTTP client is connected
    if (http_is_active) {
        pwr_tick(true);
        return;
    }

    // enforce a minimum
    int32_t sleep_secs = config_settings.pwr_save_secs;
    if (sleep_secs < 30) {
        sleep_secs = 30;
        dbg_ser.printf("pwr_save_secs too low, reset to 30 seconds\r\n");
    }

    if ((now - pwr_last_tick) > (sleep_secs * 1000) && config_settings.pwr_save_secs > 0) // time to shutdown (and feature is enabled)
    {
        pwr_shutdown();
    }
    #endif
}

void pwr_lightSleepEnter()
{
    #ifdef ENABLE_LIGHT_SLEEP
    static esp_err_t old_e = ESP_OK;
    pwr_lightSleepSetup();
    gpio_wakeup_enable(GPIO_BTN_SIDE, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(GPIO_BTN_BIG , GPIO_INTR_LOW_LEVEL);
    esp_err_t e = esp_light_sleep_start();
    if (e != ESP_OK)
    {
        if (old_e != e) {
            Serial.printf("[%u] light sleep err %d\r\n", millis(), e);
        }
    }
    old_e = e;
    #endif
}

void pwr_lightSleepSetup()
{
    #ifdef ENABLE_LIGHT_SLEEP
    static bool has_configed = false;
    if (has_configed) {
        return;
    }
    has_configed = true;

    const esp_pm_config_esp32_t cfg = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 10,
        .light_sleep_enable = true,
    };
    esp_pm_configure(&cfg);
    // esp_wifi_set_ps called during connection

    esp_sleep_enable_gpio_wakeup();
    // note: gpio_wakeup_enable is called from pwr_lightSleepEnter because the ISR changes the interrupt mode every time it fires
    esp_sleep_enable_wifi_wakeup();
    esp_sleep_enable_timer_wakeup(10000);
    #endif
}

// shutdown, or pretend to shutdown
void pwr_shutdown()
{
    // disconnect wifi
    esp_wifi_disconnect();
    dbg_ser.println("wifi disconnected");
    esp_wifi_stop();
    dbg_ser.println("wifi stopped");
    esp_wifi_deinit();
    dbg_ser.println("wifi deinited");

    // show the animation on screen
    show_poweroff();

    // no USB voltage -> power off
    if (batt_vbus < 3) {
        Serial.println("Power Save Shutdown");
        while (true) {
            M5.Axp.PowerOff();
        }
    }

    // yes USB voltage -> pretend power off but keep charging the battery
    Serial.println("Power Save Screen Saver");
    M5Lcd.fillScreen(TFT_BLACK);
    M5.Axp.ScreenSwitch(false);
    while (true)
    {
        cmdline.task();
        yield();

        batt_vbus = M5.Axp.GetVBusVoltage();
        // no USB voltage -> power off
        if (batt_vbus < 3) {
            Serial.println("Power Save Screen Saver Shutdown");
            while (true) {
                M5.Axp.PowerOff();
            }
        }
        if (M5.Axp.GetBtnPress() != 0) {
            ESP.restart();
        }
    }
}

// reset the power sleep timer, prevent power saving sleep for a while
// parameter undim means the LCD backlight should be lit up again
void pwr_tick(bool undim)
{
    pwr_last_tick = millis();
    if (undim) {
        lcddim_last_tick = pwr_last_tick;
        pwr_lcdUndim();
    }
}

// show the shutdown animation and then perform the actual shutdown
void show_poweroff()
{
    uint32_t t = millis();

    prevent_status_bar_thread = true; // just in case this function is called from a non-GUI thread, prevent the status bar from being drawn over the animation

    cpufreq_boost();

    dbg_ser.println("show poweroff");

    // prep
    M5Lcd.setRotation(0);
    M5Lcd.fillScreen(TFT_BLACK);

    bool batt_good = batt_vbatt > 3.05 || batt_vbatt <= 0 || batt_status == BATTSTAT_CHARGING || batt_status == BATTSTAT_CHARGING_LOW;

    if (batt_good)
    {
        M5Lcd.drawPngFile(SPIFFS, "/sleep.png", 0, 0);
        delay(500);

        // animation is random

        if ((rand() % 2) == 0)
        {
            // fade by horizontal lines
            int y, dly = 10, m = 50;
            for (y = m + 0; y < M5Lcd.height() - m; y += 4) {
                M5Lcd.drawFastHLine(0, y, M5Lcd.width(), TFT_BLACK);
                delay(dly);
            }
            for (y = m + 2; y < M5Lcd.height() - m; y += 4) {
                M5Lcd.drawFastHLine(0, y, M5Lcd.width(), TFT_BLACK);
                delay(dly);
            }
            for (y = m + 1; y < M5Lcd.height() - m; y += 4) {
                M5Lcd.drawFastHLine(0, y, M5Lcd.width(), TFT_BLACK);
                delay(dly);
            }
            for (y = m + 3; y < M5Lcd.height() - m; y += 4) {
                M5Lcd.drawFastHLine(0, y, M5Lcd.width(), TFT_BLACK);
                delay(dly);
            }
        }
        else
        {
            // fade by snow, with dimming backlight
            int b = config_settings.lcd_brightness - 1;
            while (true)
            {
                uint32_t d = millis() - t;
                if (d > 800) {
                    t = millis();
                    M5.Axp.ScreenBreath(b);
                    b -= 1;
                    if (b < 5) {
                        break;
                    }
                }
                int x = rand() % M5Lcd.width();
                int y = 50 + (rand() % (M5Lcd.height() - 100));
                M5Lcd.fillRect(x, y, 1, 1, TFT_BLACK);
            }
        }
    }
    else // battery is dead
    {
        
        M5Lcd.drawPngFile(SPIFFS, "/dead_batt.png", 0, 0);
        delay(500);
        int b = config_settings.lcd_brightness - 1;
        while (true)
        {
            uint32_t d = millis() - t;
            if (d > 800) {
                t = millis();
                M5.Axp.ScreenBreath(b);
                b -= 1;
                if (b < 5) {
                    break;
                }
            }
            int x = rand() % M5Lcd.width();
            int y = rand() % M5Lcd.height();
            M5Lcd.fillRect(x, y, 1, 1, TFT_BLACK);
        }
    }

    M5Lcd.fillScreen(TFT_BLACK);
    M5.Axp.ScreenSwitch(false);
}

int pmic_fnum = 0;

void pmic_startCoulombCount(void)
{
    int  fnum;
    char fname[32];
    for (fnum = 1; fnum < 999; ) {
        sprintf(fname, "/pwrlog_%u.txt", fnum);
        if (SPIFFS.exists(fname) == false) {
            pmic_fnum = fnum;
            break;
        }
        else {
            fnum++;
            continue;
        }
    }

    M5.Axp.EnableCoulombcounter();
    M5.Axp.ClearCoulombcounter();

    if (pmic_fnum > 0) {
        dbg_ser.printf("coulomb cnt started, file %s\r\n", fname);
    }
    else {
        dbg_ser.println("unable to start pmic log");
    }
}

void pmic_log(void)
{
    static uint32_t last_time = 0;
    uint32_t now = millis();

    if (pmic_fnum <= 0) {
        return;
    }

    if ((now - last_time) < 5000) {
        return;
    }
    last_time = now;

    float c    = M5.Axp.GetCoulombData();
    float vbat = M5.Axp.GetBatVoltage();
    float ibat = M5.Axp.GetBatCurrent();

    char logstr[256];
    char fname[32];

    sprintf(fname, "/pwrlog_%u.txt", pmic_fnum);

    sprintf(logstr, "%8u, %0.3f, %0.3f, %0.3f, \r\n", millis(), vbat, ibat, c);

    File f = SPIFFS.open(fname, FILE_APPEND);
    f.print(logstr);
    f.close();

    dbg_ser.printf("coulomb cnt: ");
    dbg_ser.printf(logstr);

}
