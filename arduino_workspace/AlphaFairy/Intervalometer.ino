#include "AlphaFairy.h"
#include <M5DisplayExt.h>

const configitem_t intval_config_generic[] = {
  // item pointer                           ,  max , min , step , text           , flags
  { (int32_t*)&(config_settings.intv_bulb  ), 10000,    0,     1, "Bulb Time"    , TXTFMT_TIME | TXTFMT_BULB },
  { (int32_t*)&(config_settings.intv_intval), 10000,    0,     1, "Interval"     , TXTFMT_TIME               },
  { (int32_t*)&(config_settings.intv_delay ), 10000,    0,     1, "Start Delay"  , TXTFMT_TIME               },
  { (int32_t*)&(config_settings.intv_limit ), 10000,    1,     1, "Num of Shots" , TXTFMT_BYTENS             },
  { NULL, 0, 0, 0, "" }, // end of table
};

const configitem_t intval_config_astro[] = {
  // item pointer                            ,  max , min , step , text           , flags
  { (int32_t*)&(config_settings.astro_bulb  ), 10000,    0,     5, "Bulb Time"    , TXTFMT_TIME | TXTFMT_BULB },
  { (int32_t*)&(config_settings.astro_pause ), 10000,    0,     1, "Pause Gap"    , TXTFMT_TIME               },
  { (int32_t*)&(config_settings.intv_delay  ), 10000,    0,     1, "Start Delay"  , TXTFMT_TIME               },
  { (int32_t*)&(config_settings.intv_limit  ), 10000,    1,     1, "Num of Shots" , TXTFMT_BYTENS             },
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

    gui_startAppPrint();
    M5Lcd.fillScreen(TFT_BLACK);
    interval_drawIcon(menuitm->id);
    app_waitAllRelease(BTN_DEBOUNCE);

    while (true)
    {
        app_poll();
        pwr_sleepCheck();

        if (redraw_flag) {
            redraw_flag = false;
            gui_startAppPrint();
            M5Lcd.fillScreen(TFT_BLACK);
            conf_drawIcon();
        }

        if (btnSide_hasPressed(true))
        {
            m->idx = (m->idx >= m->cnt) ? 0 : (m->idx + 1);
            M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
            interval_drawIcon(menuitm->id);
            redraw_flag = false;
            pwr_tick();
        }
        #if defined(USE_PWR_BTN_AS_BACK) && !defined(USE_PWR_BTN_AS_EXIT)
        if (btnPwr_hasPressed(true))
        {
            m->idx = (m->idx <= 0) ? m->cnt : (m->idx - 1);
            M5Lcd.fillScreen(TFT_BLACK); // item has changed so clear the screen
            interval_drawIcon(menuitm->id);
            redraw_flag = false;
            pwr_tick();
        }
        #endif

        configitem_t* cfgitm = (configitem_t*)&(config_items[m->idx]);
        if (m->idx == m->cnt) // last item is the exit item
        {
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.setTextFont(4);
            M5Lcd.print("Exit");
            gui_drawBackIcon();
            if (btnBig_hasPressed(true))
            {
                pwr_tick();
                settings_save();
                return;
            }
        }
        else if (m->idx == m->cnt - 1) // 2nd to last item is the start/stop item
        {
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.setTextFont(4);
            M5Lcd.print("Start");
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_drawGoIcon();
            M5Lcd.setTextFont(4);

            if (menuitm->id == MENUITEM_INTERVAL)
            {
                if (config_settings.intv_bulb != 0) {
                    M5Lcd.print("Bulb: ");
                    gui_showVal(config_settings.intv_bulb, TXTFMT_TIME, (Print*)&M5Lcd);
                    M5Lcd.println();
                    gui_setCursorNextLine();
                    M5Lcd.print("Intv: ");
                }
                gui_showVal(config_settings.intv_intval, TXTFMT_TIME, (Print*)&M5Lcd);
                if (config_settings.intv_limit != 0 && config_settings.intv_limit < 1000) {
                    M5Lcd.println();
                    gui_setCursorNextLine();
                    M5Lcd.print("#: ");
                    M5Lcd.print(config_settings.intv_limit, DEC);
                    M5Lcd.print("x");
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
                    M5Lcd.println();
                    gui_setCursorNextLine();
                    M5Lcd.print("#: ");
                    M5Lcd.print(config_settings.intv_limit, DEC);
                    M5Lcd.print("x");
                }
            }
            gui_blankRestOfLine();

            uint32_t total_time = interval_calcTotal(menuitm->id);

            M5Lcd.println();
            gui_setCursorNextLine();
            M5Lcd.setTextFont(2);
            if (total_time == 0) {
                gui_blankRestOfLine();
            }
            else {
                M5Lcd.print("T: ");
                gui_showVal(total_time, TXTFMT_TIMELONG, (Print*)&M5Lcd);
                gui_blankRestOfLine();
            }

            if (btnBig_hasPressed(true))
            {
                pwr_tick();
                settings_save();
                ledblink_setMode(LEDMODE_OFF);
                intervalometer_run(menuitm->id);
                ledblink_setMode(LEDMODE_NORMAL);
                gui_startAppPrint();
                M5Lcd.fillScreen(TFT_BLACK);
                interval_drawIcon(menuitm->id);
                app_waitAllRelease(BTN_DEBOUNCE);
                pwr_tick();
            }
        }
        else
        {
            configitem_t* cfgitm = (configitem_t*)&(ctable[m->idx]);
            // first line shows name of item, second line shows the value
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.setTextFont(4);
            M5Lcd.print(cfgitm->text);
            M5Lcd.println();
            gui_setCursorNextLine();
            gui_valIncDec(cfgitm);

            uint32_t total_time = interval_calcTotal(menuitm->id);

            M5Lcd.println();
            gui_setCursorNextLine();
            M5Lcd.setTextFont(4);
            if (total_time == 0) {
                gui_blankRestOfLine();
            }
            else {
                M5Lcd.print("Total: ");
                gui_showVal(total_time, TXTFMT_TIMELONG, (Print*)&M5Lcd);
                gui_blankRestOfLine();
            }
        }

        #ifdef USE_PWR_BTN_AS_EXIT
        if (btnPwr_hasPressed(true))
        {
            pwr_tick();
            return;
        }
        #endif

        interval_drawIcon(menuitm->id);
    }
}

void interval_drawIcon(uint8_t id)
{
    if (id == MENUITEM_INTERVAL) {
        #ifdef USE_SPRITE_MANAGER
        sprites->draw(
        #else
        M5Lcd.drawPngFile(SPIFFS,
        #endif
            "/intervalometer_icon.png", M5Lcd.width() - 60, M5Lcd.height() - 60
        #ifdef USE_SPRITE_MANAGER
            , 60, 60
        #endif
            );
    }
    else if (id == MENUITEM_ASTRO) {
        #ifdef USE_SPRITE_MANAGER
        sprites->draw(
        #else
        M5Lcd.drawPngFile(SPIFFS,
        #endif
            "/galaxy_icon.png", M5Lcd.width() - 60, M5Lcd.height() - 60
        #ifdef USE_SPRITE_MANAGER
            , 60, 60
        #endif
            );
    }
    gui_drawStatusBar(true);
}

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
    i %= 12;
    sprintf(fname, "/timer_%u.png", i);

    #ifdef USE_SPRITE_MANAGER
    if ((sprites->holder_flag & SPRITESHOLDER_FOCUSPULL) == 0) {
        sprites->draw(fname, M5Lcd.width() - 60, M5Lcd.height() - 60, 60, 60);
        sprites->holder_flag |= SPRITESHOLDER_INTERVAL;
    }
    else {
        M5Lcd.drawPngFile(SPIFFS, fname, M5Lcd.width() - 60, M5Lcd.height() - 60);
    }
    #else
    M5Lcd.drawPngFile(SPIFFS, fname, M5Lcd.width() - 60, M5Lcd.height() - 60);
    #endif

    if (i == 0) {
        gui_drawStatusBar(true);
    }
}

uint32_t interval_calcTotal(uint8_t menu_id)
{
    uint32_t total_time = 0, shot_cnt = 0;
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

static uint32_t intervalometer_start_time;

void intervalometer_run(uint8_t id)
{
    uint32_t t = millis(), now = t;
    intervalometer_start_time = t;

    bool stop_flag = false, stop_request = false;

    int32_t cnt = config_settings.intv_limit;
    cnt = cnt <= 0 ? -1 : cnt; // zero means infinite, indicated by negative

    uint32_t bulb         = (id == MENUITEM_ASTRO) ?  config_settings.astro_bulb                                : config_settings.intv_bulb;
    int32_t  intv_time    = (id == MENUITEM_ASTRO) ?  config_settings.astro_pause                               : config_settings.intv_intval;
    int32_t  total_period = (id == MENUITEM_ASTRO) ? (config_settings.astro_bulb + config_settings.astro_pause) : config_settings.intv_intval;

    gui_startAppPrint();
    M5Lcd.fillScreen(TFT_BLACK);
    interval_drawTimer(0); // reset the icon
    app_waitAllRelease(BTN_DEBOUNCE);
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    M5Lcd.setTextFont(4);

    if (config_settings.intv_delay > 0) {
        stop_flag |= intervalometer_wait(config_settings.intv_delay, t, cnt, "Start in...", false, total_period);
    }

    if (stop_flag) {
        return;
    }

    t = now;

    // for the number of frames we want (or infinite if negative)
    for (; cnt != 0 && stop_flag == false; )
    {
        pwr_tick();
        app_poll();

        if (redraw_flag) {
            redraw_flag = false;
            gui_startAppPrint();
            M5Lcd.fillScreen(TFT_BLACK);
            M5Lcd.setTextFont(4);
        }

        if (btnSide_hasPressed(true))
        {
            pwr_tick();
            stop_flag = true;
            break;
        }

        #ifdef USE_PWR_BTN_AS_EXIT
        if (btnPwr_hasPressed(true))
        {
            pwr_tick();
            break;
        }
        #endif

        interval_drawTimer(-1);

        t = millis();
        if (bulb == 0)
        {
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
            cam_shootOpen();
            stop_flag |= intervalometer_wait(bulb, t, cnt, "Shutter Open", true, total_period);
            cam_shootClose();
        }
        cnt--;

        if (stop_flag) {
            break;
        }
        interval_drawTimer(-1);

        if (id == MENUITEM_ASTRO) {
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
            M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
            M5Lcd.print("Timer Active");
            gui_blankRestOfLine();
        }
    }
}

bool intervalometer_wait(int32_t twait, uint32_t tstart, int32_t cnt, const char* msg, bool pausable, int32_t total_period)
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
        pwr_tick();
        app_poll();

        if (redraw_flag) {
            redraw_flag = false;
            gui_startAppPrint();
            M5Lcd.fillScreen(TFT_BLACK);
            M5Lcd.setTextFont(4);
        }

        if (btnSide_hasPressed(true))
        {
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

        #ifdef USE_PWR_BTN_AS_EXIT
        if (btnPwr_hasPressed(true))
        {
            pwr_tick();
            stop_flag = true;
            stop_request = true;
            break;
        }
        #endif

        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
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
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.print(msg);
        M5Lcd.println();
        gui_setCursorNextLine();
        gui_showVal(0, TXTFMT_TIMEMS, (Print*)&M5Lcd); // show remaining time of zero
        gui_blankRestOfLine();
    }

    return stop_flag;
}
