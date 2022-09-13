#include "FairyMenu.h"
#include "AlphaFairy.h"
#include <M5DisplayExt.h>

extern M5DisplayExt M5Lcd;

extern void gui_startAppPrint(void);
extern void gui_drawStatusBar(bool);
extern void gui_showVal(int32_t x, uint16_t txtfmt, Print* printer);

int8_t FairyCfgApp::prev_tilt = 0;

FairyMenuItem::FairyMenuItem(const char* img_fname, uint16_t id)
{
    _id = id;
    if (img_fname != NULL) {
        _main_img = (char*)malloc(strlen(img_fname) + 2);
        strcpy(_main_img, img_fname);
    }
    else {
        _main_img = NULL;
    }
}

void FairyMenuItem::draw_mainImage(void)
{
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, _main_img, _main_img_x, _main_img_y);
    // if you need to overlay something else on top of the main image, then override this function
}

void FairyMenuItem::draw_statusBar(void)
{
    gui_drawStatusBar(false); // if the background is black, then override this virtual function
}

FairySubmenu::FairySubmenu(const char* img_fname, uint16_t id) : FairyMenuItem(img_fname, id)
{
}

bool FairySubmenu::on_execute(void)
{
    FairyMenuItem* itm = (FairyMenuItem*)cur_node->item;
    itm->draw_mainImage();
    app_waitAllRelease();
    while (true)
    {
        if (app_poll())
        {
            if (task())
            {
                itm->on_navOut();
                return true;
            }
            pwr_sleepCheck();
        }
    }
    return false;
}

void FairySubmenu::install(FairyItem* itm)
{
    itm->set_parent((void*)this);

    if (head_node == NULL)
    {
        head_node = (FairyItemNode_t*)malloc(sizeof(FairyItemNode_t));
        head_node->item = itm;
        head_node->next_node = (void*)head_node;
        head_node->prev_node = (void*)head_node;
        cur_node = head_node;
    }
    else
    {
        FairyItemNode_t* tail_node = get_tailNode();
        FairyItemNode_t* new_node = (FairyItemNode_t*)malloc(sizeof(FairyItemNode_t));
        new_node->item = itm;
        new_node->next_node  = (void*)head_node;
        new_node->prev_node  = (void*)tail_node;
        tail_node->next_node = (void*)new_node;
        head_node->prev_node = (void*)new_node;
    }
}

FairyItem* FairySubmenu::nav_next(void)
{
    FairyItemNode_t* n = cur_node;
    uint8_t i;
    if (cur_node == NULL) {
        return NULL;
    }
    for (i = 0; i < 10; i++) // this is a for loop just to prevent infinite loops
    {
        n = (FairyItemNode_t*)(n->next_node);
        if (n->item->can_navTo())
        {
            cur_node = n;
            return cur_node->item;
        }
    }
    return NULL;
}

bool FairySubmenu::task(void)
{
    FairyMenuItem* itm;
    bool redraw = redraw_flag;

    itm = (FairyMenuItem*)cur_node->item;

    if (btnSide_hasPressed())
    {
        btnSide_clrPressed();
        Serial.println("nav");
        itm->on_navOut();
        itm = (FairyMenuItem*)nav_next();
        itm->on_navTo();
    }

    itm = (FairyMenuItem*)cur_node->item;

    if (redraw)
    {
        itm->on_redraw();
        redraw_flag = false;
    }

    itm->on_eachFrame();
    itm->draw_statusBar(); // the status bar function has its own frame rate control

    if (imu.getSpin() != 0)
    {
        itm->on_spin(imu.getSpin());
        imu.resetSpin();
    }

    if (btnBig_hasPressed())
    {
        btnBig_clrPressed();
        if (itm->on_execute())
        {
            return true;
        }
    }

    if (btnPwr_hasPressed())
    {
        btnPwr_clrPressed();
        return true;
    }

    return false;
}

FairyCfgItem::FairyCfgItem(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint16_t fmt_flags)
{
    set_name(disp_name);
    _linked_ptr = linked_var;
    _fmt_flags = fmt_flags;
    _val_min = val_min;
    _val_max = val_max;
    _step_size = step_size;
    if (_fmt_flags == TXTFMT_AUTOCFG)
    {
        _fmt_flags = 0;
        if (_val_min == 0 && _val_max == 1 && _step_size == 1) {
            _fmt_flags |= TXTFMT_BOOL;
        }
        else if (_val_min <= 1 && _val_max >= 1000 && _step_size == 1) {
            _fmt_flags |= TXTFMT_BYTENS;
        }
    }
}

FairyCfgItem::FairyCfgItem(const char* disp_name, bool (*cb)(void), const char* icon)
{
    _cb = cb;
    if (icon != NULL) {
        set_icon(icon);
    }
    set_name(disp_name);
}

void FairyCfgItem::set_name(const char* x)
{
    _disp_name = (char*)malloc(strlen(x) + 2);
    strcpy(_disp_name, x);
}

void FairyCfgItem::set_icon(const char* x)
{
    _icon_fpath = (char*)malloc(strlen(x) + 2);
    strcpy(_icon_fpath, x);
    _icon_width = 60;
}

void FairyCfgItem::set_font(int fn)
{
    if (fn < 0) // auto set
    {
        uint16_t dim1 = M5Lcd.width();
        uint16_t dim2 = M5Lcd.height();
        uint16_t dim = dim1 > dim2 ? dim1 : dim2;
        uint16_t w = M5Lcd.textWidth((const char*)_disp_name, _font_num = 4);
        if (w >= dim - _icon_width) {
            _font_num = 2;
        }
    }
    else
    {
        _font_num = fn;
    }
    _line0_height = M5Lcd.fontHeight(_font_num);
}

void FairyCfgItem::draw_name(void)
{
    int y = get_y(0);
    M5Lcd.setCursor(_margin_x, y);
    M5Lcd.setTextFont(_font_num);
    M5Lcd.print(_disp_name);
    M5Lcd.fillRect(M5Lcd.getCursorX(), y, M5Lcd.width() - M5Lcd.getCursorX() - _icon_width, M5Lcd.fontHeight(), TFT_BLACK);
    draw_icon();
}

void FairyCfgItem::draw_icon(void)
{
    if (_icon_fpath == NULL || _icon_width == 0) {
        return;
    }
    #ifdef USE_SPRITE_MANAGER
    sprites->draw(
    #else
    M5Lcd.drawPngFile(SPIFFS,
    #endif
        _icon_fpath, M5Lcd.width() - _icon_width, 0
    #ifdef USE_SPRITE_MANAGER
        , _icon_width, _icon_width
    #endif
        );
}

void FairyCfgItem::draw_statusBar(void)
{
    gui_drawStatusBar(true);
}

void FairyCfgItem::blank_text(void)
{
    M5Lcd.fillRect(0, _margin_y, M5Lcd.width() - _icon_width, 60, TFT_BLACK);
    M5Lcd.fillRect(0, 60, M5Lcd.width() - 60, 55, TFT_BLACK);
}

void FairyCfgItem::on_navTo(void)
{
    on_redraw();
}

void FairyCfgItem::on_redraw(void)
{
    blank_text();
    draw_name();
    draw_value(0);
}

void FairyCfgItem::draw_value(int8_t tilt)
{
    if (_linked_ptr == NULL) {
        return;
    }
    int y = get_y(1);
    M5Lcd.setCursor(_margin_x, y);
    M5Lcd.setTextFont(2);
    gui_showVal(get_val(), _fmt_flags, (Print*)&M5Lcd);
    if (tilt != 0) {
        M5Lcd.print((tilt > 0) ? " +> " : " <- "); // indicate if button press will increment or decrement
    }
    blank_line();
}

void FairyCfgItem::draw_value(void)
{
    draw_value(imu.getTilt());
}

void FairyCfgItem::blank_line(void)
{
    M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX() - _icon_width, M5Lcd.fontHeight(), TFT_BLACK);
}

void FairyCfgItem::on_tiltChange(void)
{
    draw_value();
}

void FairyCfgItem::on_checkAdjust(int8_t tilt)
{
    int32_t next_step = 0;
    if (btnBig_hasPressed())
    {
        if (tilt > 0)
        {
            (*_linked_ptr) += _step_size;
            if ((*_linked_ptr) >= _val_max) { // limit the range
                (*_linked_ptr) = _val_max;
            }
            else {
                next_step = _step_size; // indicate that change has been made
            }
        }
        else if (tilt < 0)
        {
            (*_linked_ptr) -= _step_size;
            if ((*_linked_ptr) <= _val_min) { // limit the range
                (*_linked_ptr) = _val_min;
            }
            else {
                next_step = -_step_size; // indicate that change has been made
            }
        }
        else
        {
            // flip boolean variable even if there's no tilt
            if ((_fmt_flags & TXTFMT_BOOL) != 0) {
                (*_linked_ptr) = ((*_linked_ptr) == 0) ? 1 : 0;
            }
        }
        draw_value(tilt);
        on_drawLive();
        on_readjust();
        btnBig_clrPressed();
    }

    if (next_step != 0 && (_fmt_flags & TXTFMT_BOOL) == 0) // has pressed
    {
        uint32_t press_time = millis();
        uint32_t dly = 500; // press-and-hold repeating delay
        int step_cnt = 0; // used to make sure at least some steps are done at minimum step size
        int tens = 10 * next_step * ((next_step < 0) ? (-1) : (1)); // if the step size starts at 1 or 10, these cases are handled
        while (btnBig_isPressed()) // is press-and-hold
        {
            app_poll();
            on_extraPoll();
            on_drawLive();

            uint32_t now = millis();
            if ((now - press_time) >= dly)
            {
                press_time = now;
                // make the required delay shorter for the next iteration
                // this makes the changes "accelerate"
                dly *= 3;
                dly /= 4;
                // impose a limit on the delay
                if ((_fmt_flags & TXTFMT_BYTENS) != 0) {
                    dly = (dly < 100) ? 100 : dly;
                }
                step_cnt++;
                (*_linked_ptr) += next_step;
                if ((*_linked_ptr) >= _val_max) { // limit the range
                    (*_linked_ptr) = _val_max;
                    break;
                }
                else if ((*_linked_ptr) <= _val_min) { // limit the range
                    (*_linked_ptr) = _val_min;
                    break;
                }
                if ((*_linked_ptr) >= 10 && ((*_linked_ptr) % tens) == 0 && step_cnt > 5 && (_fmt_flags & TXTFMT_BYTENS) != 0) {
                    step_cnt = 0;
                    tens *= 10;
                    next_step *= 10;
                }
                draw_value(tilt);
                on_drawLive();
                on_readjust();
            }
        }
        draw_value(tilt);
        on_drawLive();
        on_readjust();
    }
}

bool FairyCfgItem::on_execute(void)
{
    if (_cb == NULL) {
        return false;
    }
    return _cb();
}

int16_t FairyCfgItem::get_y(int8_t linenum)
{
    if (linenum == 0)
    {
        return _margin_y;
    }
    return _margin_y + _line0_height + (linenum * (M5Lcd.fontHeight(2) + _line_space));
}

FairyCfgApp::FairyCfgApp(const char* img_fname, const char* icon_fname, uint16_t id) : FairySubmenu(img_fname, id)
{
    if (icon_fname != NULL) {
        _icon_fname = (char*)malloc(strlen(icon_fname) + 2);
        strcpy(_icon_fname, icon_fname);
        _icon_width = 60;
    }
}

void FairyCfgApp::draw_icon(void)
{
    if (_icon_fname == NULL || _icon_width == 0) {
        return;
    }
    #ifdef USE_SPRITE_MANAGER
    sprites->draw(
    #else
    M5Lcd.drawPngFile(SPIFFS,
    #endif
        _icon_fname, M5Lcd.width() - _icon_width, M5Lcd.height() - _icon_width
    #ifdef USE_SPRITE_MANAGER
        , _icon_width, _icon_width
    #endif
        );
}

bool FairyCfgApp::task(void)
{
    FairyCfgItem* itm;
    bool redraw = redraw_flag;

    itm = (FairyCfgItem*)cur_node->item;

    if (btnSide_hasPressed())
    {
        btnSide_clrPressed();
        itm->on_navOut();
        itm = (FairyCfgItem*)nav_next();
        itm->on_navTo();
    }

    itm = (FairyCfgItem*)cur_node->item;
    itm->on_eachFrame();

    itm->draw_statusBar(); // the status bar function has its own frame rate control

    if (redraw)
    {
        itm->on_redraw();
        redraw_flag = false;
    }

    if (itm->is_func())
    {
        if (btnBig_hasPressed())
        {
            btnBig_clrPressed();
            bool ret = itm->on_execute();
            if (ret) {
                return ret;
            }
        }
    }
    else if (itm->is_value())
    {
        int8_t tilt = imu.getTilt();
        if (tilt != prev_tilt)
        {
            itm->on_tiltChange();
        }
        itm->on_checkAdjust(tilt);
        prev_tilt = tilt;
    }
    else
    {
        if (btnBig_hasPressed()) {
            btnBig_clrPressed();
        }
    }

    if (btnPwr_hasPressed())
    {
        btnPwr_clrPressed();
        return true;
    }

    return false;
}

bool FairyCfgApp::on_execute(void)
{
    FairyCfgItem* itm = (FairyCfgItem*)cur_node->item;
    gui_startAppPrint();
    itm->on_redraw();
    app_waitAllRelease();
    while (true)
    {
        if (app_poll())
        {
            if (task())
            {
                itm->on_navOut();
                M5Lcd.setRotation(0);
                return true;
            }
            pwr_sleepCheck();
        }
    }
    return false;
}
