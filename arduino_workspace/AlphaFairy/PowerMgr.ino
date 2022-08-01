#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>

uint8_t batt_status = BATTSTAT_NONE;

float batt_vbus  = -1;
float batt_vbatt = -1;
float batt_ibatt = -1;
float batt_ibatt_max = -1;

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
        if (li == 3) {
            uint8_t b = M5.Axp.GetBtnPress();
            if (b != 0) {
                dbg_ser.printf("user pressed power button\r\n");
                pwr_tick();
            }
        }
        li = (li + 1) % 4;

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
        M5Lcd.drawPngFile(SPIFFS, fpath, x, y);
        x += icon_width;
    }
    if (camera.isOperating() == false) {
        sprintf(fpath, "%snocam%s%s", txt_prefix, is_black ? txt_black : txt_white, txt_suffix);
        M5Lcd.drawPngFile(SPIFFS, fpath, x, y);
        x += icon_width;
    }

    if (x > max_x) {
        // track the largest status bar we've made so we can clear it
        max_x = x;
    }

    if (x < max_x) {
        // clear the blank space of the status bar
        M5Lcd.fillRect(x, y, max_x - x, icon_height, is_black ? TFT_BLACK : TFT_WHITE);
    }
}

uint32_t pwr_last_tick = 0;

void pwr_sleepCheck()
{
    #ifdef DISABLE_POWER_SAVE
    return;
    #else

    // enforce a minimum
    if (config_settings.pwr_save_secs < 30) {
        config_settings.pwr_save_secs = 30;
        dbg_ser.printf("pwr_save_secs too low, reset to 30 seconds\r\n");
    }

    uint32_t now = millis();
    if ((now - pwr_last_tick) > (config_settings.pwr_save_secs * 1000))
    {
        if (config_settings.pwr_save_secs == 0) {
            // feature disabled
            pwr_tick();
            dbg_ser.printf("pwr save disabled by user\r\n");
            return;
        }

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
        M5.Axp.ScreenBreath(0);
        while (true) {
            batt_vbus = M5.Axp.GetVBusVoltage();
            // no USB voltage -> power off
            if (batt_vbus < 3) {
                Serial.println("Power Save Screen Saver Shutdown");
                while (true) {
                    M5.Axp.PowerOff();
                }
            }
            delay(1000);
        }
    }
    #endif
}

void pwr_tick()
{
    pwr_last_tick = millis();
}
