#ifndef _SPRITEMGR_H_
#define _SPRITEMGR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "M5DisplayExt.h"
#include "M5Display.h" // this includes "Sprite.h" for us

typedef struct
{
    uint16_t uid;
    TFT_eSprite* sprite;
    void* next_node;
}
sprmgr_item_t;

class SpriteMgr
{
    public:
        SpriteMgr(M5DisplayExt* tft);
        bool load(const char* fp, int16_t width, int16_t height);
        void draw(const char* fp, int16_t x, int16_t y, int16_t width = 0, int16_t height = 0);
        TFT_eSprite* get(const char* fp);
        uint8_t holder_flag;

    private:
        M5DisplayExt* tft;
        sprmgr_item_t* head_node;
        sprmgr_item_t* last(void);
};

#endif
