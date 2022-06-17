#include "AlphaFairy.h"

extern configsettings_t config_settings;

const configitem_t config_items[] = {
  // item pointer                                     , max , min , step , text
  { (int32_t*)&(config_settings.focus_pause_time_ms  ), 1000,    0,    25, "focus pause"            },
  { (int32_t*)&(config_settings.shutter_press_time_ms), 1000,    0,    25, "shutter press duration" },
  { (int32_t*)&(config_settings.manual_focus_return  ),    1,    0,     1, "manual focus return"    },
  { (int32_t*)&(config_settings.nine_point_dist      ),  240,    0,    20, "9 point dist"           },
  { (int32_t*)&(config_settings.infrared_enabled     ),    1,    0,     1, "infrared enabled"       },
  { NULL, 0, 0, 0, "" }, // end of table
};

uint8_t submenu_idx = 0;
uint8_t submenu_cnt = 0;

void conf_settings(void* mip)
{
  submenu_idx = 0;
  // count the number of items
  for (submenu_cnt = 0; ; submenu_cnt++) {
    if (config_items[submenu_cnt].ptr_val == NULL) {
      break;
    }
  }

  gui_startPrint();

  while (true)
  {
    app_poll();

    if (btnSide_hasPressed(true))
    {
      submenu_idx = (submenu_idx >= submenu_cnt) ? 0 : (submenu_idx + 1);
      M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
    }

    configitem_t* cfgitm = (configitem_t*)&(config_items[submenu_idx]);
    if (submenu_idx == submenu_cnt) // last item is the exit item
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
      configitem_t* cfgitm = (configitem_t*)&(config_items[submenu_idx]);
      // first line shows name of item, second line shows the value
      M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
      M5Lcd.print(cfgitm->text);
      M5Lcd.println();
      gui_setCursorNextLine();
      int32_t* ptr_x = (int32_t*)(cfgitm->ptr_val);
      int32_t x = *ptr_x;
      bool is_bool = false;
      if (cfgitm->val_max == 1 && cfgitm->val_min == 0 && cfgitm->step_size == 1)
      {
          // I'm too lazy to define a "type" field, so just imply that a 0-1 value is a boolean value
          is_bool = true;
          if (x == 0) {
              M5Lcd.print("no ");
          }
          else {
              M5Lcd.print("yes ");
          }
      }
      else
      {
        M5Lcd.print(x, DEC);
      }
      M5Lcd.print(" ");
      // show if user is about to increase or decrease value based on IMU reported angle
      if (imu_angle == ANGLE_IS_UP)
      {
          M5Lcd.print("+ ");
          if (btnBig_hasPressed(true)) // change the value on button press
          {
              (*ptr_x) += cfgitm->step_size;
              if ((*ptr_x) > cfgitm->val_max) { // limit the range
                  (*ptr_x) = cfgitm->val_max;
              }
          }
      }
      else if (imu_angle == ANGLE_IS_DOWN)
      {
          M5Lcd.print("- ");
          if (btnBig_hasPressed(true)) // change the value on button press
          {
              (*ptr_x) -= cfgitm->step_size;
              if ((*ptr_x) < cfgitm->val_min) { // limit the range
                  (*ptr_x) = cfgitm->val_min;
              }
          }
      }
      else
      {
          M5Lcd.print("  ");
      }
    }
  }
}
