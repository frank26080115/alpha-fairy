#include "AlphaFairy.h"
#include <M5StickC.h>
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
            boarder_color = TFT_BLACK;
        }
        else if (i == (menuitem_idx - 1) && imu_angle == ANGLE_IS_DOWN) {
            fill_color = TFT_LIGHTGREY;
        }
        else if (i == (menuitem_idx + 1) && imu_angle != ANGLE_IS_DOWN) {
            fill_color = TFT_LIGHTGREY;
        }
        M5Lcd.drawFastVLine(x - 1       , y_start - 1       , dot_size + 2, boarder_color);
        M5Lcd.drawFastVLine(x + dot_size, y_start - 1       , dot_size + 2, boarder_color);
        M5Lcd.drawFastHLine(x - 1       , y_start - 1       , dot_size + 2, boarder_color);
        M5Lcd.drawFastHLine(x - 1       , y_start + dot_size, dot_size + 2, boarder_color);
        M5Lcd.drawRect(x, y_start, dot_size, dot_size, fill_color);
    }
}

void gui_drawVerticalDots(int x_offset, int y_margin, int y_offset, int dot_radius, int dot_cnt, int dot_idx, bool reverse, uint16_t back_color, uint16_t fore_color)
{
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
    M5Lcd.setHighlightColor(TFT_BLACK);
}

void gui_drawConnecting(bool first)
{
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

void welcome()
{
    M5Lcd.drawJpgFile(SPIFFS, "/welcome.jpg", 0, 0);
    uint32_t now;
    bool btn_quit = false;
    while ((now = millis()) < WELCOME_TIME_MS)
    {
        app_poll();
        if (btnBig_hasPressed(true) || btnSide_hasPressed(true))
        {
            btn_quit = true;
            break;
        }
    }
    if (btn_quit)
    {
        app_waitAllRelease(BTN_DEBOUNCE);
        return;
    }
    while ((now = millis()) < WELCOME_CONN_TIME_MS)
    {
        if (app_poll())
        {
            gui_drawConnecting(false);
        }
        if (btnBig_hasPressed(true) || btnSide_hasPressed(true) || camera.isOperating())
        {
            break;
        }
    }
    app_waitAllRelease(BTN_DEBOUNCE);
}
