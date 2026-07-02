#ifndef _M5DISPLAYEXT_H_
#define _M5DISPLAYEXT_H_

#include <M5GFX.h>
#include <Arduino.h>
#include <FS.h>
#include <SPI.h>

static constexpr uint8_t M5LCD_BRIGHTNESS_MAX = 255;

typedef enum {
  JPEG_DIV_NONE,
  JPEG_DIV_2,
  JPEG_DIV_4,
  JPEG_DIV_8,
  JPEG_DIV_MAX
} jpeg_div_t;

class M5DisplayExt : public M5GFX {
  public:
    void begin(void);
    inline void highlight(bool isHighlight) { _highlighted = isHighlight; }
    inline void setHighlightColor(uint16_t color) { _highlight_color = color; }

    #ifdef ENABLE_BUILD_BMP
    void drawBmpFile(fs::FS &fs, const char *path, uint16_t x, uint16_t y);
    void drawBmpFileSprite(LovyanGFX* sprite, fs::FS &fs, const char *path, uint16_t x, uint16_t y);
    #endif

    #ifdef ENABLE_BUILD_JPG
    void drawJpgFile(fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  jpeg_div_t scale = JPEG_DIV_NONE);
    //void drawJpgFileSprite(LovyanGFX* sprite, fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
    //              uint16_t maxWidth = 0, uint16_t maxHeight = 0,
    //              uint16_t offX = 0, uint16_t offY = 0,
    //              jpeg_div_t scale = JPEG_DIV_NONE);
    #endif

    void drawPngFile(fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  double scale = 1.0, uint8_t alphaThreshold = 127);
    void drawPngFileSprite(LovyanGFX* sprite, fs::FS &fs, const char *path, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  double scale = 1.0, uint8_t alphaThreshold = 127);
    void drawPngData(const uint8_t* data, size_t len, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  double scale = 1.0, uint8_t alphaThreshold = 127);
    void drawPngDataSprite(LovyanGFX* sprite, const uint8_t* data, size_t len, uint16_t x = 0, uint16_t y = 0,
                  uint16_t maxWidth = 0, uint16_t maxHeight = 0,
                  uint16_t offX = 0, uint16_t offY = 0,
                  double scale = 1.0, uint8_t alphaThreshold = 127);

    inline void writePixel(uint16_t color) {
      M5GFX::writePixels(&color, 1, true);
    }
    inline void writePixels(uint16_t * colors, uint32_t len) {
      M5GFX::writePixels(colors, len, true);
    }

  void (*cb_needboost)(void) = NULL;

  private:
    bool _highlighted = false;
    uint16_t _highlight_color = TFT_BLACK;
    void need_boost(void) { if (cb_needboost != NULL) { cb_needboost(); } };
};

extern M5DisplayExt M5Lcd;

#endif
