#include "AlphaFairy.h"
#include <M5DisplayExt.h>

void guimenu_init()
{
  int i;
  for (i = 0; ; i++) {
    if (menu_items[i].id == MENUITEM_END_OF_TABLE) {
      menuitem_cnt = i;
      break;
    }
  }
  menuitem_idx = 0;
}

void guimenu_drawPages()
{
    // this function draws a line of dots to indicate the total menu pages and which one we are currently on
    int lcd_width  = M5Lcd.width();
    int lcd_height = M5Lcd.height();
    int dot_size = PAGINATION_DOT_SIZE;
    int space_size = PAGINATION_SPACE_SIZE;
    int pagination_width = ((dot_size + space_size) * menuitem_cnt) - space_size;
    int x_start = (lcd_width - pagination_width) / 2;
    int y_start = lcd_height - space_size - dot_size - PAGINATION_BOTTOM_MARGIN;
    int i;
    for (i = 0; i < menuitem_cnt; i++)
    {
        int x = x_start + (i * (dot_size + space_size));
        uint32_t boarder_color = TFT_WHITE;
        uint32_t fill_color    = TFT_BLACK;
        if (i == menuitem_idx) {
            // current selected dot is a bigger black dot, otherwise the white default won't show up
            boarder_color = TFT_BLACK;
        }

        // draw a grey dot beside the current black dot, depending on if the user tilted the device angle
        else if (i == (menuitem_idx - 1) && imu_angle == ANGLE_IS_DOWN) {
            fill_color = TFT_LIGHTGREY;
        }
        else if (i == (menuitem_idx + 1) && imu_angle != ANGLE_IS_DOWN) {
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

void guimenu_drawScreen()
{
    menuitem_t* menuitm = (menuitem_t*)&(menu_items[menuitem_idx]);
    M5Lcd.setRotation(0);
    M5Lcd.drawJpgFile(SPIFFS, menuitm->fname, 0, 0);
    guimenu_drawPages();
}

int guimenu_getIdx(int x)
{
    // this is used to search through the whole menu to find the index of a specific menu item
    int i;
    for (i = 0; i < menuitem_cnt; i++) {
        menuitem_t* menuitm = (menuitem_t*)&(menu_items[menuitem_idx]);
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
    // TODO: set font?
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(true);
    M5Lcd.setHighlightColor(TFT_BLACK); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
}

void gui_drawConnecting(bool first)
{
    // this function will blink between a sequence of images that indicates that we are waiting for the camera to connect
    // this code can be tweaked for more animation frames if needed
    // right now it just goes between 0 and 1
    static char conn_filename[] = "/connecting0.jpg";
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
        M5Lcd.drawJpgFile(SPIFFS, conn_filename, 0, 0);
    }
}

void gui_setCursorNextLine()
{
    // the new-line sequence only shifts Y and puts X back to zero
    // so this wrapper call restores X but uses the new Y
    M5Lcd.setCursor(SUBMENU_X_OFFSET, M5Lcd.getCursorY());
}

void welcome()
{
    // show a splash screen first
    M5Lcd.drawJpgFile(SPIFFS, "/welcome.jpg", 0, 0);
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
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }
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
                break;
            }
        }
    }
    app_waitAllRelease(BTN_DEBOUNCE);
}
