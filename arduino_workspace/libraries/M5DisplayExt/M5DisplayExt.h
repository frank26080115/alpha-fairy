#ifndef _M5DISPLAYEXT_H_
#define _M5DISPLAYEXT_H_

#include <M5Display.h>
#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include <SPIFFS.h>

class M5DisplayExt : public M5Display {
  public:
    #ifdef ENABLE_BUILD_BMP
    void drawBmpFile(fs::FS &fs, const char *path, uint16_t x, uint16_t y);
    void drawBmpFileSprite(TFT_eSPI* sprite, fs::FS &fs, const char *path, uint16_t x, uint16_t y);
    #endif

    #ifdef ENABLE_BUILD_JPG
    void drawJpgFile(fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  jpeg_div_t scale = JPEG_DIV_NONE);
    //void drawJpgFileSprite(TFT_eSPI* sprite, fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
    //              uint16_t maxWidth = 0, uint16_t maxHeight = 0,
    //              uint16_t offX = 0, uint16_t offY = 0,
    //              jpeg_div_t scale = JPEG_DIV_NONE);
    #endif

    void drawPngFile(fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  double scale = 1.0, uint8_t alphaThreshold = 127);
    void drawPngFileSprite(TFT_eSPI* sprite, fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  double scale = 1.0, uint8_t alphaThreshold = 127);

    inline void writePixel(uint16_t color) {
      SPI.write16(color);
    }
    inline void writePixels(uint16_t * colors, uint32_t len) {
      SPI.writePixels((uint8_t*)colors , len * 2);
    }

  void (*cb_needboost)(void) = NULL;

  private:
    void need_boost(void) { if (cb_needboost != NULL) { cb_needboost(); } };
};

extern M5DisplayExt M5Lcd;

#endif