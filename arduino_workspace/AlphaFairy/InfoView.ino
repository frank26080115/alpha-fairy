#if 0

#include "AlphaFairy.h"
#include "FairyMenu.h"

uint8_t infoscreen_mode;

enum
{
    INFOSCR_LANDSCAPE_WHITE,
    INFOSCR_LANDSCAPE_BLACK,
    INFOSCR_PORTRAIT_WHITE,
    INFOSCR_PORTRAIT_BLACK,
    INFOSCR_END;
};

void infoscreen_setup(uint8_t mode)
{
    infoscreen_mode = mode;
    uint16_t forecolour, backcolour;
    switch (mode)
    {
        case INFOSCR_LANDSCAPE_BLACK:
            forecolour = TFT_BLACK;
            backcolour = TFT_WHITE;
            M5Lcd.rotate(1);
            break;
        case INFOSCR_PORTRAIT_BLACK:
            forecolour = TFT_BLACK;
            backcolour = TFT_WHITE;
            M5Lcd.rotate(0);
            break;
        case INFOSCR_PORTRAIT_WHITE:
            forecolour = TFT_WHITE;
            backcolour = TFT_BLACK;
            M5Lcd.rotate(0);
            break;
        case INFOSCR_LANDSCAPE_WHITE:
        default:
            forecolour = TFT_WHITE;
            backcolour = TFT_BLACK;
            M5Lcd.rotate(1);
            break;
    };
    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(true);
    M5Lcd.setHighlightColor(backcolour); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
    M5Lcd.setTextColor(forecolour, backcolour);
    M5Lcd.fillScreen(backcolour);
}

void infoscreen_update()
{
    
}

class AppInfoView : public FairyMenuItem
{
    public:
        AppInfoView() : FairyMenuItem()
        {
        };

        virtual bool can_navTo(void)
        {
            return fairycam.isOperating();
        };

        virtual void on_navTo(void)
        {
        };

        virtual void on_navOut(void)
        {
        };

        virtual bool on_execute(void)
        {
            return false;
        };

    protected:
};

#endif