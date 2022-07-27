#include "AlphaFairy.h"
#include <M5DisplayExt.h>

const configitem_t intval_config_generic[] = {
  // item pointer                           ,  max , min , step , text           , flags
  { (int32_t*)&(config_settings.intv_bulb  ), 10000,    0,     1, "bulb time"    , CFGFMT_TIME | CFGFMT_BULB },
  { (int32_t*)&(config_settings.intv_intval), 10000,    0,     1, "interval"     , CFGFMT_TIME               },
  { (int32_t*)&(config_settings.intv_delay ), 10000,    0,     1, "start delay"  , CFGFMT_TIME               },
  { (int32_t*)&(config_settings.intv_limit ), 10000,    1,     1, "num of shots" , CFGFMT_BYTENS             },
  { NULL, 0, 0, 0, "" }, // end of table
};

const configitem_t intval_config_astro[] = {
  // item pointer                            ,  max , min , step , text           , flags
  { (int32_t*)&(config_settings.astro_bulb  ), 10000,    0,     5, "bulb time"    , CFGFMT_TIME | CFGFMT_BULB },
  { (int32_t*)&(config_settings.astro_pause ), 10000,    0,     1, "pause gap"    , CFGFMT_TIME               },
  { (int32_t*)&(config_settings.astro_delay ), 10000,    0,     1, "start delay"  , CFGFMT_TIME               },
  { (int32_t*)&(config_settings.astro_limit ), 10000,    1,     1, "num of shots" , CFGFMT_BYTENS             },
  { NULL, 0, 0, 0, "" }, // end of table
};

void intervalometer_config(void* mip)
{
    menuitem_t* menuitm = (menuitem_t*)mip;
    menustate_t* m = NULL;
    configitem_t* ctable = NULL;
    if (menuitm->id == MENUITEM_INTERVAL) {
        m = &menustate_intval;
        ctable = (configitem_t*)intval_config_generic;
    }
    else if (menuitm->id == MENUITEM_ASTRO) {
        m = &menustate_astro;
        ctable = (configitem_t*)intval_config_astro;
    }

    m->idx = 0;
    m->last_idx = -1;
    for (m->cnt = 0; ; m->cnt++) {
        configitem_t* itm = &(ctable[m->cnt]);
        if (itm->ptr_val == NULL) {
            break;
        }
    }
    m->cnt++; // one more for the back option

    gui_startPrint();

    while (true)
    {
        app_poll();

        if (batt_need_recheck) {
            // if battery state changed, force a redraw
            batt_need_recheck = false;
            M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
            if (is_low_batt()) {
                battlow_draw(true);
            }
            interval_drawIcon(menuitm->id);
        }

        if (btnSide_hasPressed(true))
        {
            m->idx = (m->idx >= m->cnt) ? 0 : (m->idx + 1);
            M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
            if (is_low_batt()) {
                battlow_draw(true);
            }
            interval_drawIcon(menuitm->id);
        }

        configitem_t* cfgitm = (configitem_t*)&(config_items[m->idx]);
        if (m->idx == m->cnt) // last item is the exit item
        {
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.print("Go Back to Menu");
            if (btnBig_hasPressed(true))
            {
                settings_save();
                return;
            }
        }
        else if (m->idx == m->cnt - 1) // 2nd to last item is the start/stop item
        {
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.print("Press to Start");
            M5Lcd.println();
            gui_setCursorNextLine();

            if (menuitm->id == MENUITEM_INTERVAL)
            {
                if (config_settings.intv_bulb != 0) {
                    M5Lcd.print(config_settings.intv_bulb, DEC);
                    M5Lcd.print("s @ ");
                }
                gui_showVal(config_settings.intv_intval, CFGFMT_TIME, (Print*)&M5Lcd);
                if (config_settings.intv_limit != 0 && config_settings.intv_limit < 1000) {
                    M5Lcd.print(" X ");
                    M5Lcd.print(config_settings.intv_bulb, DEC);
                }
            }
            else if (menuitm->id == MENUITEM_ASTRO)
            {
                if (config_settings.astro_bulb != 0) {
                    M5Lcd.print(config_settings.astro_bulb, DEC);
                    M5Lcd.print("s");
                }
                if (config_settings.astro_pause > 1) {
                    M5Lcd.print(" + ");
                    M5Lcd.print(config_settings.astro_pause, DEC);
                    M5Lcd.print("s");
                }
                else if (config_settings.astro_bulb == 0) {
                    M5Lcd.print(config_settings.astro_pause, DEC);
                    M5Lcd.print("s");
                }
                if (config_settings.intv_limit != 0 && config_settings.intv_limit < 1000) {
                    M5Lcd.print(" X ");
                    M5Lcd.print(config_settings.intv_bulb, DEC);
                }
            }
            gui_blankRestOfLine();

            if (btnBig_hasPressed(true))
            {
                settings_save();
            }
        }
        else
        {
            configitem_t* cfgitm = (configitem_t*)&(ctable[m->idx]);
            // first line shows name of item, second line shows the value
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.print(cfgitm->text);
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_valIncDec(cfgitm);
            uint32_t total_time = 0, shot_cnt = 0;
            if (menuitm->id == MENUITEM_INTERVAL) {
                shot_cnt = config_settings.intv_limit;
                total_time = shot_cnt * config_settings.intv_intval;
            }
            else if (menuitm->id == MENUITEM_ASTRO) {
                shot_cnt = config_settings.astro_limit;
                if (config_settings.astro_bulb == 0) {
                    total_time = shot_cnt * config_settings.astro_pause;
                }
                else {
                    total_time = shot_cnt * (config_settings.astro_bulb + config_settings.astro_pause);
                }
            }
            M5Lcd.println();
            gui_setCursorNextLine();
            if (total_time == 0) {
                gui_blankRestOfLine();
            }
            else {
                M5Lcd.print("Total: ");
                gui_showVal(total_time, CFGFMT_TIME, (Print*)&M5Lcd);
            }
        }
        if (is_low_batt()) {
            battlow_draw(true);
        }
        interval_drawIcon(menuitm->id);
    }
}

void interval_drawIcon(uint8_t id)
{
    if (id == MENUITEM_INTERVAL) {
        M5Lcd.drawPngFile(SPIFFS, "/intervalometer_icon.png", M5Lcd.width() - 60, M5Lcd.height() - 60);
    }
    else if (id == MENUITEM_ASTRO) {
        M5Lcd.drawPngFile(SPIFFS, "/galaxy_icon.png", M5Lcd.width() - 60, M5Lcd.height() - 60);
    }
}