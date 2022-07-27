#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>

bool batt_need_recheck = false;

bool is_low_batt()
{
    static uint32_t last_time = 0;
    uint32_t now;

    static bool is_low = false;

    if (((now = millis()) - last_time) > 1000 || last_time == 0)
    {
        last_time = now;
        float vbus = M5.Axp.GetVBusVoltage();
        float vbatt = M5.Axp.GetVBusVoltage();
        if (vbus > 3 || vbatt > 3.4) {
            if (is_low) {
                batt_need_recheck = true;
            }
            is_low = false;
        }
        else if (vbatt < 3.2) {
            if (is_low == false) {
                batt_need_recheck = true;
            }
            is_low = true;
        }
    }

    return is_low;
}

void battlow_draw(bool is_black)
{
    if (is_black) {
        M5Lcd.drawPngFile(SPIFFS, "/lowbatt_black.png", 0, M5Lcd.height() - 12);
    }
    else {
        M5Lcd.drawPngFile(SPIFFS, "/lowbatt_white.png", 0, M5Lcd.height() - 12);
    }
}