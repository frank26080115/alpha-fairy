#include "AlphaFairy.h"

bool focusfrust_state_prev = false;
uint32_t focusfrust_last_time_on  = 0;
uint32_t focusfrust_last_time_off = 0;
uint8_t focusfrust_cnt = 0;

uint32_t focusfrust_objinmem = 0;
uint32_t focusfrust_cardrem = 0;

void focusfrust_reset()
{
    focusfrust_state_prev    = false;
    focusfrust_last_time_on  = 0;
    focusfrust_last_time_off = 0;
    focusfrust_cnt = 0;
}

void focusfrust_task()
{
    if (ptpcam.isOperating() == false) {
        focusfrust_reset();
        return;
    }

    if (ptpcam.has_property(SONYALPHA_PROPCODE_FocusFound) == false) {
        return;
    }

    uint32_t x;

    /*
    track the card space and buffer space, reset the tap counter if the user actually takes a photo
    */
    uint32_t cardspace = 0;

    if (ptpcam.has_property(SONYALPHA_PROPCODE_ObjectInMemory))
    {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_ObjectInMemory);
        if (x != focusfrust_objinmem) {
            // buffer space has changed
            focusfrust_cnt = 0;
            focusfrust_objinmem = x;
            dbg_ser.printf("focusfrust obj in mem %u\r\n", x);
        }
    }

    if (ptpcam.has_property(SONYALPHA_PROPCODE_MemoryRemaining_Card1))
    {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_MemoryRemaining_Card1);
        cardspace += x;
    }
    if (ptpcam.has_property(SONYALPHA_PROPCODE_MemoryRemaining_Card2))
    {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_MemoryRemaining_Card2);
        cardspace += x;
    }
    if (cardspace != focusfrust_cardrem) {
        // cardspace has changed
        focusfrust_cnt = 0;
        focusfrust_cardrem = cardspace;
        dbg_ser.printf("focusfrust card space %u\r\n", cardspace);
    }

    x = ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound);
    uint32_t now = millis();
    if (x != SONYALPHA_FOCUSSTATUS_NONE)
    {
        // shutter button half press
        if (focusfrust_state_prev == false) {
            focusfrust_last_time_on = now;
            pwr_tick();
            dbg_ser.printf("focusfrust pressed %u\r\n", now);
        }
        focusfrust_state_prev = true;
    }
    else
    {
        // shutter button released
        if (focusfrust_state_prev)
        {
            pwr_tick();
            if ((now - focusfrust_last_time_on) < 500 && (now - focusfrust_last_time_off) < 1000)
            {
                // the tap was rapid enough
                dbg_ser.printf("focusfrust released %u\r\n", now);
                focusfrust_cnt += 1;
            }
            else
            {
                focusfrust_cnt = 0;
            }
            focusfrust_last_time_off = now;
        }
        focusfrust_state_prev = false;
    }

    if (focusfrust_cnt >= 4)
    {
        dbg_ser.println("focusfrust cnt reached limit");
        M5Lcd.fillRect(0, 0, M5Lcd.width(), 12, TFT_RED);
        focusfrust_execute(NULL);
        focusfrust_cnt = 0;
        M5Lcd.fillRect(0, 0, M5Lcd.width(), 12, TFT_WHITE);
    }
}

void focusfrust_execute(void* mip)
{
    dbg_ser.println("focusfrust_execute");

    if (ptpcam.isOperating() == false) {
        // show user that the camera isn't connected
        app_waitAllReleaseConnecting(BTN_DEBOUNCE);
        return;
    }

    ledblink_setMode(LEDMODE_OFF);

    #ifdef APP_DOES_NOTHING
    app_waitAllRelease(BTN_DEBOUNCE);
    return;
    #endif

    // obviously we need to be in manual focus mode in order to change focus
    ptpcam.cmd_ManualFocusMode(true, true);

    ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
    for (int i = 0; i < 10 || btnBig_isPressed(); i++) {
        ptpcam.cmd_ManualFocusStep(SONYALPHA_FOCUSSTEP_CLOSER_LARGE);
        ptpcam.wait_while_busy(config_settings.focus_pause_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
    }

    // return to AF mode
    ptpcam.cmd_ManualFocusMode(false, false);

    app_waitAllRelease(BTN_DEBOUNCE);
}
