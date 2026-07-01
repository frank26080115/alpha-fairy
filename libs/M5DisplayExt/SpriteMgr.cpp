#include "SpriteMgr.h"

SpriteMgr::SpriteMgr(M5DisplayExt* tft)
{
    this->tft = tft;
    this->head_node = NULL;
    this->holder_flag = 0;
}

bool SpriteMgr::load(const uint8_t* data, size_t len, int16_t width, int16_t height)
{
    need_boost();

    if (data == NULL || len == 0) {
        return false;
    }

    if (get(data, len) != NULL) {
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
    this->tft->drawPngDataSprite(sprite, data, len, 0, 0);
    node->sprite = sprite;
    node->data = data;
    node->len = len;
    node->next_node = NULL;

    //Serial.printf("SpMgr free heap after %u\r\n", ESP.getFreeHeap());

    return true;
}

void SpriteMgr::draw(const uint8_t* data, size_t len, int16_t x, int16_t y, int16_t width, int16_t height)
{
    need_boost();

    TFT_eSprite* sprite = get(data, len);
    if (sprite == NULL)
    {
        // does not exist, create new if possible
        if (width > 0 && height > 0)
        {
            if (load(data, len, width, height))
            {
                sprite = get(data, len);
                if (sprite == NULL) {
                    return;
                }
            }
            else
            {
                // failed, maybe ran out of memory, so draw directly
                this->tft->drawPngData(data, len, x, y);
                return;
            }
        }
        else {
            return;
        }
    }
    // draw to screen
    sprite->pushSprite(x, y);
}

TFT_eSprite* SpriteMgr::get(const uint8_t* data, size_t len)
{
    sprmgr_item_t* cur_node = this->head_node;
    while (cur_node != NULL) {
        if (cur_node->data == data && cur_node->len == len) {
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
    need_boost();

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
