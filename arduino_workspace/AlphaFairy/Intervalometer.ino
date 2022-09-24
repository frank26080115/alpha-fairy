#include "AlphaFairy.h"
#include <M5DisplayExt.h>
#include "FairyMenu.h"

static uint32_t intervalometer_start_time;

void interval_drawTimer(int8_t x)
{
    static uint8_t i = 0;
    char fname[24];
    if (x < 0) { // negative arg means auto-increment
        i++;
    }
    else {
        i = x; // otherwise, assign
    }
    i %= 12; // only 12 animation frames available
    sprintf(fname, "/timer_%u.png", i);

    // note: this is the fastest animation in the entire project, best to use the sprite manager

    if ((sprites->holder_flag & SPRITESHOLDER_FOCUSPULL) == 0) {
        sprites->draw(fname, M5Lcd.width() - 60, M5Lcd.height() - 60, 60, 60);
        sprites->holder_flag |= SPRITESHOLDER_INTERVAL;
    }
    else {
        M5Lcd.drawPngFile(SPIFFS, fname, M5Lcd.width() - 60, M5Lcd.height() - 60);
    }

    if (i == 0) {
        gui_drawStatusBar(true);
    }
}

int32_t interval_calcTotal(uint8_t menu_id)
{
    int32_t total_time = 0;
    int32_t shot_cnt = 0;
    if (menu_id == MENUITEM_INTERVAL) {
        shot_cnt = config_settings.intv_limit;
        total_time = shot_cnt * config_settings.intv_intval;
    }
    else if (menu_id == MENUITEM_ASTRO) {
        shot_cnt = config_settings.intv_limit;
        if (config_settings.astro_bulb == 0) {
            total_time = shot_cnt * config_settings.astro_pause;
        }
        else {
            total_time = shot_cnt * (config_settings.astro_bulb + config_settings.astro_pause);
        }
    }
    return total_time;
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

        virtual void on_readjust(void)
        {
            if (is_value() == false) {
                return;
            }
            draw_total();
        };

        virtual void on_redraw(void)
        {
            FairyCfgItem::on_redraw();
            if (is_func())
            {
                draw_start();
            }
            else if (is_value())
            {
                draw_total();
            }
        };

    protected:
        void draw_total(void)
        {
            int32_t total_time = interval_calcTotal(_parent_id);
            M5Lcd.setTextFont(4);
            M5Lcd.setCursor(_margin_x, get_y(2));
            if (total_time > 0) {
                M5Lcd.print("Total: ");
                gui_showVal(total_time, TXTFMT_TIMELONG, (Print*)&M5Lcd);
            }
            blank_line();
        };

        void draw_start(void)
        {
            FairyCfgItem::draw_name();
            M5Lcd.setTextFont(4);
            int linenum = 1;
            M5Lcd.setCursor(_margin_x, get_y(linenum));
            if (_parent_id == MENUITEM_INTERVAL)
            {
                gui_showVal(config_settings.intv_intval, TXTFMT_TIME, (Print*)&M5Lcd);
                if (config_settings.intv_bulb != 0) {
                    M5Lcd.setTextFont(2);
                    M5Lcd.print(" (B: ");
                    gui_showVal(config_settings.intv_bulb, TXTFMT_TIME, (Print*)&M5Lcd);
                    M5Lcd.print(")");
                    M5Lcd.setTextFont(4);
                    blank_line();
                }
                if (config_settings.intv_limit != 0 && config_settings.intv_limit < 1000) {
                    linenum++;
                    M5Lcd.setCursor(_margin_x, get_y(linenum));
                    M5Lcd.print("#: ");
                    M5Lcd.print(config_settings.intv_limit, DEC);
                    M5Lcd.print("x");
                }
            }
            else if (_parent_id == MENUITEM_ASTRO)
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
                blank_line();
                if (config_settings.intv_limit != 0 && config_settings.intv_limit < 1000) {
                    linenum++;
                    M5Lcd.setCursor(_margin_x, get_y(linenum));
                    M5Lcd.print("#: ");
                    M5Lcd.print(config_settings.intv_limit, DEC);
                    M5Lcd.print("x");
                }
            }

            int32_t total_time = interval_calcTotal(_parent_id);

            if (total_time > 0)
            {
                linenum++;
                M5Lcd.setCursor(_margin_x, get_y(linenum));
                M5Lcd.setTextFont(2);
                M5Lcd.print("T: ");
                gui_showVal(total_time, TXTFMT_TIMELONG, (Print*)&M5Lcd);
                blank_line();
            }

            FairyCfgItem::draw_icon();
        };
};

bool intervalometer_func(void* ptr)
{
    FairyItem* pg = (FairyItem*)ptr;
    uint16_t caller_id = pg->get_parentId(); // need to know if this is normal intervalometer or astrophotography intervalometer

    uint32_t t = millis(), now = t;
    intervalometer_start_time = t;

    bool stop_flag = false, stop_request = false;

    int32_t cnt = config_settings.intv_limit;
    cnt = cnt <= 0 ? -1 : cnt; // zero means infinite, indicated by negative

    uint32_t bulb         = (caller_id == MENUITEM_ASTRO) ?  config_settings.astro_bulb                                : config_settings.intv_bulb;
    int32_t  intv_time    = (caller_id == MENUITEM_ASTRO) ?  config_settings.astro_pause                               : config_settings.intv_intval;
    int32_t  total_period = (caller_id == MENUITEM_ASTRO) ? (config_settings.astro_bulb + config_settings.astro_pause) : config_settings.intv_intval;

    // prep screen for drawing
    gui_startAppPrint();
    M5Lcd.fillScreen(TFT_BLACK);
    interval_drawTimer(0); // reset the icon
    app_waitAllRelease();
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    M5Lcd.setTextFont(4);

    // wait the starting countdown if required
    if (caller_id != MENUITEM_TRIGGER)
    {
        if (config_settings.intv_delay > 0) {
            stop_flag |= intervalometer_wait(config_settings.intv_delay, t, cnt, "Start in...", false, total_period);
        }
    }

    // handle early quit
    if (stop_flag) {
        return false;
    }

    t = now;
    pwr_tick(true);
    imu.hasMajorMotion = false;

    // for the number of frames we want (or infinite if negative)
    for (; cnt != 0 && stop_flag == false; )
    {
        app_poll();
        pwr_tick(false); // app_poll already checks for IMU motion to undim the LCD

        if (redraw_flag) {
            redraw_flag = false;
            gui_startAppPrint();
            M5Lcd.fillScreen(TFT_BLACK);
            M5Lcd.setTextFont(4);
        }

        if (btnSide_hasPressed())
        {
            stop_flag = true;
            btnSide_clrPressed();
            break;
        }

        if (btnPwr_hasPressed())
        {
            btnPwr_clrPressed();
            break;
        }

        if (btnBig_hasPressed())
        {
            btnBig_clrPressed();
            // do nothing, clear the press so it doesn't queue up
        }

        interval_drawTimer(-1);

        t = millis();
        if (bulb == 0)
        {
            // shoot photo normally and indicate
            cam_shootQuick();
            if (intv_time <= 0 && bulb <= 0)
            {
                M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
                M5Lcd.print("SHOOT!");
                gui_blankRestOfLine();
            }
        }
        else
        {
            // bulb mode, open the shutter and wait the specified time before closing
            cam_shootOpen();
            stop_flag |= intervalometer_wait(bulb, t, cnt, "Shutter Open", true, total_period);
            cam_shootClose();
        }
        cnt--;

        if (stop_flag) {
            break;
        }
        interval_drawTimer(-1);

        if (caller_id == MENUITEM_ASTRO) {
            // astro mode uses a pause gap instead of fixed intervals, so remember timestamp here
            t = millis();
        }

        if (intv_time > 0)
        {
            stop_flag |= intervalometer_wait(intv_time, t, cnt, (bulb != 0) ? "Next in..." : "Interval", false, total_period);
            if (stop_flag) {
                break;
            }
            interval_drawTimer(-1);
        }

        if (intv_time <= 0 && bulb <= 0)
        {
            // this is a special case when the intervalometer is just spamming the shutter
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.print("Timer Active");
            gui_blankRestOfLine();
        }

    } // end of for-loop

    redraw_flag = true; // force parent to redraw
    app_waitAllRelease();
    return false;
}

extern bool gui_microphoneActive;

bool intervalometer_wait(
        int32_t     twait         // number of seconds to wait
      , uint32_t    tstart        // start time, in milliseconds, of the wait
      , int32_t     cnt           // number of photos remaining
      , const char* msg           // message to show in the first line of text
      , bool        pausable      // if true, then a side button press will cause the pause to happen at the end of bulb
      , int32_t     total_period  // total time period for each interval, in seconds, for display purposes
      )
{
    uint32_t now, telapsed;
    bool stop_flag = false, stop_request = false;
    bool need_blank = false, need_icon = false;
    if (twait < 0) {
        return false;
    }
    twait *= 1000;

    while ((telapsed = ((now = millis()) - tstart)) < twait)
    {
        app_poll();

        if (redraw_flag) {
            redraw_flag = false;
            gui_startAppPrint();
            M5Lcd.fillScreen(TFT_BLACK);
            M5Lcd.setTextFont(4);
        }

        if (btnSide_hasPressed())
        {
            btnSide_clrPressed();
            if (pausable)
            {
                if (stop_request == false) {
                    stop_request = true;

                    need_blank = true; // these actions need to only happen once
                    need_icon = true;  // these actions need to only happen once
                }
                else if (stop_flag != false) {
                    // a release means two button press, if a second press is detected, quit immediately
                    break;
                }
            }
            else
            {
                stop_flag = true;
                break;
            }
        }
        if (btnSide_isPressed() == false && stop_request != false) {
            // a release means two button press, if a second press is detected, quit immediately
            stop_flag = true;
        }

        if (btnPwr_hasPressed())
        {
            stop_flag = true;
            stop_request = true;
            btnPwr_clrPressed();
            break;
        }

        M5Lcd.setCursor(SUBMENU_X_OFFSET, gui_microphoneActive == false ? SUBMENU_Y_OFFSET : MICTRIG_LEVEL_MARGIN);
        if (stop_request == false) {
            M5Lcd.print(msg);
        }
        else {
            // stop has been requested
            M5Lcd.print("Stop in...");
        }
        gui_blankRestOfLine();

        M5Lcd.println();
        gui_setCursorNextLine();
        gui_showVal(twait - telapsed, TXTFMT_TIMEMS, (Print*)&M5Lcd); // show remaining time
        gui_blankRestOfLine();

        if (cnt > 0 && stop_request == false) {
            M5Lcd.println();
            gui_setCursorNextLine();
            M5Lcd.print("# Rem: ");
            M5Lcd.print(cnt, DEC);
            gui_blankRestOfLine();
            if (total_period > 0) // only show if data is available
            {
                uint32_t total_time = total_period * cnt;
                M5Lcd.println();
                gui_setCursorNextLine();
                if (total_time > 120 && cnt > 5) { // this counter isn't live, so don't show it if it needs to be precise
                    M5Lcd.print("T Rem: ");
                    gui_showVal(total_time, TXTFMT_TIMELONG, (Print*)&M5Lcd);
                }
                gui_blankRestOfLine();
            }
        }
        else if (need_blank) {
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_blankRestOfLine();
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_blankRestOfLine();
            need_blank = false; // do only once
        }
        if (stop_request && need_icon) {
            M5Lcd.drawPngFile(SPIFFS, "/back_icon.png", M5Lcd.width() - 60, 0);
            need_icon = false; // do only once, SPI flash file read and file decoding is extremely slow
        }
        interval_drawTimer(-1);
    }

    // make sure 0 is the last number shown
    if (stop_request == false)
    {
        M5Lcd.setCursor(SUBMENU_X_OFFSET, gui_microphoneActive == false ? SUBMENU_Y_OFFSET : MICTRIG_LEVEL_MARGIN);
        M5Lcd.print(msg);
        M5Lcd.println();
        gui_setCursorNextLine();
        gui_showVal(0, TXTFMT_TIMEMS, (Print*)&M5Lcd); // show remaining time of zero
        gui_blankRestOfLine();
    }

    return stop_flag;
}

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
                    install(new PageInterval("Bulb Time", (int32_t*)&(config_settings.intv_bulb)  , 0, 10000, 1, TXTFMT_TIME | TXTFMT_BULB));
                    install(new PageInterval("Interval" , (int32_t*)&(config_settings.intv_intval), 0, 10000, 1, TXTFMT_TIME));
                }
                else if (id == MENUITEM_ASTRO)
                {
                    install(new PageInterval("Bulb Time", (int32_t*)&(config_settings.astro_bulb) , 0, 10000, 1, TXTFMT_TIME | TXTFMT_BULB));
                    install(new PageInterval("Pause Gap", (int32_t*)&(config_settings.astro_pause), 0, 10000, 1, TXTFMT_TIME));
                }
                install(new PageInterval("Start Delay" , (int32_t*)&(config_settings.intv_delay), 0, 10000, 1, TXTFMT_TIME));
                install(new PageInterval("Num of Shots", (int32_t*)&(config_settings.intv_limit), 0, 10000, 1, TXTFMT_BYTENS));
                install(new PageInterval("Start", intervalometer_func, "/go_icon.png"));
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
