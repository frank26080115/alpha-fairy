#include "AlphaFairy.h"
#include <M5DisplayExt.h>
#include "FairyMenu.h"

void intervalometer_func(void* ptr)
{
    PageInterval* pg = (PageInterval*)ptr;
    uint16_t caller_id = pg->get_parentId();
}

class PageInterval : public FairyCfgItem
{
    public:
        PageInterval(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint16_t fmt_flags)
                    : FairyCfgItem(disp_name, linked_var, val_min, val_max, step_size, fmt_flags)
                    {
                        _autosave = true;
                    };

        PageInterval(const char* disp_name, bool (*cb)(void*), const char* icon = NULL)
                    : FairyCfgItem(disp_name, cb, icon)
                    {
                    };

        void draw_total(void)
        {
            uint32_t total_time = interval_calcTotal(_parent_id);
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(_margin_x, get_y(2));
            if (total_time > 0) {
                M5Lcd.print("Total: ");
                gui_showVal(total_time, TXTFMT_TIMELONG, (Print*)&M5Lcd);
            }
            blank_line();
        };

        virtual void on_readjust(void)
        {
            if (is_value() == false) {
                return;
            }
            draw_total();
        };
};

class AppIntervalometer : public FairyCfgApp
{
    public:
        AppIntervalometer(uint16_t id) :
            FairyCfgApp(id == MENUITEM_INTERVAL ? "/main_interval.png" : "/main_astro.png",
                        id == MENUITEM_INTERVAL ? "/intervalometer_icon.png" : "/galaxy_icon.png",
                        id
                        )
            {
                if (id == MENUITEM_INTERVAL)
                {
                    install(new PageInterval("Bulb Time", (int32_t*)&(config_settings.intv_bulb)  , 0, 10000, 1, TXTFMT_TIME | TXTFMT_BULB);
                    install(new PageInterval("Interval" , (int32_t*)&(config_settings.intv_intval), 0, 10000, 1, TXTFMT_TIME);
                }
                else if (id == MENUITEM_ASTRO)
                {
                    install(new PageInterval("Bulb Time", (int32_t*)&(config_settings.astro_bulb) , 0, 10000, 1, TXTFMT_TIME | TXTFMT_BULB);
                    install(new PageInterval("Pause Gap", (int32_t*)&(config_settings.astro_pause), 0, 10000, 1, TXTFMT_TIME);
                }
                install(new PageInterval("Start Delay" , (int32_t*)&(config_settings.intv_delay), 0, 10000, 1, TXTFMT_TIME);
                install(new PageInterval("Num of Shots", (int32_t*)&(config_settings.intv_limit), 0, 10000, 1, TXTFMT_BYTENS);
                install(new PageInterval("Start", intervalometer_func, "/start_icon.png");
            };
};

extern FairySubmenu main_menu;
void setup_intervalometer()
{
    static AppIntervalometer app_interval(MENUITEM_INTERVAL);
    static AppIntervalometer app_astro   (MENUITEM_ASTRO);
    main_menu.install(&app_interval);
    main_menu.install(&app_astro);
}