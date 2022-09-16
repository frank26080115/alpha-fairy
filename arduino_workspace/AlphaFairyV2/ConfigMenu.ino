#include "AlphaFairy.h"
#include "FairyMenu.h"

static bool has_saved = false;

bool config_save_exit(void*)
{
    has_saved = true;
    settings_save();
    return true;
}

class PageLcdBrightness : public FairyCfgItem
{
    public:
        PageLcdBrightness(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint16_t fmt_flags) : FairyCfgItem(disp_name, linked_var, val_min, val_max, step_size, fmt_flags)
        {
        };

        virtual void on_readjust(void)
        {
            M5.Axp.ScreenBreath(config_settings.lcd_brightness);
        };

        virtual void on_eachFrame(void)
        {
            pwr_tick(true);
        };
};

class AppConfigMenu : public FairyCfgApp
{
    public:
        AppConfigMenu() : FairyCfgApp("/config.png", "/config_icon.png", MENUITEM_CONFIG)
        {
install(new FairyCfgItem("focus pause"            , (int32_t*)&(config_settings.focus_pause_time_ms    ),    0, 1000,    10, TXTFMT_BYTENS   ));
install(new FairyCfgItem("MF knob steps"          , (int32_t*)&(config_settings.fenc_multi             ), -100,  100,     1, TXTFMT_NONE     ));
install(new FairyCfgItem("MF knob large steps"    , (int32_t*)&(config_settings.fenc_large             ),    0, 1000,     1, TXTFMT_NONE     ));
install(new FairyCfgItem("shutter press duration" , (int32_t*)&(config_settings.shutter_press_time_ms  ),    0, 1000,    10, TXTFMT_BYTENS   ));
install(new FairyCfgItem("MF return"              , (int32_t*)&(config_settings.manual_focus_return    ),    0,    1,     1, TXTFMT_BOOL     ));
install(new FairyCfgItem("9-pt dist"              , (int32_t*)&(config_settings.nine_point_dist        ),    0,  240,    10, TXTFMT_BYTENS   ));
install(new FairyCfgItem("Tv step size"           , (int32_t*)&(config_settings.shutter_speed_step_cnt ),    1,    9,     1, TXTFMT_BYTENS   ));
install(new FairyCfgItem("Tv step delay"          , (int32_t*)&(config_settings.shutter_step_time_ms   ),    0, 5000,    10, TXTFMT_BYTENS   ));
install(new FairyCfgItem("Tally Light en"         , (int32_t*)&(config_settings.tallylite              ),    0,    1,     1, TXTFMT_BOOL     ));
install(new FairyCfgItem("power save time (s)"    , (int32_t*)&(config_settings.pwr_save_secs          ),    0, 1000,    10, TXTFMT_BYTENS   ));
install(new PageLcdBrightness("LCD bright"        , (int32_t*)&(config_settings.lcd_brightness         ),    7,   12,     1, TXTFMT_LCDBRITE ));
install(new FairyCfgItem("LCD dim time (s)"       , (int32_t*)&(config_settings.lcd_dim_secs           ),    0, 1000,     1, TXTFMT_BYTENS   ));
install(new FairyCfgItem("WiFi power"             , (int32_t*)&(config_settings.wifi_pwr               ),    0,   14,     1, TXTFMT_BYTENS   ));
install(new FairyCfgItem("LED en"                 , (int32_t*)&(config_settings.led_enabled            ),    0,    1,     1, TXTFMT_BOOL     ));
install(new FairyCfgItem("IR en"                  , (int32_t*)&(config_settings.infrared_enabled       ),    0,    1,     1, TXTFMT_BOOL     ));
install(new FairyCfgItem("GPIO en"                , (int32_t*)&(config_settings.gpio_enabled           ),    0,    1,     1, TXTFMT_BOOL     ));
install(new FairyCfgItem("Save + Exit", config_save_exit, "/back_icon.png"));
        };

        virtual bool on_execute(void)
        {
            has_saved = false;
            if (_backup == NULL) {
                _backup = (configsettings_t*)malloc(sizeof(configsettings_t));
            }
            memcpy(_backup, &config_settings, sizeof(configsettings_t)); // allows for changes to be undone

            bool exit = FairyCfgApp::on_execute();

            if (exit)
            {
                if (has_saved == false) {
                    memcpy(&config_settings, _backup, sizeof(configsettings_t)); // restore settings to unchanged
                    M5.Axp.ScreenBreath(config_settings.lcd_brightness);
                }
                if (_backup != NULL) {
                    free(_backup);
                    _backup = NULL;
                }
            }
            return exit;
        }

    protected:
        configsettings_t* _backup = NULL;
};

extern FairySubmenu menu_utils;
void setup_configmenu(void)
{
    static AppConfigMenu app;
    menu_utils.install(&app);
}
