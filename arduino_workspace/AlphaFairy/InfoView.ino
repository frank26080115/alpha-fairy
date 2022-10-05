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
    M5Lcd.setTextWrap(false);
    M5Lcd.setHighlightColor(backcolour); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
    M5Lcd.setTextColor(forecolour, backcolour);
    M5Lcd.fillScreen(backcolour);
    M5Lcd.setTextFont(4);
}

void infoscreen_print()
{
    uint16_t expomode = fairycam.get_exposureMode();

    infoscreen_printResetState();
    if (expomode == SONYALPHA_EXPOMODE_P || expomode == SONYALPHA_EXPOMODE_MovieP || expomode == SONYALPHA_EXPOMODE_IntelligentAuto || expomode == SONYALPHA_EXPOMODE_SuperiorAuto)
    {
        infoscreen_printExpoComp();
        infoscreen_printShutterSpeed();
        infoscreen_printAperture();
        infoscreen_printIso();
    }
    else if (expomode == SONYALPHA_EXPOMODE_A || expomode == SONYALPHA_EXPOMODE_MovieA)
    {
        infoscreen_printAperture();
        infoscreen_printShutterSpeed();
        infoscreen_printIso();
        infoscreen_printExpoComp();
    }
    else
    {
        infoscreen_printShutterSpeed();
        infoscreen_printIso();
        infoscreen_printAperture();
        infoscreen_printExpoComp();
    }
}

void infoscreen_printExpoMode(uint16_t expomode)
{
    if (expomode == SONYALPHA_EXPOMODE_IntelligentAuto || expomode == SONYALPHA_EXPOMODE_SuperiorAuto)
    {
        M5Lcd.print("AUTO");
    }
    else
    {
        M5Lcd.print("   ");
        if (expomode == SONYALPHA_EXPOMODE_M || expomode == SONYALPHA_EXPOMODE_MovieM) {
            M5Lcd.print("M");
        }
        else if (expomode == SONYALPHA_EXPOMODE_P || expomode == SONYALPHA_EXPOMODE_MovieP) {
            M5Lcd.print("P");
        }
        else if (expomode == SONYALPHA_EXPOMODE_A || expomode == SONYALPHA_EXPOMODE_MovieA) {
            M5Lcd.print("A");
        }
        else if (expomode == SONYALPHA_EXPOMODE_S || expomode == SONYALPHA_EXPOMODE_MovieS) {
            M5Lcd.print("S");
        }
        else {
            M5Lcd.print(" ");
        }
    }
}

void infoscreen_printCornerInfo(uint16_t expomode)
{
    bool diff_colour = false;

    M5Lcd.setTextFont(4);
    M5Lcd.setCursor(M5Lcd.width() - (12 * 4), SUBMENU_Y_OFFSET);
    infoscreen_printExpoMode(expomode);

    M5Lcd.setTextFont(2);
    M5Lcd.setCursor(M5Lcd.width() - (12 * 4), SUBMENU_Y_OFFSET + 16);
    if (fairycam.is_manuallyfocused())
    {
        M5Lcd.print("MF   ");
    }
    else if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_FocusMode))
    {
        int32_t afmode = ptpcam.get_property(SONYALPHA_PROPCODE_FocusMode);
        if (afmode == SONYALPHA_AFMODE_DMF) {
            M5Lcd.print("DMF  ");
        }
        else {

            if (ptpcam.has_property(SONYALPHA_PROPCODE_FocusFound))
            {
                int32_t focus_status = ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound);
                if (focus_status == SONYALPHA_FOCUSSTATUS_FOCUSED)
                {
                    M5Lcd.setHighlightColor(TFT_DARKGREEN);
                    M5Lcd.setTextColor(TFT_WHITE, TFT_DARKGREEN);
                    diff_colour = true;
                }
                else if (focus_status == SONYALPHA_FOCUSSTATUS_HUNTING)
                {
                    M5Lcd.setHighlightColor(backcolour);
                    M5Lcd.setTextColor(backcolour == TFT_BLACK ? TFT_GREEN : TFT_DARKGREEN, backcolour);
                    diff_colour = true;
                }
                else if (focus_status == SONYALPHA_FOCUSSTATUS_FAILED || focus_status == SONYALPHA_FOCUSSTATUS_FAILED)
                {
                    M5Lcd.setHighlightColor(backcolour);
                    M5Lcd.setTextColor(TFT_RED, backcolour);
                    diff_colour = true;
                }
            }

            M5Lcd.print("AF");
            if (afmode == SONYALPHA_AFMODE_AFC) {
                M5Lcd.print("-C");
            }
            else if (afmode == SONYALPHA_AFMODE_AFS) {
                M5Lcd.print("-S");
            }
            else if (afmode == SONYALPHA_AFMODE_AFA) {
                M5Lcd.print("-A");
            }
            M5Lcd.print(" ");
        }
    }
    else if (httpcam.isOperating())
    {
        M5Lcd.print(httpcam.get_str_afmode());
        M5Lcd.print(" ");
    }

    if (ptpcam.isOperating() && ptpcam.is_manuallyfocused() && ptpcam.has_property(SONYALPHA_PROPCODE_ManualFocusDist))
    {
        M5Lcd.setCursor(M5Lcd.width() - (12 * 4), SUBMENU_Y_OFFSET + 24);
        M5Lcd.printf("%d  ", ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist));
    }

    if (diff_colour) {
        M5Lcd.setHighlightColor(backcolour);
        M5Lcd.setTextColor(forecolour, backcolour);
    }
}

void infoscreen_update()
{
    uint16_t expomode = fairycam.get_exposureMode();


    

    
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