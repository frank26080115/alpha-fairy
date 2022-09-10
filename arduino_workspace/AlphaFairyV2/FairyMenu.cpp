#include "FairyMenu.h"
#include "AlphaFairy.h"
#include <M5DisplayExt.h>


extern M5DisplayExt M5Lcd;

FairyMenuItem::FairyMenuItem(char* img_fname, uint16_t id)
{
    _id = id;
    _main_img = malloc(strlen(img_fname) + 2);
    strcpy(_main_img, img_fname);
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

void FairySubmenu::on_execute(void)
{
    //rewind();
    cur_node->item->draw_mainImage();
    app_waitAllRelease(BTN_DEBOUNCE);
    while (true)
    {
        if (app_poll())
        {
            if (task())
            {
                return;
            }
            pwr_sleepCheck();
        }
    }
}

void FairySubmenu::install(FairyMenuItem* itm)
{
    if (head_node == NULL)
    {
        head_node = (FairyMenuItemNode_t*)malloc(sizeof(FairyMenuItemNode_t));
        head_node->itm = itm;
        head_node->next_node = (void*)head_node;
        head_node->prev_node = (void*)head_node;
    }
    else
    {
        FairyMenuItemNode_t* tail = get_tailNode();
        FairyMenuItemNode_t* new_node = (FairyMenuItemNode_t*)malloc(sizeof(FairyMenuItemNode_t));
        new_node->item = itm;
        new_node->next_node  = (void*)head_node;
        new_node->prev_node  = (void*)tail;
        head_node->prev_node = (void*)new_node;
    }
    if (head_node != NULL && cur_node == NULL)
    {
        cur_node = head_node;
    }
}

FairyMenuItem* FairySubmenu::nav_next(void)
{
    FairyMenuItemNode_t* n = cur_node;
    uint8_t;
    if (cur_node == NULL) {
        return NULL;
    }
    for (i = 0; i < 10; i++) // this is a for loop just to prevent infinite loops
    {
        n = (FairyMenuItemNode_t*)(n->next_node);
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

    if (btnSide_hasPressed())
    {
        btnSide_clrPressed();
        cur_node->item->on_navOut();
        itm = nav_next();
        itm->on_navTo();
    }

    if (imu.getSpin() != 0)
    {
        cur_node->item->on_spin(imu.getSpin());
        imu.resetSpin();
    }

    cur_node->item->on_eachFrame();
    cur_node->item->draw_statusBar(); // the status bar function has its own frame rate control

    if (redraw)
    {
        cur_node->item->on_redraw();
        redraw_flag = false;
    }

    if (btnBig_hasPressed())
    {
        btnBig_clrPressed();
        cur_node->item->on_execute();
    }

    if (btnPwr_hasPressed())
    {
        btnPwr_clrPressed();
        return true;
    }

    return false;
}