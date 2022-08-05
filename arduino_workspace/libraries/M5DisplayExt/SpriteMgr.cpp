#include "SpriteMgr.h"
#include <FS.h>
#include <SPIFFS.h>

static uint16_t fletcher16_str(const uint8_t* data);

SpriteMgr::SpriteMgr(M5DisplayExt* tft)
{
    this->tft = tft;
    this->head_node = NULL;
    this->holder_flag = 0;
}

bool SpriteMgr::load(const char* fp, int16_t width, int16_t height)
{
    if (get(fp) != NULL) {
        return true;
    }

    #ifdef SPMGR_DEBUG_MEMORY
    if (this->head_node == NULL) {
        Serial.printf("SpMgr free heap before head %u\r\n", ESP.getFreeHeap());
    }
    #endif

    sprmgr_item_t* node = (sprmgr_item_t*)malloc(sizeof(sprmgr_item_t));
    if (node == NULL) {
        Serial.printf("SpMgr malloc null\r\n");
        return false;
    }
    TFT_eSprite* sprite = new TFT_eSprite(this->tft);
    if (sprite == NULL) {
        Serial.printf("SpMgr sprite null\r\n");
        free(node);
        return false;
    }

    if (this->head_node == NULL) {
        // first ever sprite
        this->head_node = node;
        node->prev_node = NULL;
    }
    else {
        // add to end of list
        sprmgr_item_t* last_node = last();
        last_node->next_node = (void*)node;
        node->prev_node = (void*)last_node;
    }

    sprite->createSprite(width, height);
    this->tft->drawPngFileSprite(sprite, SPIFFS, fp, 0, 0);
    node->sprite = sprite;
    node->uid = fletcher16_str((const uint8_t*)fp);
    node->next_node = NULL;

    //Serial.printf("SpMgr free heap after %u\r\n", ESP.getFreeHeap());

    return true;
}

void SpriteMgr::draw(const char* fp, int16_t x, int16_t y, int16_t width, int16_t height)
{
    TFT_eSprite* sprite = get(fp);
    if (sprite == NULL)
    {
        // does not exist, create new if possible
        if (width > 0 && height > 0)
        {
            if (load(fp, width, height))
            {
                sprite = get(fp);
                if (sprite == NULL) {
                    return;
                }
            }
            else
            {
                // failed, maybe ran out of memory, so draw directly
                this->tft->drawPngFile(SPIFFS, fp, x, y);
            }
        }
        else {
            return;
        }
    }
    // draw to screen
    sprite->pushSprite(x, y);
}

TFT_eSprite* SpriteMgr::get(const char* fp)
{
    uint16_t uid = fletcher16_str((const uint8_t*)fp);
    sprmgr_item_t* cur_node = this->head_node;
    while (cur_node != NULL) {
        if (cur_node->uid == uid) {
            return cur_node->sprite;
        }
        cur_node = (sprmgr_item_t*)(cur_node->next_node);
    }
    return NULL;
}

sprmgr_item_t* SpriteMgr::last(void)
{
    if (this->head_node == NULL) {
        return NULL;
    }
    sprmgr_item_t* cur_node = this->head_node;
    sprmgr_item_t* next_node;
    while (true) {
        // iterate through all nodes, loop until the current node has no more next node
        next_node = (sprmgr_item_t*)(cur_node->next_node);
        if (next_node == NULL) {
            return cur_node;
        }
        cur_node = next_node;
    }
    return NULL;
}

void SpriteMgr::unload_all(void)
{
    sprmgr_item_t* node = last();
    sprmgr_item_t* prev_node;
    while (node != NULL)
    {
        TFT_eSprite* s = node->sprite;
        if (s != NULL)
        {
            s->deleteSprite();
            delete s;
        }
        prev_node = (sprmgr_item_t*)(node->prev_node);
        free(node);
        node = prev_node;
    }
    this->head_node = NULL;

    #ifdef SPMGR_DEBUG_MEMORY
    Serial.printf("SpMgr free heap after unload %u\r\n", ESP.getFreeHeap());
    #endif
}

static uint16_t fletcher16_str(const uint8_t* data)
{
    // https://en.wikipedia.org/wiki/Fletcher%27s_checksum
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    int index;

    for ( index = 0; ; ++index )
    {
        uint8_t x = data[index];
        if (x == 0) { // string is null terminated
            break;
        }
        sum1 = (sum1 + x   ) % 255;
        sum2 = (sum2 + sum1) % 255;
    }
    
    return (sum2 << 8) | sum1;
}
