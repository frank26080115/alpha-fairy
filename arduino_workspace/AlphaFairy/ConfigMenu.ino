#include "AlphaFairy.h"

extern configsettings_t config_settings;

const configitem_t config_items[] = {
  // item pointer                                       , max , min , step , text                     , flags
  { (int32_t*)&(config_settings.focus_pause_time_ms    ), 1000,    0,    10, "focus pause"            , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.shutter_press_time_ms  ), 1000,    0,    10, "shutter press duration" , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.manual_focus_return    ),    1,    0,     1, "MF return"              , CFGFMT_BOOL   },
  { (int32_t*)&(config_settings.nine_point_dist        ),  240,    0,    10, "9-pt dist"              , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.shutter_speed_step_cnt ),    9,    1,     1, "Tv step size"           , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.shutter_step_time_ms   ), 1000,    0,    10, "Tv step delay"          , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.pwr_save_secs          ), 1000,    0,    10, "power save time (s)"    , CFGFMT_BYTENS },
  { (int32_t*)&(config_settings.led_enabled            ),    1,    0,     1, "LED en"                 , CFGFMT_BOOL   },
  { (int32_t*)&(config_settings.infrared_enabled       ),    1,    0,     1, "IR en"                  , CFGFMT_BOOL   },
  { (int32_t*)&(config_settings.gpio_enabled           ),    1,    0,     1, "GPIO en"                , CFGFMT_BOOL   },
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
    pwr_sleepCheck();

    if (redraw_flag) {
        redraw_flag = false;
        gui_startPrint();
        M5Lcd.fillScreen(TFT_BLACK);
        conf_drawIcon();
    }

    if (btnSide_hasPressed(true))
    {
      menustate_confsettings.idx = (menustate_confsettings.idx >= menustate_confsettings.cnt) ? 0 : (menustate_confsettings.idx + 1);
      M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
      conf_drawIcon();
      pwr_tick();
      redraw_flag = false;
    }

    configitem_t* cfgitm = (configitem_t*)&(config_items[menustate_confsettings.idx]);
    if (menustate_confsettings.idx == menustate_confsettings.cnt) // last item is the exit item
    {
      M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
      M5Lcd.setTextFont(4);
      M5Lcd.print("Save + Exit");
      M5Lcd.drawPngFile(SPIFFS, "/back_icon.png", M5Lcd.width() - 60, 0);
      if (btnBig_hasPressed(true))
      {
        pwr_tick();
        settings_save();
        return;
      }
    }
    else
    {
      configitem_t* cfgitm = (configitem_t*)&(config_items[menustate_confsettings.idx]);
      // first line shows name of item, second line shows the value
      M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
      if (strlen(cfgitm->text) < 14) {
          M5Lcd.setTextFont(4);
      }
      else {
          M5Lcd.setTextFont(2);
      }
      M5Lcd.print(cfgitm->text);
      M5Lcd.setTextFont(4);
      M5Lcd.println();
      gui_setCursorNextLine();
      gui_valIncDec(cfgitm);
    }
    conf_drawIcon();
  }
}

void conf_drawIcon()
{
    gui_drawStatusBar(true);
    M5Lcd.drawPngFile(SPIFFS, "/config_icon.png", M5Lcd.width() - 60, M5Lcd.height() - 60);
}