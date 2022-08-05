#include "AlphaFairy.h"
#include <M5DisplayExt.h>

void guimenu_init(int8_t id, menustate_t* m, menuitem_t* itms)
{
  int i;
  m->id = id;
  m->items = itms;
  for (i = 0; ; i++) {
    if (itms[i].id == MENUITEM_END_OF_TABLE) {
      m->cnt = i;
      break;
    }
  }
  m->idx = 0;
  m->last_idx = -1;
}

void guimenu_drawPages()
{
    if (curmenu == NULL) {
        return;
    }

    // this function draws a line of dots to indicate the total menu pages and which one we are currently on
    int lcd_width  = M5Lcd.width();
    int lcd_height = M5Lcd.height();
    int dot_size = PAGINATION_DOT_SIZE;
    int space_size = PAGINATION_SPACE_SIZE;
    int pagination_width = ((dot_size + space_size) * curmenu->cnt) - space_size;
    int x_start = (lcd_width - pagination_width) / 2;
    int y_start = lcd_height - space_size - dot_size - PAGINATION_BOTTOM_MARGIN;
    int i;
    for (i = 0; i < curmenu->cnt; i++)
    {
        int x = x_start + (i * (dot_size + space_size));
        uint32_t boarder_color = TFT_WHITE;
        uint32_t fill_color    = TFT_BLACK;
        if (i == curmenu->idx) {
            // current selected dot is a bigger black dot, otherwise the white default won't show up
            boarder_color = TFT_BLACK;
        }

        // draw a grey dot beside the current black dot, depending on if the user tilted the device angle
        else if (i == (curmenu->idx - 1) && imu.getTilt() == TILT_IS_DOWN) {
            fill_color = TFT_LIGHTGREY;
        }
        else if (i == (curmenu->idx + 1) && imu.getTilt() != TILT_IS_DOWN) {
            fill_color = TFT_LIGHTGREY;
        }

        // draw a box outside the dot
        M5Lcd.drawFastVLine(x - 1       , y_start - 1       , dot_size + 2, boarder_color);
        M5Lcd.drawFastVLine(x + dot_size, y_start - 1       , dot_size + 2, boarder_color);
        M5Lcd.drawFastHLine(x - 1       , y_start - 1       , dot_size + 2, boarder_color);
        M5Lcd.drawFastHLine(x - 1       , y_start + dot_size, dot_size + 2, boarder_color);
        M5Lcd.drawRect(x, y_start, dot_size, dot_size, fill_color);
    }
}

void gui_drawVerticalDots(int x_offset, int y_margin, int y_offset, int dot_radius, int dot_cnt, int dot_idx, bool reverse, uint16_t back_color, uint16_t fore_color)
{
    // draws a line of vertical dots, with one dot that's highlighted
    // the calling function can move the highlighted dot to indicate progress/count-down/busy-status
    // defaults to being in the middle of the screen but the offsets can be defined
    int lcd_width  = M5Lcd.height();
    int lcd_height = M5Lcd.width();
    int x = (lcd_width / 2) + x_offset;
    int y_span = lcd_height - (2 * y_margin);
    int i;

    if (reverse) {
        dot_idx %= dot_cnt;
        dot_idx = dot_cnt - dot_idx - 1;
    }

    for (i = 0; i < dot_cnt; i++)
    {
        int y = y_margin + y_offset + ((y_span * i) / (dot_cnt - 1));
        uint32_t color = (i == (dot_idx % dot_cnt)) ? fore_color : back_color;
        M5Lcd.fillCircle(x, y, dot_radius, color);
    }
}

void gui_drawMovieRecStatus()
{
    uint16_t colour = TFT_WHITE;
    if (camera.isOperating() && camera.is_movierecording()) {
        colour = TFT_RED;
    }
    uint32_t x = 11, r = 5, y = 47;
    M5Lcd.fillCircle(x, y, r, colour);
}

void guimenu_drawScreen(menuitem_t* menu)
{
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, menu->fname, 0, 0);
}

int guimenu_getIdx(int x)
{
    // this is used to search through the whole menu to find the index of a specific menu item
    int i;
    for (i = 0; i < curmenu->cnt; i++) {
        menuitem_t* menuitm = (menuitem_t*)&(curmenu->items[curmenu->idx]);
        if (menuitm->id == x) {
            return i;
        }
    }
    return -1;
}

void gui_startAppPrint()
{
    // setup for printing text for a specific application
    // all black screen, rotated landscape, white text
    M5Lcd.fillScreen(TFT_BLACK);
    M5Lcd.setRotation(1);
    M5Lcd.setTextFont(2);
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(true);
    M5Lcd.setHighlightColor(TFT_BLACK); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
    M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
}

void gui_startMenuPrint()
{
    // setup for printing text on a menu screen
    // white text, large font
    M5Lcd.setTextFont(4);
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(false);
    M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    M5Lcd.setHighlightColor(TFT_WHITE);
}

void gui_drawConnecting(bool first)
{
    // this function will blink between a sequence of images that indicates that we are waiting for the camera to connect
    // this code can be tweaked for more animation frames if needed
    // right now it just goes between 0 and 1
    static char conn_filename[] = "/connecting0.png";
    static int last_idx = -1;
    if (first) {
        last_idx = -1;
    }
    int cur_idx;
    uint32_t now = millis();
    now %= 1400;
    cur_idx = now / 700;
    if (cur_idx != last_idx) {
        last_idx = cur_idx;
        conn_filename[11] = '0' + cur_idx;
        M5Lcd.setRotation(0);
        M5Lcd.drawPngFile(SPIFFS, conn_filename, 0, 0);
    }
}

void gui_setCursorNextLine()
{
    // the new-line sequence only shifts Y and puts X back to zero
    // so this wrapper call restores X but uses the new Y
    M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
}

void gui_blankRestOfLine()
{
    uint32_t margin = (M5Lcd.getRotation() == 0) ? 8 : 65;
    uint32_t lim = M5Lcd.width() - margin;
    while (M5Lcd.getCursorX() < lim) {
        M5Lcd.print(" ");
    }
}

void gui_showValOnLcd(int32_t val, uint16_t txtfmt, int lcdx, int lcdy, int8_t dir, bool blank_rest)
{
    M5Lcd.setCursor(lcdx, lcdy); // important to keep the coordinate for quick overwriting
    gui_showVal(val, txtfmt, (Print*)&M5Lcd); // show the value
    if (dir != 0) {
        M5Lcd.print((dir > 0) ? " + " : " - "); // indicate if button press will increment or decrement
    }
    else {
        M5Lcd.print("   "); // blanks the indicator
    }
    if (blank_rest) {
        gui_blankRestOfLine();
    }
}

extern bool gui_microphoneActive;

void gui_valIncDec(configitem_t* cfgitm)
{
    int32_t* val_ptr = cfgitm->ptr_val;
    uint8_t txtfmt = cfgitm->flags;
    int32_t next_step = 0;
    uint32_t press_time = 0;
    int lcdx, lcdy;

    if (txtfmt == 0xFF) // use auto config
    {
        txtfmt = 0;
        if (cfgitm->val_min == 0 && cfgitm->val_max == 1 && cfgitm->step_size == 1) {
            txtfmt |= TXTFMT_BOOL;
        }
        else if (cfgitm->val_min <= 1 && cfgitm->val_max >= 1000 && cfgitm->step_size == 1) {
            txtfmt |= TXTFMT_BYTENS;
        }
    }

    lcdx = M5Lcd.getCursorX(); lcdy = M5Lcd.getCursorY(); // remember start of line position for quick redraw of values
    if (imu.getTilt() == TILT_IS_UP)
    {
        if (btnBig_hasPressed()) // change the value on button press
        {
            (*val_ptr) += cfgitm->step_size;
            if ((*val_ptr) >= cfgitm->val_max) { // limit the range
                (*val_ptr) = cfgitm->val_max;
            }
            else {
                next_step = cfgitm->step_size; // indicate that change has been made
            }
            btnBig_clrPressed();
        }
        else {
            gui_showValOnLcd((*val_ptr), txtfmt, lcdx, lcdy, cfgitm->step_size, true);
            if (gui_microphoneActive) { mictrig_drawLevel(); }
        }
    }
    else if (imu.getTilt() == TILT_IS_DOWN)
    {
        if (btnBig_hasPressed()) // change the value on button press
        {
            (*val_ptr) -= cfgitm->step_size;
            if ((*val_ptr) <= cfgitm->val_min) { // limit the range
                (*val_ptr) = cfgitm->val_min;
            }
            else {
                next_step = -cfgitm->step_size; // indicate that change has been made
            }
            btnBig_clrPressed();
        }
        else {
            gui_showValOnLcd((*val_ptr), txtfmt, lcdx, lcdy, -cfgitm->step_size, true);
            if (gui_microphoneActive) { mictrig_drawLevel(); }
        }
    }
    else {
        gui_showValOnLcd((*val_ptr), txtfmt, lcdx, lcdy, 0, true);
        if (gui_microphoneActive) { mictrig_drawLevel(); }
    }

    if (next_step != 0 && (txtfmt & TXTFMT_BOOL) == 0) // has pressed
    {
        press_time = millis();
        gui_showValOnLcd((*val_ptr), txtfmt, lcdx, lcdy, next_step, true);
        if (gui_microphoneActive) { mictrig_drawLevel(); }

        uint32_t dly = 500; // press-and-hold repeating delay
        int step_cnt = 0; // used to make sure at least some steps are done at minimum step size
        int tens = 10 * next_step * ((next_step < 0) ? (-1) : (1)); // if the step size starts at 1 or 10, these cases are handled
        while (btnBig_isPressed()) // is press-and-hold
        {
            app_poll();

            if (gui_microphoneActive) {
                mictrig_poll();
            }

            uint32_t now = millis();
            if ((now - press_time) >= dly)
            {
                press_time = now;
                // make the required delay shorter for the next iteration
                // this makes the changes "accelerate"
                dly *= 3;
                dly /= 4;
                // impose a limit on the delay
                if ((txtfmt & TXTFMT_BYTENS) != 0) {
                    dly = (dly < 100) ? 100 : dly;
                }
                step_cnt++;
                (*val_ptr) += next_step;
                if ((*val_ptr) >= cfgitm->val_max) { // limit the range
                    (*val_ptr) = cfgitm->val_max;
                    break;
                }
                else if ((*val_ptr) <= cfgitm->val_min) { // limit the range
                    (*val_ptr) = cfgitm->val_min;
                    break;
                }
                if ((*val_ptr) >= 10 && ((*val_ptr) % tens) == 0 && step_cnt > 5 && (txtfmt & TXTFMT_BYTENS) != 0) {
                    step_cnt = 0;
                    tens *= 10;
                    next_step *= 10;
                }

                gui_showValOnLcd((*val_ptr), txtfmt, lcdx, lcdy, next_step, true);
                if (gui_microphoneActive) { mictrig_drawLevel(); }
            }
        }
        gui_showValOnLcd((*val_ptr), txtfmt, lcdx, lcdy, next_step, true);
        if (gui_microphoneActive) { mictrig_drawLevel(); }
    }
}

int8_t gui_drawFocusPullState()
{
    int ang = lroundf(imu.getPitch());
    int aang = (ang < 0) ? (-ang) : (ang);
    int8_t n = 0;
    char s = (ang < 0) ? 'n' : 'p';
    static const char* prefix = "/fpull_";
    static const char* suffix = ".png";
    char fname[32];
    int x = 0, y = 108;
    if (aang >= 7) {
        n = aang / 15;
        n = n > 3 ? 3 : n;
    }
    sprintf(fname, "%s%c%d%s", prefix, s, n, suffix);
    #if defined(USE_SPRITE_MANAGER) && false
    // NOTE: using the quicker sprite drawing seems to make the IMU calculation too sensitive and jittery
    // this code is disabled so the animation isn't jittery
    if ((sprites->holder_flag & SPRITESHOLDER_INTERVAL) == 0) {
        sprites->draw(fname, x, y, 135, 24);
        sprites->holder_flag |= SPRITESHOLDER_FOCUSPULL;
    }
    else {
        M5Lcd.drawPngFile(SPIFFS, fname, x, y);
    }
    #else
    M5Lcd.drawPngFile(SPIFFS, fname, x, y);
    #endif
    return (ang < 0) ? (-n) : (n);
}

void gui_drawTopThickLine(uint16_t thickness, uint16_t colour)
{
    M5Lcd.fillRect(0, 0, M5Lcd.width(), thickness, colour);
}

void gui_drawBackIcon()
{
    #ifdef USE_SPRITE_MANAGER
    sprites->draw(
    #else
    M5Lcd.drawPngFile(SPIFFS,
    #endif
        "/back_icon.png", M5Lcd.width() - 60, 0
    #ifdef USE_SPRITE_MANAGER
        , 60, 60
    #endif
        );
}

void gui_drawGoIcon()
{
    #ifdef USE_SPRITE_MANAGER
    sprites->draw(
    #else
    M5Lcd.drawPngFile(SPIFFS,
    #endif
        "/go_icon.png", M5Lcd.width() - 60, 0
    #ifdef USE_SPRITE_MANAGER
        , 60, 60
    #endif
        );
}

void welcome()
{
#ifdef DISABLE_WELCOME
    return;
#else
    // show a splash screen first
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, "/welcome.png", 0, 0);
    uint32_t now;
    bool btn_quit = false;
    while ((now = millis()) < WELCOME_TIME_MS)
    {
        app_poll();
        if (btnAll_hasPressed())
        {
            // exit on any button press
            btn_quit = true;
            btnAll_clrPressed();
            break;
        }
    }
    if (btn_quit)
    {
        dbg_ser.printf("welcome exit via button\r\n");
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }
    dbg_ser.printf("welcome ended due to timeout\r\n");

    dbg_ser.printf("conn-wait-screen... ");

    // show camera is still connecting
    if (camera.isOperating() == false)
    {
        while ((now = millis()) < WELCOME_CONN_TIME_MS)
        {
            if (app_poll())
            {
                gui_drawConnecting(false);
            }
            if (btnAll_hasPressed() || camera.isOperating())
            {
                // exit on any button press, or successful connection
                dbg_ser.printf("event! ");
                btnAll_clrPressed();
                break;
            }
        }
    }
    dbg_ser.printf(" done.\r\n");
    app_waitAllRelease(BTN_DEBOUNCE);
#endif
}
