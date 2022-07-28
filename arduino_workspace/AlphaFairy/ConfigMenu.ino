#include "AlphaFairy.h"

extern configsettings_t config_settings;

const configitem_t config_items[] = {
  // item pointer                                     , max , min , step , text                     , flags
  { (int32_t*)&(config_settings.focus_pause_time_ms  ), 1000,    0,    10, "focus pause"            , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.shutter_press_time_ms), 1000,    0,    10, "shutter press duration" , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.manual_focus_return  ),    1,    0,     1, "manual focus return"    , CFGFMT_BOOL   },
  { (int32_t*)&(config_settings.nine_point_dist      ),  240,    0,    10, "9 point dist"           , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.infrared_enabled     ),    1,    0,     1, "infrared enabled"       , CFGFMT_BOOL   },
  { (int32_t*)&(config_settings.gpio_enabled         ),    1,    0,     1, "gpio enabled"           , CFGFMT_BOOL   },
  { NULL, 0, 0, 0, "" }, // end of table
};

menustate_t menustate_confsettings;


void conf_settings(void* mip)
{
  menustate_confsettings.idx = 0;
  // count the number of items
  for (menustate_confsettings.cnt = 0; ; menustate_confsettings.cnt++) {
    if (config_items[menustate_confsettings.cnt].ptr_val == NULL) {
      break;
    }
  }

  gui_startPrint();

  while (true)
  {
    app_poll();

    if (btnSide_hasPressed(true))
    {
      menustate_confsettings.idx = (menustate_confsettings.idx >= menustate_confsettings.cnt) ? 0 : (menustate_confsettings.idx + 1);
      M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
    }

    if (batt_need_recheck) {
      batt_need_recheck = false;
      M5Lcd.fillScreen(TFT_BLACK);
      if (is_low_batt()) {
        battlow_draw(true);
      }
    }

    configitem_t* cfgitm = (configitem_t*)&(config_items[menustate_confsettings.idx]);
    if (menustate_confsettings.idx == menustate_confsettings.cnt) // last item is the exit item
    {
      M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
      M5Lcd.print("Save and Go Back");
      if (btnBig_hasPressed(true))
      {
        settings_save();
        return;
      }
    }
    else
    {
      configitem_t* cfgitm = (configitem_t*)&(config_items[menustate_confsettings.idx]);
      // first line shows name of item, second line shows the value
      M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
      M5Lcd.print(cfgitm->text);
      M5Lcd.println();
      gui_setCursorNextLine();
      gui_valIncDec(cfgitm);
    }
    if (is_low_batt()) {
      battlow_draw(true);
    }
  }
}
