#ifndef _FAIRYMENU_H_
#define _FAIRYMENU_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "AlphaFairy.h"

extern bool redraw_flag;

class FairyItem
{
    public:
        inline  uint16_t get_id           (void) { return _id; };
        virtual bool     on_execute       (void) { return false; };
        virtual void     set_parent(void* x, uint16_t id) { _parent = x; _parent_id = id; };
        inline  uint16_t get_parentId     (void) { return _parent_id; };
        virtual bool     can_navTo        (void) { return true; };
        virtual void     on_navTo         (void) { };
        virtual void     on_navOut        (void) { };
        virtual void     on_eachFrame     (void) { };
    protected:
        void* _parent = NULL;
        uint16_t _id, _parent_id = 0;
};

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
        FairyMenuItem(const char* img_fname, uint16_t id);
        virtual void     reset            (void) {};
        virtual bool     on_execute       (void) { return false; };
        virtual bool     can_navTo        (void) { return true; };
        virtual void     on_navTo         (void) { on_redraw(); };
        virtual void     on_navOut        (void) {};
        virtual void     on_eachFrame     (void) {};
        virtual void     on_spin          (int8_t x) {};
        virtual void     on_redraw        (void) { draw_mainImage(); };
        virtual bool     check_redraw     (void) { return false; };
        virtual void     draw_mainImage   (void);
        virtual void     draw_statusBar   (void);
        inline  char*    get_mainImage    (void) { return _main_img; };
        inline  int16_t  get_mainImage_X  (void) { return _main_img_x; };
        inline  int16_t  get_mainImage_Y  (void) { return _main_img_y; };

        inline void      set_redraw       (void) { redraw_flag = true; }

    protected:
        char* _main_img;
        int16_t _main_img_x = 0, _main_img_y = 0;
};

class FairySubmenu : public FairyMenuItem
{
    public:
        FairySubmenu(const char* img_fname, uint16_t id);
        virtual void install(FairyItem* itm);
        virtual bool on_execute(void);
        virtual bool task(void);
        inline void rewind(void) { cur_node = head_node; };
        FairyItem* nav_next(void);

        inline FairyItemNode_t* get_headNode(void) { return head_node; };
        inline FairyItemNode_t* get_tailNode(void) { if (head_node == NULL ) return NULL; return (FairyItemNode_t*)(head_node->prev_node); };

    protected:
        FairyItemNode_t* head_node = NULL;
        FairyItemNode_t* cur_node = NULL;

        bool _first_run;
};

class FairyCfgItem : public FairyItem
{
    public:
        FairyCfgItem(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint16_t fmt_flags);
        FairyCfgItem(const char* disp_name, bool (*cb)(void*), const char* icon = NULL);
               void    set_icon(const char* icon);
               void    set_font(int fn);
        inline int32_t get_val (void) { return *_linked_ptr; };

        inline bool is_value(void) { return _linked_ptr != NULL; };
        inline bool is_func (void) { return _cb != NULL; };

        virtual bool     on_execute     (void);
        virtual bool     can_navTo      (void) { return true; };
        virtual void     on_navTo       (void);
        virtual void     on_navOut      (void) { if (_autosave && dirty) { dirty = false; settings_save(); } };
        virtual void     on_redraw      (void);
        virtual void     on_eachFrame   (void) { on_extraPoll(); on_drawLive(); };
        virtual void     on_readjust    (void) {};
        virtual void     on_drawLive    (void) {};
        virtual void     on_extraPoll   (void) {};
        virtual void     on_tiltChange  (void);
        virtual void     on_checkAdjust (int8_t);
        virtual void     draw_statusBar (void);
        virtual void     draw_value(void), draw_value(int8_t);
        virtual void     draw_name(void);
        virtual void     draw_icon(void);
        virtual void     blank_text(void);
        virtual void     blank_line(void);

    protected:
        uint16_t _margin_x = SUBMENU_X_OFFSET, _margin_y = SUBMENU_Y_OFFSET, _line0_height = 0, _line_space = 1, _font_num = 4;
        char* _disp_name;
        char* _icon_fpath = NULL;
        int16_t _icon_width = 0;
        int32_t* _linked_ptr = NULL;
        bool (*_cb)(void*) = NULL;
        int32_t _val_min, _val_max, _step_size;
        uint16_t _fmt_flags;
        bool _autosave = false;
        static bool dirty;

        void set_name(const char*);
        int16_t get_y(int8_t linenum);
};

class FairyCfgApp : public FairySubmenu
{
    public:
        FairyCfgApp(const char* img_fname, const char* icon_fname, uint16_t id);
        virtual void install(FairyCfgItem* itm) { FairySubmenu::install((FairyItem*)itm); };
        virtual bool on_execute(void);
        virtual bool task(void);
        virtual bool has_icon(void) { return _icon_fname != NULL; };
        virtual void draw_icon(void);

    protected:
        char* _icon_fname = NULL;
        int16_t _icon_width = 0;
        static int8_t prev_tilt;
};

#endif
