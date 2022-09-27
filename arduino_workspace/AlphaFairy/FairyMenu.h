#ifndef _FAIRYMENU_H_
#define _FAIRYMENU_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "AlphaFairy.h"

extern bool redraw_flag;
extern void cpufreq_boost(void);

// the base class that every type of menu item is derived from
class FairyItem
{
    public:
        virtual bool     on_execute       (void) { return false; }; // performs a the app's action, return true for "should exit out of submenu"

        // for some shared items, it'll be useful to know what parent menu is holding it
        inline  uint16_t get_id           (void) { return _id; };
        virtual void     set_parent(void* x, uint16_t id) { _parent = x; _parent_id = id; };
        inline  uint16_t get_parentId     (void) { return _parent_id; };
        inline  void*    get_parent       (void) { return _parent; };

        virtual bool     can_navTo        (void) { return true; }; // used to hide an item
        virtual void     on_navTo         (void) { };              // usually used to draw an item
        virtual void     on_navOut        (void) { };              // usually used to stop something
        virtual void     on_eachFrame     (void) { };              // usually used to poll something specific and/or to update text

        inline void      set_redraw       (void) { cpufreq_boost(); redraw_flag = true; } // forces a redraw on next loop, useful for when app execution has drawn extra stuff that needs to disappear

    protected:
        void* _parent = NULL;
        uint16_t _id, _parent_id = 0;
};

// used for linked list
typedef struct
{
    FairyItem* item;
    void* next_node;
    void* prev_node;
}
FairyItemNode_t;

class FairyMenuItem : public FairyItem
{
    public:
        FairyMenuItem(const char* img_fname, uint16_t id = 0);
        virtual void     reset            (void) {};
        virtual bool     on_execute       (void) { return false; };      // do the thing, return true means "exit submenu" // by default, do nothing and do not exit
        virtual bool     can_navTo        (void) { return true; };       // used to hide an item // by default, do not hide
        virtual void     on_navTo         (void) { on_redraw(); };       // usually used to draw the main image
        virtual void     on_navOut        (void) {};
        virtual void     on_eachFrame     (void) {};
        virtual void     on_spin          (int8_t x) {};                 // handle IMU spin, do not actually do actions to the imu object here
        virtual void     on_redraw        (void) { draw_mainImage(); };  // usually used to draw the main image
        virtual bool     check_redraw     (void) { return false; };
        virtual void     draw_mainImage   (void);
        virtual void     draw_statusBar   (void);
        inline  char*    get_mainImage    (void) { return _main_img; };
        inline  int16_t  get_mainImage_X  (void) { return _main_img_x; };
        inline  int16_t  get_mainImage_Y  (void) { return _main_img_y; };

    protected:
        char* _main_img;
        int16_t _main_img_x = 0, _main_img_y = 0;

        bool must_be_connected(void); // convenient to show the "connecting..." animation when required // returns true if connected, false if disconnected
        bool must_be_ptp(void);       // convenient to show the "unsupported camera" error screen, also uses must_be_connected // returns true if connected and supported, false if otherwise unable to operate
};

class FairySubmenu : public FairyMenuItem
{
    // this class represents a submenu with items that are full screen, mostly white background, in portrait orientation
    public:
        FairySubmenu(const char* img_fname, uint16_t id = 0);
        inline  void set_bigbtn_nav(bool x) { _bigbtn_nav = x; }; // allows the use of the big button as a next button, which disables on_execute completely
        virtual void install(FairyItem* itm);                     // adds item to linked list
        virtual bool on_execute(void);                            // usually used for a menu loop
        virtual bool task(void);                                  // usually used to do stuff inside the menu loop, return true means "exit submenu"
        inline  void rewind(void) { cur_node = head_node; };      // reset to showing the first item in the linked list
        FairyItem* nav_next(void);                                // navigate to the next item that is not hidden
        #ifdef ENABLE_BUILD_LEPTON
        FairyItem* nav_prev(void);                                // navigate to the prev item that is not hidden
        #endif

        // linked list getters
        inline FairyItemNode_t* get_headNode(void) { return head_node; };
        inline FairyItemNode_t* get_tailNode(void) { if (head_node == NULL ) return NULL; return (FairyItemNode_t*)(head_node->prev_node); };

    protected:
        // linked list
        FairyItemNode_t* head_node = NULL;
        FairyItemNode_t* cur_node = NULL;

        bool _bigbtn_nav = false; // allows the use of the big button as a next button, which disables on_execute completely
};

class FairyCfgItem : public FairyItem
{
    // this class is a page within the "configurable app" style of apps, the apps usually consist of many pages of settings and one page for execution
    public:
        // initialize as a page representing a setting item
        FairyCfgItem(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint32_t fmt_flags);

        // initialize as a page representing a way to execute action
        FairyCfgItem(const char* disp_name, bool (*cb)(void*), const char* icon = NULL);

               void    set_icon(const char* icon);              // sets the icon for the top-right corner
               void    set_font(int fn);                        // set the font size of the first line of text, use a negative number for auto-sizing
        inline int32_t get_val (void) { return *_linked_ptr; }; // get the actual value of the configurable item

        // a way of checking what kind of page this is
        inline bool is_value(void) { return _linked_ptr != NULL; };
        inline bool is_func (void) { return _cb != NULL; };

        virtual bool     on_execute     (void); // do the thing, by default, this calls the registered callback function, return true means "exit submenu"
        virtual bool     can_navTo      (void) { return true; };
        virtual void     on_navTo       (void);
        virtual void     on_navOut      (void) { if (_autosave && dirty) { dirty = false; settings_save(); } }; // auto-save if the value changed
        virtual void     on_redraw      (void);
        virtual void     on_eachFrame   (void) { on_extraPoll(); on_drawLive(); };
        virtual void     on_readjust    (void) {};
        virtual void     on_drawLive    (void) {}; // this is used for drawing even faster than on_eachFrame, example: mic level bar
        virtual void     on_extraPoll   (void) {}; // this is used for polling even faster than on_eachFrame, example: mic sample reading
        virtual void     on_tiltChange  (void);
        virtual void     on_checkAdjust (int8_t);
        virtual void     draw_statusBar (void);
        virtual void     draw_value(void), draw_value(int8_t);
        virtual void     draw_name(void);
        virtual void     draw_icon(void);
        virtual void     blank_text(void); // blanks entire text area
        virtual void     blank_line(void); // blanks rest of line

    protected:
        uint16_t _margin_x = SUBMENU_X_OFFSET, _margin_y = SUBMENU_Y_OFFSET, _line0_height = 16, _line_space = 1, _font_num = 4; // default graphic config
        char* _disp_name;
        char* _icon_fpath = NULL;
        int16_t _icon_width = 0;
        int32_t* _linked_ptr = NULL;
        bool (*_cb)(void*) = NULL;
        int32_t _val_min, _val_max, _step_size;
        uint32_t _fmt_flags;
        bool _autosave = false;
        static bool dirty;

        void set_name(const char*);
        int16_t get_y(int8_t linenum); // gets the Y coordinate of a text line, compensating for margin and font sizes, 0 indexed
};

class FairyCfgApp : public FairySubmenu
{
    // this class represents an app with configurable items, mostly black background with icons, in landscape orientation
    public:
        FairyCfgApp(const char* img_fname, const char* icon_fname, uint16_t id = 0);
        virtual void install(FairyCfgItem* itm) { FairySubmenu::install((FairyItem*)itm); };
        virtual bool on_execute(void); // this is the app loop, return true means "exit app"
        virtual bool task(void);       // the inner part of the app loop, return true means "exit app"
        virtual bool has_icon(void) { return _icon_fname != NULL; };
        virtual void draw_icon(void);

    protected:
        char* _icon_fname = NULL;
        int16_t _icon_width = 0;
        static int8_t prev_tilt;
};

#endif
