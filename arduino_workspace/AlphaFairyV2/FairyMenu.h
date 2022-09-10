#ifndef _FAIRYMENU_H_
#define _FAIRYMENU_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

extern bool redraw_flag;

class FairyMenuItem
{
    public:
        FairyMenuItem(char* img_fname, uint16_t id);
        virtual void     reset            (void) {};
        inline  uint16_t get_id           (void) { return _id; };
        virtual bool     can_navTo        (void) { return true; };
        virtual void     on_navTo         (void) { draw_mainImage(); };
        virtual void     on_navOut        (void) {};
        virtual void     on_eachFrame     (void) {};
        virtual void     on_spin          (int8_t x) {};
        virtual void     on_redraw        (void) {};
        virtual void     draw_mainImage   (void);
        virtual void     draw_statusBar   (void);
        inline  char*    get_mainImage    (void) { return _main_img; };
        inline  int16_t  get_mainImage_X  (void) { return _main_img_x; }
        inline  int16_t  get_mainImage_Y  (void) { return _main_img_y; }
        virtual void     on_exeute        (void);

        inline void      set_redraw       (void) { redraw_flag = true; }

    protected:
        uint16_t _id;
        char* _main_img;
        int16_t _main_img_x = 0, _main_img_y = 0;
};

struct
{
    FairyMenuItem* item;
    void* next_node;
    void* prev_node;
}
FairyMenuItemNode_t;

class FairySubmenu : FairyMenuItem
{
    public:
        void install  (FairyMenuItem* itm);
        void on_exeute(void);
        bool task     (void);
        inline void rewind(void) { cur_node = head_node; };
        FairyMenuItem* nav_next(void);

        inline FairyMenuItemNode_t* get_headNode(void) { return head_node; };
        inline FairyMenuItemNode_t* get_tailNode(void) { if (head_node == NULL ) return NULL; return head_node->prev_node; };

    protected:
        FairyMenuItemNode_t* head_node = NULL;
        FairyMenuItemNode_t* cur_node = NULL;
};

class FairyConfigurableApp : FairyMenuItem
{
    
};

#endif
