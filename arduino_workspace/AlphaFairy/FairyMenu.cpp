#include "FairyMenu.h"
#include "AlphaFairy.h"
#include <M5DisplayExt.h>

extern M5DisplayExt M5Lcd;

extern void gui_startAppPrint(void);
extern void gui_drawStatusBar(bool);
extern void gui_showVal(int32_t x, uint32_t txtfmt, Print* printer);

extern void tallylite_task(void);

extern void handle_user_reauth(void); // shows the wifi error screen and offers the user a way of changing wifi password

#ifdef ENABLE_BUILD_LEPTON
extern void lepton_encRead(bool* sw, int16_t* inc, int16_t* rem);
extern void lepton_encClear(void);
#endif

int8_t FairyCfgApp::prev_tilt = 0;
bool FairyCfgItem::dirty = false;

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
    cpufreq_boost();
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, _main_img, _main_img_x, _main_img_y);
    // if you need to overlay something else on top of the main image, then override this function, call it first, then do whatever you need to do
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
    cpufreq_boost();
    rewind();
    imu.resetSpin();
    FairyMenuItem* itm = (FairyMenuItem*)cur_node->item;
    itm->on_navTo();
    app_waitAllRelease();
    do
    {
        if (app_poll()) // true if low priority tasks can execute
        {
            tallylite_task();

            if (task()) // true if user wants to exit out of submenu
            {
                itm->on_navOut();
                break;
            }
            pwr_sleepCheck();
        }
    } 
    while (true);
    // user exit
    set_redraw();
    return false;
}

void FairySubmenu::install(FairyItem* itm)
{
    itm->set_parent((void*)this, this->_id);

    if (head_node == NULL) // first node of the list, need to be assigned to head_node
    {
        head_node = (FairyItemNode_t*)malloc(sizeof(FairyItemNode_t));
        head_node->item = itm;
        head_node->next_node = (void*)head_node;
        head_node->prev_node = (void*)head_node;
        cur_node = head_node;
    }
    else // not first node, need to insert between head node and tail node, to become the new tail node
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
        // find the next node that isn't hidden
        if (n->item->can_navTo())
        {
            cur_node = n;
            return cur_node->item;
        }
    }
    return NULL;
}

#ifdef ENABLE_BUILD_LEPTON
FairyItem* FairySubmenu::nav_prev(void)
{
    FairyItemNode_t* n = cur_node;
    uint8_t i;
    if (cur_node == NULL) {
        return NULL;
    }
    for (i = 0; i < 10; i++) // this is a for loop just to prevent infinite loops
    {
        n = (FairyItemNode_t*)(n->prev_node);
        // find the next node that isn't hidden
        if (n->item->can_navTo())
        {
            cur_node = n;
            return cur_node->item;
        }
    }
    return NULL;
}
#endif

bool FairySubmenu::task(void)
{
    handle_user_reauth();

    FairyMenuItem* itm;
    bool redraw = redraw_flag;

    #ifdef ENABLE_BUILD_LEPTON
    bool enc_center_btn;
    int16_t enc_nav;
    #endif

    itm = (FairyMenuItem*)cur_node->item;

    bool to_nav = false;

    if (btnSide_hasPressed())
    {
        // next button pressed
        btnSide_clrPressed();
        to_nav = true;
    }
    else if (_bigbtn_nav)
    {
        // big button becomes next button
        if (btnBig_hasPressed())
        {
            btnBig_clrPressed();
            to_nav = true;
        }
    }
    #ifdef ENABLE_BUILD_LEPTON
    if (_enc_nav)
    {
        lepton_encRead(&enc_center_btn, &enc_nav, NULL);
        if (enc_nav > 0)
        {
            to_nav = true;
        }
        else if (enc_nav < 0)
        {
            to_nav = false;
            itm->on_navOut();
            itm = (FairyMenuItem*)nav_prev();
            itm->on_navTo();
            imu.resetSpin();
        }
    }
    #endif

    if (to_nav) // next button pressed
    {
        itm->on_navOut();
        itm = (FairyMenuItem*)nav_next();
        itm->on_navTo();
        imu.resetSpin();
    }

    itm = (FairyMenuItem*)cur_node->item;

    redraw |= itm->check_redraw();

    if (redraw)
    {
        itm->on_redraw();
        redraw_flag = false;
    }

    // user has spun the IMU
    if (imu.getSpin() != 0)
    {
        itm->on_spin(imu.getSpin());
        imu.resetSpin();
    }
    #ifdef ENABLE_BUILD_LEPTON
    else if (_enc_nav == false)
    {
        lepton_encRead(&enc_center_btn, &enc_nav, NULL);
        if (enc_nav != 0)
        {
            itm->on_spin(enc_nav);
        }
    }
    #endif

    itm->on_eachFrame();
    itm->draw_statusBar(); // the status bar function has its own frame rate control

    #ifdef ENABLE_BUILD_LEPTON
    // waits until encoder stops
    while (enc_nav != 0) {
        lepton_encRead(&enc_center_btn, &enc_nav, NULL);
        app_poll();
    }

    if (enc_center_btn)
    {
        // encoder center button will act as big button
        sprites->unload_all();
        bool need_exit = itm->on_execute();
        sprites->unload_all();
        if (need_exit)
        {
            return true;
        }
    }
    else
    #endif

    if (_bigbtn_nav)
    {
        // do nothing here, big button is acting as the next button
    }
    else if (btnBig_hasPressed())
    {
        btnBig_clrPressed();
        sprites->unload_all();
        bool need_exit = itm->on_execute();
        sprites->unload_all();
        if (need_exit)
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

FairyCfgItem::FairyCfgItem(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint32_t fmt_flags)
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

FairyCfgItem::FairyCfgItem(const char* disp_name, bool (*cb)(void*), const char* icon)
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
    set_font(-1);
}

void FairyCfgItem::set_icon(const char* x)
{
    _icon_fpath = (char*)malloc(strlen(x) + 2);
    strcpy(_icon_fpath, x);
    _icon_width = GENERAL_ICON_WIDTH;
}

void FairyCfgItem::set_font(int fn)
{
    if (fn < 0) // auto set
    {
        uint16_t dim1 = M5Lcd.width();
        uint16_t dim2 = M5Lcd.height();
        uint16_t dim = dim1 > dim2 ? dim1 : dim2;
        uint16_t w = M5Lcd.textWidth((const char*)_disp_name, _font_num = 4);
        if (w >= dim - _icon_width - 10) {
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
    M5Lcd.setTextFont(4);
    M5Lcd.fillRect(M5Lcd.getCursorX(), y, M5Lcd.width() - M5Lcd.getCursorX() - _icon_width, M5Lcd.fontHeight(), TFT_BLACK);
    draw_icon();
}

void FairyCfgItem::draw_icon(void)
{
    if (_icon_fpath != NULL && _icon_width > 0)
    {
        M5Lcd.drawPngFile(SPIFFS, _icon_fpath, M5Lcd.width() - _icon_width, 0);
    }
    FairyCfgApp* p = dynamic_cast<FairyCfgApp*>((FairyCfgApp*)get_parent());
    if (p != NULL)
    {
        p->draw_icon();
    }
}

void FairyCfgItem::draw_statusBar(void)
{
    gui_drawStatusBar(true);
}

void FairyCfgItem::blank_text(void)
{
    M5Lcd.fillRect(M5Lcd.width() - GENERAL_ICON_WIDTH, 0                    , GENERAL_ICON_WIDTH                , GENERAL_ICON_WIDTH + _margin_y    , TFT_BLACK);
    M5Lcd.fillRect(0                                 , _margin_y            , M5Lcd.width() - _icon_width       , GENERAL_ICON_WIDTH                , TFT_BLACK);
    M5Lcd.fillRect(0                                 , GENERAL_ICON_WIDTH   , M5Lcd.width() - GENERAL_ICON_WIDTH, GENERAL_ICON_WIDTH - 5            , TFT_BLACK);
}

void FairyCfgItem::on_navTo(void)
{
    on_redraw();
}

void FairyCfgItem::on_redraw(void)
{
    blank_text();
    draw_name();
    draw_value(imu.getTilt());
}

void FairyCfgItem::draw_value(int8_t tilt)
{
    if (_linked_ptr == NULL) {
        return;
    }
    int y = get_y(1);
    M5Lcd.setCursor(_margin_x, y);
    M5Lcd.setTextFont(4);
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
    M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX() - 60, M5Lcd.fontHeight(), TFT_BLACK);
}

void FairyCfgItem::on_tiltChange(void)
{
    draw_value();
}

void FairyCfgItem::on_checkAdjust(int8_t tilt)
{
    // this function handles:
    //  * displaying the arrows beside the value according to tilt
    //  * changing the value on button press
    //  * changing the value even faster when the button is held down

    int32_t next_step = 0; // this will latch the direction of change during button-hold

    int16_t rx, ry;
    rx = M5Lcd.getCursorX(); ry = M5Lcd.getCursorY();

    #ifdef ENABLE_BUILD_LEPTON
    bool enc_btn = false;
    int16_t enc_inc = 0;
    int16_t enc_rem;
    int16_t enc_inc_prev = 0;
    #endif

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
            dirty = true;
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
            dirty = true;
        }
        else
        {
            // flip boolean variable even if there's no tilt
            if ((_fmt_flags & TXTFMT_BOOL) != 0) {
                (*_linked_ptr) = ((*_linked_ptr) == 0) ? 1 : 0;
                dirty = true;
            }
        }
        draw_value(tilt);
        on_drawLive();
        on_readjust();
        M5Lcd.setCursor(rx, ry);

        btnBig_clrPressed();
    }
    #ifdef ENABLE_BUILD_LEPTON
    else
    {
        lepton_encRead(&enc_btn, &enc_inc, &enc_rem);
        if (enc_inc > 0)
        {
            (*_linked_ptr) += _step_size;
            if ((*_linked_ptr) >= _val_max) { // limit the range
                (*_linked_ptr) = _val_max;
            }
            else {
                next_step = _step_size; // indicate that change has been made
            }
            dirty = true;
        }
        else if (enc_inc < 0)
        {
            (*_linked_ptr) -= _step_size;
            if ((*_linked_ptr) <= _val_min) { // limit the range
                (*_linked_ptr) = _val_min;
            }
            else {
                next_step = -_step_size; // indicate that change has been made
            }
            dirty = true;
        }
        if (enc_inc != 0)
        {
            draw_value(tilt);
            on_drawLive();
            on_readjust();
            M5Lcd.setCursor(rx, ry);
        }
        enc_inc_prev = enc_inc;
    }
    #endif

    if (next_step != 0 && (_fmt_flags & TXTFMT_BOOL) == 0) // has pressed
    {
        uint32_t press_time = millis();
        uint32_t dly = btnBig_isPressed() ? 500 : 1000; // press-and-hold repeating delay
        int step_cnt = 0; // used to make sure at least some steps are done at minimum step size
        int tens = 10 * next_step * ((next_step < 0) ? (-1) : (1)); // if the step size starts at 1 or 10, these cases are handled
        while (true) // is press-and-hold
        {
            if (btnBig_isPressed() == false)
            {
                #ifdef ENABLE_BUILD_LEPTON
                #if 0 // this chunk of code doesn't work
                lepton_encRead(&enc_btn, &enc_inc, &enc_rem);
                if (enc_inc != 0 || enc_rem != 0)
                {
                    if (enc_inc != 0 && enc_inc != enc_inc_prev && enc_rem == 0)
                    {
                        break;
                    }
                }
                else if (enc_inc == 0 && enc_rem == 0)
                {
                    break;
                }
                else
                #endif
                #endif
                break;
            }
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
                M5Lcd.setCursor(rx, ry);
            }
        }
        draw_value(tilt);
        on_drawLive();
        on_readjust();
        M5Lcd.setCursor(rx, ry);
    }

    #ifdef ENABLE_BUILD_LEPTON
    if (enc_inc_prev != 0)
    {
        lepton_encClear();
    }
    #endif
}

bool FairyCfgItem::on_execute(void)
{
    if (_cb == NULL) {
        return false;
    }
    return _cb(this);
}

int16_t FairyCfgItem::get_y(int8_t linenum)
{
    if (linenum == 0)
    {
        return _margin_y;
    }
    return _margin_y + _line0_height + ((linenum - 1) * (M5Lcd.fontHeight(4) + _line_space));
}

FairyCfgApp::FairyCfgApp(const char* img_fname, const char* icon_fname, uint16_t id) : FairySubmenu(img_fname, id)
{
    if (icon_fname != NULL) {
        _icon_fname = (char*)malloc(strlen(icon_fname) + 2);
        strcpy(_icon_fname, icon_fname);
        _icon_width = GENERAL_ICON_WIDTH;
    }
    _enc_nav = false;
}

void FairyCfgApp::draw_icon(void)
{
    if (_icon_fname == NULL || _icon_width == 0) {
        return;
    }
    M5Lcd.drawPngFile(SPIFFS, _icon_fname, M5Lcd.width() - _icon_width, M5Lcd.height() - _icon_width);
}

// this function is similar to FairySubmenu::task(void)
bool FairyCfgApp::task(void)
{
    handle_user_reauth();

    FairyCfgItem* itm;
    bool redraw = redraw_flag;

    bool enc_btn; int16_t enc_inc;

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
        cpufreq_boost();
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
        #ifdef ENABLE_BUILD_LEPTON
        else
        {
            lepton_encRead(&enc_btn, &enc_inc, NULL);
            if (enc_btn)
            {
                bool ret = itm->on_execute();
                if (ret) {
                    return ret;
                }
            }
        }
        #endif
    }
    else if (itm->is_value())
    {
        int8_t tilt = imu.getTilt();
        if (tilt != prev_tilt) {
            itm->on_tiltChange();
        }
        itm->on_checkAdjust(tilt);
        prev_tilt = tilt;
    }
    else
    {
        // this case is not supposed to ever happen, but we need to clear the big button's event flag
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

// this function is similar to FairySubmenu::on_execute(void)
bool FairyCfgApp::on_execute(void)
{
    cpufreq_boost();
    rewind();
    FairyCfgItem* itm = (FairyCfgItem*)cur_node->item;
    gui_startAppPrint();
    itm->on_navTo();
    app_waitAllRelease();
    while (true)
    {
        if (app_poll())
        {
            if (task())
            {
                itm->on_navOut();
                break;
            }
            pwr_sleepCheck();
        }
    }
    M5Lcd.setRotation(0);
    set_redraw();
    return false;
}

bool FairyMenuItem::must_be_connected(void)
{
    if (fairycam.isOperating() == false) {
        app_waitAllReleaseConnecting();
        return false;
    }
    return true;
}

bool FairyMenuItem::must_be_ptp(void)
{
    if (must_be_connected() == false) {
        return false;
    }
    if (httpcam.isOperating()) {
        app_waitAllReleaseUnsupported();
        return false;
    }
    return true;
}
