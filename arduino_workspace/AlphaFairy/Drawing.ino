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
        else if (i == (curmenu->idx - 1) && imu_angle == ANGLE_IS_DOWN) {
            fill_color = TFT_LIGHTGREY;
        }
        else if (i == (curmenu->idx + 1) && imu_angle != ANGLE_IS_DOWN) {
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

void gui_startPrint()
{
    M5Lcd.fillScreen(TFT_BLACK);
    M5Lcd.setRotation(1);
    M5Lcd.setTextFont(2);
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(true);
    M5Lcd.setHighlightColor(TFT_BLACK); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
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
    uint32_t lim = M5Lcd.width() - 65;
    while (M5Lcd.getCursorX() < lim) {
        M5Lcd.print(" ");
    }
}

void gui_formatSecondsTime(int32_t x, char* str, bool shorten)
{
    // format time in 00:00:00 format when provided in seconds
    // optionally shortens to 0H:00 format when the time is very long
    int i = 0;
    if (x < 0) {
        // negative sign
        i += sprintf(&(str[i]), "-");
        x *= -1;
    }
    uint32_t mins = x / 60;
    uint32_t hrs = mins / 60;
    mins %= 60;
    uint32_t secs = x % 60;
    if (hrs > 0) {
        i += sprintf(&(str[i]), "%u", hrs);
        if (shorten) {
            // add a H just so we understand it's HH:MM instead of MM:SS
            i += sprintf(&(str[i]), "H");
        }
        i += sprintf(&(str[i]), ":");
        if (mins < 10) {
            i += sprintf(&(str[i]), "0");
        }
    }
    if (shorten && hrs > 0) {
        i += sprintf(&(str[i]), "%u", mins);
    }
    else {
        i += sprintf(&(str[i]), "%u:%02u", mins, secs);
    }
}

void gui_showVal(int32_t x, uint8_t cfgfmt, Print* printer)
{
    char str[16]; int i = 0;
    if ((cfgfmt & CFGFMT_BOOL) != 0) {
        if (x == 0) {
            i += sprintf(&(str[i]), "NO");
        }
        else {
            i += sprintf(&(str[i]), "YES");
        }
    }
    else if ((cfgfmt & CFGFMT_BULB) != 0) {
        if (x == 0) {
            // when bulb = 0, the shutter speed setting on the camera is used
            i += sprintf(&(str[i]), "(Tv)");
        }
        else {
            gui_formatSecondsTime(x, str, false);
        }
    }
    else if ((cfgfmt & CFGFMT_TIMELONG) != 0) {
        gui_formatSecondsTime(x, str, true);
    }
    else if ((cfgfmt & CFGFMT_TIME) != 0) {
        gui_formatSecondsTime(x, str, false);
    }
    else if ((cfgfmt & CFGFMT_TIMEMS) != 0) {
        // if time is provided in milliseconds
        // print the time as usual (after calculating the whole seconds)
        x = x < 0 ? 0 : x;
        uint32_t tsec = x / 1000;
        uint32_t tsubsec = (x / 100) % 10; // get one decimal place
        gui_formatSecondsTime(tsec, str, false);
        // add one decimal place
        sprintf(&(str[strlen(str)]), ".%d", tsubsec);
    }
    else {
        i += sprintf(&(str[i]), "%d", x);
    }
    if (printer != NULL) {
        printer->print(str);
    }
}

void gui_showValOnLcd(int32_t val, uint8_t cfgfmt, int lcdx, int lcdy, int8_t dir, bool blank_rest)
{
    M5Lcd.setCursor(lcdx, lcdy); // important to keep the coordinate for quick overwriting
    gui_showVal(val, cfgfmt, (Print*)&M5Lcd); // show the value
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

void gui_valIncDec(configitem_t* cfgitm)
{
    int32_t* val_ptr = cfgitm->ptr_val;
    uint8_t cfgfmt = cfgitm->flags;
    int32_t next_step = 0;
    uint32_t press_time = 0;
    int lcdx, lcdy;

    if (cfgfmt == 0xFF) // use auto config
    {
        cfgfmt = 0;
        if (cfgitm->val_min == 0 && cfgitm->val_max == 1 && cfgitm->step_size == 1) {
            cfgfmt |= CFGFMT_BOOL;
        }
        else if (cfgitm->val_min <= 1 && cfgitm->val_max >= 1000 && cfgitm->step_size == 1) {
            cfgfmt |= CFGFMT_BYTENS;
        }
    }

    lcdx = M5Lcd.getCursorX(); lcdy = M5Lcd.getCursorY(); // remember start of line position for quick redraw of values
    if (imu_angle == ANGLE_IS_UP)
    {
        if (btnBig_hasPressed(true)) // change the value on button press
        {
            (*val_ptr) += cfgitm->step_size;
            if ((*val_ptr) >= cfgitm->val_max) { // limit the range
                (*val_ptr) = cfgitm->val_max;
            }
            else {
                next_step = cfgitm->step_size; // indicate that change has been made
            }
        }
        else {
            gui_showValOnLcd((*val_ptr), cfgfmt, lcdx, lcdy, cfgitm->step_size, true);
        }
    }
    else if (imu_angle == ANGLE_IS_DOWN)
    {
        if (btnBig_hasPressed(true)) // change the value on button press
        {
            (*val_ptr) -= cfgitm->step_size;
            if ((*val_ptr) <= cfgitm->val_min) { // limit the range
                (*val_ptr) = cfgitm->val_min;
            }
            else {
                next_step = -cfgitm->step_size; // indicate that change has been made
            }
        }
        else {
            gui_showValOnLcd((*val_ptr), cfgfmt, lcdx, lcdy, -cfgitm->step_size, true);
        }
    }
    else {
        gui_showValOnLcd((*val_ptr), cfgfmt, lcdx, lcdy, 0, true);
    }

    if (next_step != 0 && (cfgfmt & CFGFMT_BOOL) == 0) // has pressed
    {
        press_time = millis();
        gui_showValOnLcd((*val_ptr), cfgfmt, lcdx, lcdy, next_step, true);
        uint32_t dly = 500; // press-and-hold repeating delay
        int step_cnt = 0; // used to make sure at least some steps are done at minimum step size
        int tens = 10 * next_step * ((next_step < 0) ? (-1) : (1)); // if the step size starts at 1 or 10, these cases are handled
        while (btnBig_isPressed()) // is press-and-hold
        {
            app_poll();
            uint32_t now = millis();
            if ((now - press_time) >= dly)
            {
                press_time = now;
                // make the required delay shorter for the next iteration
                // this makes the changes "accelerate"
                dly *= 3;
                dly /= 4;
                // impose a limit on the delay
                if ((cfgfmt & CFGFMT_BYTENS) != 0) {
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
                if ((*val_ptr) >= 10 && ((*val_ptr) % tens) == 0 && step_cnt > 5 && (cfgfmt & CFGFMT_BYTENS) != 0) {
                    step_cnt = 0;
                    tens *= 10;
                    next_step *= 10;
                }
                gui_showValOnLcd((*val_ptr), cfgfmt, lcdx, lcdy, next_step, true);
            }
        }
        gui_showValOnLcd((*val_ptr), cfgfmt, lcdx, lcdy, next_step, true);
    }

    app_waitAllRelease(BTN_DEBOUNCE);
}

void welcome()
{
    return; // welcome screen disabled

#if 0
    // show a splash screen first
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, "/welcome.png", 0, 0);
    uint32_t now;
    bool btn_quit = false;
    while ((now = millis()) < WELCOME_TIME_MS)
    {
        app_poll();
        if (btnBig_hasPressed(true) || btnSide_hasPressed(true))
        {
            // exit on any button press
            btn_quit = true;
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
            if (btnBig_hasPressed(true) || btnSide_hasPressed(true) || camera.isOperating())
            {
                // exit on any button press, or successful connection
                dbg_ser.printf("event! ");
                break;
            }
        }
    }
    dbg_ser.printf(" done.\r\n");
    app_waitAllRelease(BTN_DEBOUNCE);
#endif
}
