#ifndef _FAIRYKEYBOARD_H_
#define _FAIRYKEYBOARD_H_

#include <Arduino.h>
#include <M5StickCPlus.h>
#include <M5Display.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define KBD_COLOURFLAG_MASK        0xFF0000
#define KBD_COLOUR_MASK            0xFFFF
#define KBD_COLOURFLAG_NONE        0x000000
#define KBD_COLOURFLAG_BOXBACKFILL 0x020000
#define KBD_COLOURFLAG_BOXBORDER   0x010000
#define KBD_COLOURFLAG_HLACTIVE    0x010000

#define KBD_STR_LEN                15

class FairyKeyboard
{
    public:
        FairyKeyboard(M5Display* used_lcd,
                            uint16_t colour_bg    = TFT_WHITE,
                            uint16_t colour_fg    = TFT_BLACK,
                            uint16_t colour_fade  = TFT_LIGHTGREY,
                            uint32_t colour_hl    = KBD_COLOURFLAG_HLACTIVE    | TFT_GREEN,
                            uint32_t colour_box   = KBD_COLOURFLAG_BOXBACKFILL | TFT_RED,
                            uint16_t roll_span    = 90,
                            uint16_t pitch_span   = 90,
                            uint16_t pitch_hyster = 5,
                            uint32_t blink_dly    = 400,
                            bool     auto_flip    = true
                            );

        inline char* get_str(void) { return _str; };               // get the current input string
        inline void  clr_str(void) { _str[0] = 0; _str_idx = 0; }; // clear the current input string

        void reset(void);                          // resets state variables to default
        void lcd_setup(void);                      // sets basic LCD paramters
        void draw_base(void);                      // draws the whole background basic keyboard
        void update(float roll, float pitch);      // call for redraw and new IMU data
        void toggleLowerCase(void);                // toggles shift key
        void setLowerCase(bool x);                 // presses shift key
        bool click(void);                          // call when the big button is pressed, returns true if the ENTER key is the one being clicked upon
        void register_redraw_cb(void(*)(void));    // register a function to be called when a whole screen redraw is called

    protected:
        M5Display* _lcd;

        void (*redraw_cb)(void) = NULL;

        bool _auto_flip;
        uint16_t _screen_width, _screen_height;
        char _cur_char, _prev_char;
        uint8_t _cur_rot, _prev_rot;
        uint16_t _colour_back, _colour_fore, _colour_fade;
        uint32_t _colour_hilite, _colour_box;
        uint16_t _roll_span, _pitch_span;
        int8_t _cur_row , _cur_col;
        int8_t _prev_row, _prev_col;
        bool _has_pitch;
        int16_t _center_pitch;
        int16_t _pitch_hyster;
        uint32_t _start_time, _blink_dly;
        bool _is_lowercase, _prev_lowercase;
        bool _recent_click;

        char _str[KBD_STR_LEN + 2]; // stores current input string
        uint8_t _str_idx;           // write pointer for input string

        void get_char_coords(int8_t row, int8_t col, char* outchar, int16_t* outx, int16_t* outy); // get the character, X, and Y coordinates, when given row and column as parameters
        void draw_box(int16_t x, int16_t y, uint16_t colour, bool fill); // draws a box around the currently selected key
        void draw_backspace(bool sel); // draws the backspace key, param "sel" means "currently selected"
        void draw_enter(bool sel);     // draws the   enter   key, param "sel" means "currently selected"
        void set_sel_colour(void);     // sets the selected colour according to settings
        void draw_inputstr(void);      // draws the input text
};

#endif
