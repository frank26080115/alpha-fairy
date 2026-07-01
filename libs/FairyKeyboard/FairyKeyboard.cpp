#include "FairyKeyboard.h"

#define KBD_LINE_SPACING         24
#define KBD_LEFT_MARGIN           7
#define KBD_DIVIDER_Y            (_screen_height - 75)
#define KBD_DIVIDER_Y2           (KBD_DIVIDER_Y + KBD_LINE_SPACING + 2)
#define KBD_DIVIDER_Y3           (KBD_DIVIDER_Y2 + KBD_LINE_SPACING)
#define KBD_TYPE_Y               (KBD_DIVIDER_Y - 7)
#define KBD_BOARD_Y              (KBD_DIVIDER_Y + 20)
#define KBD_CHAR_WIDTH           (14 + 3)
#define KBD_ALLCHAR_WIDTH        (KBD_CHAR_WIDTH * 13)
#define KBD_ALLNUM_WIDTH         (KBD_CHAR_WIDTH * 14)
#define KBD_ROW_CNT              3
#define KBD_GET_COL_SPAN(xyz)    (((xyz) == 2) ? 14 : 13)
#define KBD_FONT_MAIN            FreeMono12pt7b

#define KBD_SPECIALCHAR_BACKSPACE '<'
#define KBD_SPECIALCHAR_ENTER     '='

static const char line_1_u[] = "ABCDEFGHIJKLM";
static const char line_1_l[] = "abcdefghijklm";
static const char line_2_u[] = "NOPQRSTUVWXYZ";
static const char line_2_l[] = "nopqrstuvwxyz";
static const char line_3[]   = "<<1234567890==";

FairyKeyboard::FairyKeyboard(M5Display* used_lcd, uint16_t colour_bg, uint16_t colour_fg, uint16_t colour_fade, uint32_t colour_hl, uint32_t colour_box, uint16_t roll_span, uint16_t pitch_span, uint16_t pitch_hyster, uint32_t blink_dly, bool auto_flip)
{
    _lcd           = used_lcd;
    _colour_back   = colour_bg;
    _colour_fore   = colour_fg;
    _colour_hilite = colour_hl;
    _colour_box    = colour_box;
    _colour_fade   = colour_fade;
    _roll_span     = roll_span;
    _pitch_span    = pitch_span;
    _pitch_hyster  = pitch_hyster;
    _blink_dly     = blink_dly;
    _auto_flip     = auto_flip;
    reset();
}

void FairyKeyboard::reset()
{
    _has_pitch     = false;
    _center_pitch  =  0;
    _cur_row       = -1;
    _cur_col       = -1;
    _prev_row      = -1;
    _prev_col      = -1;
    _prev_char     =  0;
    _cur_rot       =  1;
    _prev_rot      =  0;
    _start_time    = millis();
    _is_lowercase  = false;
    _prev_lowercase = false;
    _recent_click  = false;
    _str_idx       = 0;
    _str[0]        = 0;
}

void FairyKeyboard::lcd_setup()
{
    _lcd->setRotation(_cur_rot);
    _screen_width  = _lcd->width();
    _screen_height = _lcd->height();
    _lcd->setFreeFont(&KBD_FONT_MAIN);
    _lcd->setTextColor(_colour_fade, _colour_back);
}

void FairyKeyboard::draw_base()
{
    lcd_setup();
    _lcd->fillRect(0, KBD_TYPE_Y, _screen_width, _screen_height - KBD_TYPE_Y, _colour_back); // erase everything below the typing line
    _lcd->drawFastHLine(0, KBD_DIVIDER_Y, _screen_width, _colour_fade);                      // draw typing line divider

    // draw all keys
    int16_t row, col, x, y;
    for (row = 0; row < KBD_ROW_CNT; row++)
    {
        int col_span = KBD_GET_COL_SPAN(row);
        for (col = 0; col < col_span; col++)
        {
            char c;
            get_char_coords(row, col, &c, &x, &y);
            _lcd->setCursor(x, y);
            if (c != KBD_SPECIALCHAR_BACKSPACE && c != KBD_SPECIALCHAR_ENTER)
            {
                _lcd->write(c);
            }
            else if (row == 2 && c == KBD_SPECIALCHAR_BACKSPACE)
            {
                draw_backspace(false);
                _lcd->setTextColor(_colour_fade, _colour_back);
            }
            else if (row == 2 &&c == KBD_SPECIALCHAR_ENTER)
            {
                draw_enter(false);
                _lcd->setTextColor(_colour_fade, _colour_back);
            }
        }
    }
    if (redraw_cb != NULL) {
        redraw_cb();
    }
}

/*
pitch is up/down
roll is left/right

               + pitch +
            +-------------------------+
            | +---------------+  +--  |
            | |##row#0########|  |    |
     -roll- | |##row#1########|  |    | +roll+
            | |##row#2########|  |    |
            | +---------------+  +--  |
            +-------------------------+
               - pitch -

*/

void FairyKeyboard::update(float roll, float pitch)
{
    if (_auto_flip)
    {
        // extreme pitch angle causes auto-flipping
        bool flip;
        if (_cur_rot != 3) {
            flip = (pitch > (_pitch_span / 2));
        }
        else {
            flip = (pitch > -(_pitch_span / 2));
        }

        // do the flip
        if (flip) {
            roll  *= -1;
            pitch *= -1;
            _cur_rot = 3; // used inside draw_base()
        }
        else {
            _cur_rot = 1; // used inside draw_base()
        }
    }

    // clear the screen
    if (_cur_rot != _prev_rot)
    {
        _lcd->fillScreen(_colour_back);
        draw_base();
        _prev_char = 0; // trigger redraw of keys later
    }
    else if (_prev_lowercase != _is_lowercase)
    {
        draw_base();
        _prev_char = 0; // trigger redraw of keys later
    }

    lcd_setup(); // call just in case if the user redraw callback mucked up the font

    int pitchi = lroundf(pitch); // speed up processing, shrink code size

    if (_has_pitch == false) {
        // set some defaults if it's the first run
        _center_pitch = pitchi;
        _has_pitch = true;
        _cur_row = 1;
    }

    // determine row index from pitch angle
    if (pitchi > (_center_pitch + (_pitch_span / 2)))
    {
        // angle exceeded extreme limits, so move the center-point
        _center_pitch = pitchi - (_pitch_span / 2);
        _cur_row = 0;
    }
    else if (pitchi < (_center_pitch - (_pitch_span / 2)))
    {
        // angle exceeded extreme limits, so move the center-point
        _center_pitch = pitchi + (_pitch_span / 2);
        _cur_row = (KBD_ROW_CNT - 1);
    }
    else if (_cur_row == 1)
    {
        if (pitchi > (_center_pitch + (_pitch_span / (KBD_ROW_CNT * 2)) + _pitch_hyster)) {
            _cur_row = 0;
        }
        else if (pitchi < (_center_pitch - (_pitch_span / (KBD_ROW_CNT * 2)) - _pitch_hyster)) {
            _cur_row = 2;
        }
    }
    else if (_cur_row == 0)
    {
        if (pitchi < (_center_pitch - (_pitch_span / (KBD_ROW_CNT * 2)))) {
            _cur_row = 2;
        }
        if (pitchi < (_center_pitch + (_pitch_span / (KBD_ROW_CNT * 2)) - _pitch_hyster)) {
            _cur_row = 1;
        }
    }
    else if (_cur_row == (KBD_ROW_CNT - 1))
    {
        if (pitchi > (_center_pitch + (_pitch_span / (KBD_ROW_CNT * 2)))) {
            _cur_row = 2;
        }
        if (pitchi > (_center_pitch - (_pitch_span / (KBD_ROW_CNT * 2)) + _pitch_hyster)) {
            _cur_row = 1;
        }
    }

    int col_span = KBD_GET_COL_SPAN(_cur_row); // number of columns vary depending on row
    float col_width =  _roll_span / col_span;  // width of each column is dynamic

    // enforce a limit
    if (roll < -(_roll_span / 2)) {
        roll = -(_roll_span / 2);
    }
    else if (roll > (_roll_span / 2)) {
        roll = (_roll_span / 2);
    }
    // offset so it's useful for zero-indexed calculations
    roll += (_roll_span / 2);

    if (_cur_col < 0)
    {
        // first run, very easy to find location, no need for hysteresis
        for (_cur_col = 0; _cur_col < col_span; _cur_col++)
        {
            if ((col_width * (_cur_col + 1)) > roll) {
                break;
            }
        }
    }
    else
    {
        do
        {
            // determine boundaries of column, accounting for hysteresis
            float roll_hyster = col_width / 10;
            float roll_left  = ( _cur_col      * col_width) - roll_hyster;
            float roll_right = ((_cur_col + 1) * col_width) + roll_hyster;
            if (roll >= roll_left && roll <= roll_right) {
                // if within boundaries, then we have found the right column
                break;
            }
            else if (roll <= roll_left ) { // exceed boundary, move left
                _cur_col -= 1;
            }
            else if (roll >= roll_right) { // exceed boundary, move right
                _cur_col += 1;
            }
        }
        while (_cur_col > 0 && _cur_col < col_span);
    }

    // enforce limits
    _cur_col = ((_cur_col < 0) ? 0 : ((_cur_col >=    col_span) ? (col_span    - 1) : _cur_col));
    _cur_row = ((_cur_row < 0) ? 0 : ((_cur_row >= KBD_ROW_CNT) ? (KBD_ROW_CNT - 1) : _cur_row));

    int16_t x, y, px, py, bw, bh;
    char pc;
    get_char_coords(_cur_row, _cur_col, &_cur_char, &x, &y);

    // only draw if changes detected
    if (_cur_char != _prev_char || _prev_lowercase != _is_lowercase)
    {
        if (_prev_col >= 0 || _prev_row >= 0 || _prev_char > 0)
        {
            // instead of redrawing the whole keyboard with idle-state keys
            // only draw the previously selected key with idle-state key colouring
            get_char_coords(_prev_row, _prev_col, &pc, &px, &py);
            _lcd->setTextColor(_colour_fade, _colour_back);
            if (pc == KBD_SPECIALCHAR_BACKSPACE) {
                draw_backspace(false);
            }
            else if (pc == KBD_SPECIALCHAR_ENTER) {
                draw_enter(false);
            }
            else
            {
                _lcd->setCursor(px, py);
                draw_box(px, py, _colour_back, true);
                _lcd->write(pc);
            }
        }

        // now we can draw the selected key with selected-state colouring
        set_sel_colour();

        if (_cur_char == KBD_SPECIALCHAR_BACKSPACE)
        {
            draw_backspace(true);
        }
        else if (_cur_char == KBD_SPECIALCHAR_ENTER)
        {
            draw_enter(true);
        }
        else
        {
            _lcd->setCursor(x, y);
            if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
                draw_box(x, y, _colour_box & KBD_COLOUR_MASK, true);
            }
            _lcd->write(_cur_char);
            if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBORDER) {
                draw_box(x, y, _colour_box & KBD_COLOUR_MASK, false);
            }
        }
    }

    draw_inputstr();

    _prev_col  = _cur_col;
    _prev_row  = _cur_row;
    _prev_char = _cur_char;
    _prev_rot  = _cur_rot;
    _prev_lowercase = _is_lowercase;
}

void FairyKeyboard::toggleLowerCase()
{
    _is_lowercase ^= true;
    _start_time = millis();
}

void FairyKeyboard::setLowerCase(bool x)
{
    if (x != _is_lowercase) {
        _start_time = millis();
    }
    _is_lowercase = x;
}

bool FairyKeyboard::click(void)
{
    _start_time = millis();
    _recent_click = true;
    if (_cur_char == KBD_SPECIALCHAR_ENTER)
    {
        // enter
        return true;
    }
    else if (_cur_char == KBD_SPECIALCHAR_BACKSPACE)
    {
        // backspace
        if (_str_idx > 0) {
            _str_idx -= 1;
        }
        _str[_str_idx] = 0;
        return false;
    }

    if (_str_idx < KBD_STR_LEN)
    {
        _str[_str_idx] = _cur_char;
        _str_idx += 1;
        _str[_str_idx] = 0;
    }
    return false;
}

void FairyKeyboard::get_char_coords(int8_t row, int8_t col, char* outchar, int16_t* outx, int16_t* outy)
{
    int col_span = KBD_GET_COL_SPAN(row);
    char c;
    *outy = KBD_BOARD_Y + (row * KBD_LINE_SPACING);
    *outx = ((_screen_width - (KBD_CHAR_WIDTH * col_span)) / 2) + (col * KBD_CHAR_WIDTH);
    if (row == 2) {
        *outchar = line_3[col];
    }
    else if (row == 1) {
        *outchar = _is_lowercase ? line_2_l[col] : line_2_u[col];
    }
    else if (row == 0) {
        *outchar = _is_lowercase ? line_1_l[col] : line_1_u[col];
    }
    // TODO: this can be extended to include more possible characters
}

void FairyKeyboard::draw_inputstr()
{
    uint32_t now = millis();
    uint32_t tspan = now - _start_time;
    int col_span = KBD_GET_COL_SPAN(0);
    int x = ((_screen_width - (KBD_CHAR_WIDTH * col_span)) / 2);
    _lcd->setCursor(x, KBD_TYPE_Y);
    _lcd->setTextColor(_colour_fore, _colour_back);
    if (_recent_click)
    {
        // a recent click leaves the risk of leaving the carrot
        int i;
        // write out every character except the last one
        for (i = 0; i < _str_idx - 1; i++) {
            _lcd->write(_str[i]);
        }
        // blank out the space underneat the last character
        _lcd->fillRect(_lcd->getCursorX(), KBD_DIVIDER_Y - KBD_LINE_SPACING, KBD_CHAR_WIDTH * 2, KBD_LINE_SPACING, _colour_back); // blank a little section
        _lcd->write(_str[_str_idx - 1]); // write the last character
        _recent_click = false;
    }
    else {
        _lcd->print(_str); // no risk, print the string normally
    }

    if (_str_idx < KBD_STR_LEN)
    {
        // blink the carrot
        if ((tspan % _blink_dly) < (_blink_dly / 2)) {
            _lcd->write('_');
        }
        // yes I know it's spelled caret
    }

    // blank rest of line
    _lcd->fillRect(_lcd->getCursorX(), KBD_DIVIDER_Y - KBD_LINE_SPACING, _screen_width - _lcd->getCursorX() - 1, KBD_LINE_SPACING, _colour_back);
}

void FairyKeyboard::draw_box(int16_t x, int16_t y, uint16_t colour, bool fill)
{
    #define KBD_DRAW_BOX_COORDS(xxx, yyy)    (xxx) - 2, (yyy) - KBD_LINE_SPACING + 5, KBD_CHAR_WIDTH + 1, KBD_LINE_SPACING + 1
    if (fill) {
        _lcd->fillRect(KBD_DRAW_BOX_COORDS(x, y), colour);
    }
    else {
        _lcd->drawRect(KBD_DRAW_BOX_COORDS(x, y), colour);
    }
};

void FairyKeyboard::draw_backspace(bool sel)
{
    int16_t row = 2, col = 0, x, y;
    char c;
    get_char_coords(row, col, &c, &x, &y);
    _lcd->setTextFont(2);
    _lcd->setCursor(x + 7, y - KBD_LINE_SPACING + 10);
    #define KBD_BACKSPACE_BOX_COORDS(xxx, yyy) (xxx) - 1, (yyy) - KBD_LINE_SPACING + 5, (KBD_CHAR_WIDTH * 2) - 1, KBD_LINE_SPACING + 1
    if (sel)
    {
        set_sel_colour();
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
            _lcd->fillRect(KBD_BACKSPACE_BOX_COORDS(x, y), _colour_box & KBD_COLOUR_MASK);
        }
    }
    else
    {
        _lcd->setTextColor(_colour_fade, _colour_back);
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
            _lcd->fillRect(KBD_BACKSPACE_BOX_COORDS(x, y), _colour_back);
        }
    }
    _lcd->write('<');
    _lcd->setCursor(_lcd->getCursorX() + 1, _lcd->getCursorY() - 1); // needs a bit more space
    _lcd->write('x');
    _lcd->setCursor(_lcd->getCursorX() + 1, _lcd->getCursorY());     // needs a bit more space
    _lcd->write('x');
    if (sel)
    {
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBORDER) {
            _lcd->drawRect(KBD_BACKSPACE_BOX_COORDS(x, y), _colour_box & KBD_COLOUR_MASK);
        }
    }
    else
    {
        _lcd->drawRect(KBD_BACKSPACE_BOX_COORDS(x, y), _colour_back);
    }
    _lcd->setFreeFont(&KBD_FONT_MAIN);
};

void FairyKeyboard::draw_enter(bool sel)
{
    int16_t row = 2, col = 12, x, y;
    char c;
    get_char_coords(row, col, &c, &x, &y);
    _lcd->setTextFont(2);
    _lcd->setCursor(x + 1, y - KBD_LINE_SPACING + 9);
    #define KBD_ENTERKEY_BOX_COORDS(xxx, yyy) (xxx) - 1, (yyy) - KBD_LINE_SPACING + 5, (KBD_CHAR_WIDTH * 2) + 2, KBD_LINE_SPACING + 1
    if (sel)
    {
        set_sel_colour();
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
            _lcd->fillRect(KBD_ENTERKEY_BOX_COORDS(x, y), _colour_box & KBD_COLOUR_MASK);
        }
    }
    else
    {
        _lcd->setTextColor(_colour_fade, _colour_back);
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
            _lcd->fillRect(KBD_ENTERKEY_BOX_COORDS(x, y), _colour_back);
        }
    }
    _lcd->write('>');
    _lcd->write('E');
    _lcd->write('N');
    _lcd->write('T');
    if (sel)
    {
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBORDER) {
            _lcd->drawRect(KBD_ENTERKEY_BOX_COORDS(x, y), _colour_box & KBD_COLOUR_MASK);
        }
    }
    else
    {
        _lcd->drawRect(KBD_ENTERKEY_BOX_COORDS(x, y), _colour_back);
    }
    _lcd->setFreeFont(&KBD_FONT_MAIN);
};

void FairyKeyboard::set_sel_colour()
{
    if ((_colour_hilite & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_HLACTIVE)
    {
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
            _lcd->setTextColor(_colour_hilite & KBD_COLOUR_MASK, _colour_box & KBD_COLOUR_MASK);
        }
        else {
            _lcd->setTextColor(_colour_hilite & KBD_COLOUR_MASK, _colour_back);
        }
    }
    else
    {
        if ((_colour_box & KBD_COLOURFLAG_MASK) == KBD_COLOURFLAG_BOXBACKFILL) {
            _lcd->setTextColor(_colour_fore, _colour_box & KBD_COLOUR_MASK);
        }
        else {
            _lcd->setTextColor(_colour_fore, _colour_back);
        }
    }
}

void FairyKeyboard::register_redraw_cb(void(*cb)(void))
{
    redraw_cb = cb;
}

void FairyKeyboard::set_str(char* x)
{
    strncpy(_str, x, KBD_STR_LEN);
    _str_idx = strlen(_str);
    if (_str_idx >= KBD_STR_LEN) {
        _str_idx = KBD_STR_LEN;
        _str[_str_idx] = 0;
    }
}
