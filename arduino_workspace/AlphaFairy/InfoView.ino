#include "AlphaFairy.h"
#include "FairyMenu.h"

#define INFOSCR_CORNER_MARGIN 65
#define INFOSCR_MIDDIV_MARGIN 5
#define INFOSCR_LINESPACE 2

#define INFOSCR_REQ_TIME_LIMIT 2000

uint8_t infoscr_mode = 0;
uint16_t infoscr_forecolour, infoscr_backcolour;
uint8_t infoscr_itmIdx = 0;
uint32_t infoscr_tickTime = 0;
int8_t infoscr_editItem = -1;
uint16_t infoscr_expomode = 0;

extern bool tallylite_enable;
extern uint8_t qikrmt_imuState;

uint32_t infoscr_reqTime = 0;
int32_t infoscr_reqShutter, infoscr_reqAperture, infoscr_reqIso, infoscr_reqExpoComp;

void infoscr_setup(uint8_t mode, bool clr)
{
    static bool prev_movie = false;
    infoscr_mode = mode;

    // assign colour and rotation based on the mode
    switch (mode)
    {
        case INFOSCR_LANDSCAPE_BLACK:
            infoscr_forecolour = TFT_BLACK;
            infoscr_backcolour = TFT_WHITE;
            M5Lcd.setRotation(1);
            break;
        case INFOSCR_PORTRAIT_BLACK:
            infoscr_forecolour = TFT_BLACK;
            infoscr_backcolour = TFT_WHITE;
            M5Lcd.setRotation(0);
            break;
        case INFOSCR_PORTRAIT_WHITE:
            infoscr_forecolour = TFT_WHITE;
            infoscr_backcolour = TFT_BLACK;
            M5Lcd.setRotation(0);
            break;
        case INFOSCR_LANDSCAPE_WHITE:
        default:
            infoscr_forecolour = TFT_WHITE;
            infoscr_backcolour = TFT_BLACK;
            M5Lcd.setRotation(1);
            break;
    };

    // if a video recording is active, change everything to use a red background
    if (config_settings.tallylite != TALLYLITE_OFF && fairycam.isOperating() && fairycam.is_movierecording()) {
        infoscr_forecolour = TFT_WHITE - (TFT_BLUE / 4) - ((TFT_RED / 4) & TFT_RED);
        infoscr_backcolour = TFT_RED;
        pwr_tick(true); // keep backlight on
        if (prev_movie == false) {
            clr = true; // a full screen refresh is required
            if (config_settings.tallylite != TALLYLITE_SCREEN) {
                tallylite_ledOn();
            }
        }
        prev_movie = true;
    }
    else {
        if (prev_movie) {
            clr = true; // a full screen refresh is required
        }
        prev_movie = false;
        tallylite_ledOff();
    }

    M5Lcd.highlight(true);
    M5Lcd.setTextWrap(false);
    M5Lcd.setHighlightColor(infoscr_backcolour); // there's no frame buffer, so use the highlight function to prevent messy overlapping text
    M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);

    // clear the screen if required
    if (clr) {
        M5Lcd.fillScreen(infoscr_backcolour);
        infoscr_tickTime = millis();
    }

    // show the locked/unlocked status from quickmenu
    if (qikrmt_imuState == QIKRMTIMU_LOCKED && clr == false) {
        gui_drawTopThickLine(SUBMENU_X_OFFSET - 2, infoscr_backcolour);
    }
    else if (qikrmt_imuState != QIKRMTIMU_LOCKED) {
        gui_drawTopThickLine(SUBMENU_X_OFFSET - 2, TFT_ORANGE);
    }

    M5Lcd.setTextFont(4);
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    infoscr_itmIdx = 0;
}

void infoscr_print()
{
    uint16_t expomode = infoscr_expomode = fairycam.get_exposureMode();

    infoscr_setup(infoscr_mode, false);

    // choose layout based on exposure program mode
    if (expomode == SONYALPHA_EXPOMODE_P || expomode == SONYALPHA_EXPOMODE_MovieP || expomode == SONYALPHA_EXPOMODE_IntelligentAuto || expomode == SONYALPHA_EXPOMODE_SuperiorAuto)
    {
        infoscr_printExpoComp();
        infoscr_nextPos(true);
        infoscr_printShutterSpeed();
        infoscr_nextPos(false);
        infoscr_printAperture();
        infoscr_nextPos(false);
        infoscr_printIso();
    }
    else if (expomode == SONYALPHA_EXPOMODE_A || expomode == SONYALPHA_EXPOMODE_MovieA)
    {
        infoscr_printAperture();
        infoscr_nextPos(true);
        infoscr_printShutterSpeed();
        infoscr_nextPos(false);
        infoscr_printIso();
        infoscr_nextPos(false);
        infoscr_printExpoComp();
        
    }
    else
    {
        infoscr_printShutterSpeed();
        infoscr_nextPos(true);
        infoscr_printIso();
        infoscr_nextPos(false);
        infoscr_printAperture();
        infoscr_nextPos(false);
        infoscr_printExpoComp();
    }

    infoscr_nextPos(false);
    infoscr_printFocus();
    infoscr_nextPos(true);
    infoscr_printExpoMode(expomode);

    if (infoscr_editItem >= 0)
    {
        infoscr_printEditIndicator();
        if (infoscr_mode == INFOSCR_PORTRAIT_BLACK || infoscr_mode == INFOSCR_PORTRAIT_WHITE)
        {
            infoscr_clearRestOfLines();
        }
    }
    else
    {
        infoscr_clearRestOfLines();
    }

    gui_drawStatusBar(infoscr_backcolour == TFT_BLACK); // finalize the screen
}

void infoscr_printShutterSpeed()
{
    bool changed_font = false;

    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2); // landscape mode is a bit cramped for space, so make the label smaller
        changed_font = true;
    }

    if (infoscr_editItem == EDITITEM_SHUTTER) {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
    }

    M5Lcd.print("Tv ");

    if (infoscr_editItem == EDITITEM_SHUTTER) {
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }

    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }

    bool has_val = false;
    uint32_t x;

    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ShutterSpeed)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed);
        has_val = true;
    }
    else if (httpcam.isOperating() && (x = httpcam.get_shutterspd_32()) != 0) {
        has_val = true;
    }

    if (infoscr_editItem == EDITITEM_SHUTTER && infoscr_reqTime != 0 && (millis() - infoscr_reqTime) < INFOSCR_REQ_TIME_LIMIT)
    {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        if (has_val)
        {
            if (x != infoscr_reqShutter)
            {
                M5Lcd.setTextColor(TFT_YELLOW, infoscr_backcolour);
            }
        }
        x = infoscr_reqShutter;
        has_val = true;
    }
    else
    {
        if (infoscr_editItem == EDITITEM_SHUTTER && fairycam.isOperating()) {
            M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        }
        if (has_val) {
            infoscr_reqShutter = x;
        }
    }

    if (has_val)
    {
        gui_showVal(x, TXTFMT_SHUTTER, &M5Lcd);
    }

    M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);

    if (has_val == false)
    {
        M5Lcd.print("?");
        infoscr_reqShutter = -1;
    }
}

void infoscr_printAperture()
{
    bool changed_font = false;
    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2);
        changed_font = true;
    }

    if (infoscr_editItem == EDITITEM_APERTURE) {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
    }

    M5Lcd.print("Av ");

    if (infoscr_editItem == EDITITEM_APERTURE) {
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }

    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }

    uint32_t x = 0;
    float fx;

    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_Aperture)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_Aperture);
    }
    else if (httpcam.isOperating()) {
        x = (uint32_t)lround(100.0f * atof(httpcam.get_aperture_str()));
        uint32_t xr = x % 10;
        if (xr < 5) {
            x -= xr;
        }
        else if (xr >= 5) {
            x += (10 - xr);
        }
    }

    if (infoscr_editItem == EDITITEM_APERTURE && infoscr_reqTime != 0 && (millis() - infoscr_reqTime) < INFOSCR_REQ_TIME_LIMIT)
    {
        if (infoscr_reqAperture != x)
        {
            M5Lcd.setTextColor(TFT_YELLOW, infoscr_backcolour);
            x = infoscr_reqAperture;
        }
        else
        {
            M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        }
    }
    else
    {
        if (infoscr_editItem == EDITITEM_APERTURE && fairycam.isOperating()) {
            M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        }
    }

    if (x != 0)
    {
        infoscr_reqAperture = x;
        fx = x;
        M5Lcd.printf("f/%0.1f", fx / 100.0f);
    }

    M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);

    if (x == 0)
    {
        M5Lcd.print("?");
        infoscr_reqAperture = -1;
    }
}

void infoscr_printIso()
{
    bool changed_font = false;
    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2);
        changed_font = true;
    }

    if (infoscr_editItem == EDITITEM_ISO) {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
    }

    M5Lcd.print("ISO");

    if (infoscr_editItem == EDITITEM_ISO) {
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }

    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }

    bool has_val = false;
    uint32_t x;

    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ISO)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_ISO);
        has_val = true;
    }
    else if (httpcam.isOperating()) {
        char* iso_str = httpcam.get_iso_str();
        if (iso_str[0] < '0' || iso_str[0] > '9') {
            M5Lcd.print(" ");
            x = 0;
        }
        else
        {
            x = atoi(iso_str);
        }
        has_val = true;
    }

    if (infoscr_editItem == EDITITEM_ISO && infoscr_reqTime != 0 && (millis() - infoscr_reqTime) < INFOSCR_REQ_TIME_LIMIT)
    {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        if (has_val && x != infoscr_reqIso) {
            M5Lcd.setTextColor(TFT_YELLOW, infoscr_backcolour);
        }
        x = infoscr_reqIso;
        has_val = true;
    }
    else
    {
        if (infoscr_editItem == EDITITEM_ISO && fairycam.isOperating()) {
            M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        }
        if (has_val) {
            infoscr_reqIso = x;
        }
    }

    if (has_val)
    {
        // the word "auto" I wanted to format in a specific way here
        if (x != 0 && x != 0xFFFFFF) {
            gui_showVal(x, TXTFMT_ISO, &M5Lcd);
        }
        else {
            M5Lcd.print(" Auto");
            x = 0;
        }
        infoscr_reqIso = x;
    }

    M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);

    if (has_val == false)
    {
        M5Lcd.print(" ?");
    }
}

void infoscr_printExpoComp()
{
    bool changed_font = false;
    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2);
        changed_font = true;
    }

    if (infoscr_editItem == EDITITEM_EXPOCOMP) {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
    }

    M5Lcd.print("Ev ");

    if (infoscr_editItem == EDITITEM_EXPOCOMP) {
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }

    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }

    bool has_val = false;

    int32_t x;
    float fx;

    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ExpoComp)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_ExpoComp);
        has_val = true;
    }
    else if (httpcam.isOperating()) {
        x = httpcam.get_expocomp();
        has_val = true;
    }

    if (infoscr_editItem == EDITITEM_EXPOCOMP && infoscr_reqTime != 0 && (millis() - infoscr_reqTime) < INFOSCR_REQ_TIME_LIMIT)
    {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        if (has_val && x != infoscr_reqExpoComp) {
            M5Lcd.setTextColor(TFT_YELLOW, infoscr_backcolour);
        }
        x = infoscr_reqExpoComp;
        has_val = true;
    }
    else
    {
        if (infoscr_editItem == EDITITEM_EXPOCOMP && fairycam.isOperating()) {
            M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
        }
        if (has_val) {
            infoscr_reqExpoComp = x;
        }
    }

    if (has_val == false)
    {
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
        M5Lcd.print("?");
        return;
    }

    fx = x;
    if (x >= 0) {
        M5Lcd.print("+");
    }
    M5Lcd.printf("%0.1f", fx / 1000.0f);
    M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
}

void infoscr_printExpoMode(uint16_t expomode)
{
    static bool was_rec = false;
    int16_t old_y = M5Lcd.getCursorY();
    int16_t next_y = old_y; // calculate the next line so the other line clearing functions work correctly
    if (infoscr_mode == INFOSCR_PORTRAIT_WHITE || infoscr_mode == INFOSCR_PORTRAIT_BLACK) {
        // in portrait mode, the top right corner will not have room, so put this in the bottom right corner
        M5Lcd.setCursor(M5Lcd.width() - INFOSCR_CORNER_MARGIN, old_y);
        next_y += M5Lcd.fontHeight() + INFOSCR_LINESPACE;
    }
    else {
        // in landscape mode, put this in the top right corner
        M5Lcd.setCursor(M5Lcd.width() - INFOSCR_CORNER_MARGIN, SUBMENU_Y_OFFSET);
    }

    bool print_normally = true;

    if (fairycam.isOperating() && fairycam.is_movierecording())
    {
        if (was_rec == false)
        {
            infoscr_tickTime = millis();
            was_rec = true;
        }

        // blink the REC text if video recording
        uint32_t tspan = millis() - infoscr_tickTime;
        if ((tspan % 1500) < 750)
        {
            if (config_settings.tallylite == 0) { // if talley light is off then make the text red since the background is not red
                M5Lcd.setTextColor(TFT_RED, infoscr_backcolour);
            }
            M5Lcd.print("REC");
            if (config_settings.tallylite == 0) {
                M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
            }

            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), infoscr_backcolour); // clear current line
            M5Lcd.setCursor(SUBMENU_X_OFFSET, next_y); // since this text is placed in weird places, go to the next line so the other line clearing functions work correctly
            return;
        }
    }
    else
    {
        was_rec = false;
    }

    if (expomode == SONYALPHA_EXPOMODE_IntelligentAuto)
    {
        M5Lcd.setTextColor(infoscr_backcolour == TFT_BLACK ? TFT_GREEN : TFT_GREEN, infoscr_backcolour);
        M5Lcd.print("iA");
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }
    else if (expomode == SONYALPHA_EXPOMODE_SuperiorAuto)
    {
        M5Lcd.setTextColor(infoscr_backcolour == TFT_BLACK ? TFT_ORANGE : TFT_ORANGE, infoscr_backcolour);
        M5Lcd.print("sA");
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }
    else
    {
        if (expomode == SONYALPHA_EXPOMODE_M || expomode == SONYALPHA_EXPOMODE_MovieM) {
            M5Lcd.print("Man");
        }
        else if (expomode == SONYALPHA_EXPOMODE_P || expomode == SONYALPHA_EXPOMODE_MovieP) {
            M5Lcd.print("pA");
        }
        else if (expomode == SONYALPHA_EXPOMODE_A || expomode == SONYALPHA_EXPOMODE_MovieA) {
            M5Lcd.print("Av");
        }
        else if (expomode == SONYALPHA_EXPOMODE_S || expomode == SONYALPHA_EXPOMODE_MovieS) {
            M5Lcd.print("Tv");
        }
        else {
            if (fairycam.isOperating() == false) {
                M5Lcd.print("?");
            }
        }
    }

    M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), infoscr_backcolour); // clear current line
    M5Lcd.setCursor(SUBMENU_X_OFFSET, next_y); // since this text is placed in weird places, go to the next line so the other line clearing functions work correctly
}

void infoscr_printFocus()
{
    bool diff_colour = false;
    if (fairycam.isOperating() == false) {
        M5Lcd.print("F?");
        return;
    }
    if (fairycam.is_manuallyfocused())
    {
        M5Lcd.print("MF");
        // if possible, print the focus plane distance (given as a percentage)
        if (ptpcam.isOperating() && ptpcam.is_manuallyfocused() && ptpcam.has_property(SONYALPHA_PROPCODE_ManualFocusDist))
        {
            M5Lcd.printf(" %d %%", ptpcam.get_property(SONYALPHA_PROPCODE_ManualFocusDist));
        }
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
                // change the colour of the AF text if we can determine the AF status
                int32_t focus_status = ptpcam.get_property(SONYALPHA_PROPCODE_FocusFound);
                if (focus_status == SONYALPHA_FOCUSSTATUS_FOCUSED)
                {
                    M5Lcd.setHighlightColor(TFT_GREEN);
                    M5Lcd.setTextColor(TFT_WHITE, TFT_GREEN);
                    diff_colour = true;
                }
                else if (focus_status == SONYALPHA_FOCUSSTATUS_HUNTING)
                {
                    M5Lcd.setHighlightColor(infoscr_backcolour);
                    M5Lcd.setTextColor(infoscr_backcolour == TFT_BLACK ? TFT_GREEN : TFT_GREEN, infoscr_backcolour);
                    diff_colour = true;
                }
                else if (focus_status == SONYALPHA_FOCUSSTATUS_FAILED || focus_status == SONYALPHA_FOCUSSTATUS_FAILED)
                {
                    M5Lcd.setHighlightColor(infoscr_backcolour);
                    M5Lcd.setTextColor(TFT_RED, infoscr_backcolour);
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
        }
    }
    else if (httpcam.isOperating())
    {
        bool is_focused = httpcam.is_focused;
        if (is_focused) {
            M5Lcd.setHighlightColor(TFT_GREEN);
            M5Lcd.setTextColor(TFT_WHITE, TFT_GREEN);
            diff_colour = true;
        }
        M5Lcd.print(httpcam.get_str_afmode());
    }

    // restore the text colour if it was changed by AF status
    if (diff_colour) {
        M5Lcd.setHighlightColor(infoscr_backcolour);
        M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    }
}

void infoscr_blankLine(bool force_new_line)
{
    if (M5Lcd.getCursorX() >= M5Lcd.width()) {
        return; // nothing to clear
    }

    // clear the rest of a line, or rest of a segment
    // this function calculates where to end the clearing rectangle based on current X cursor position

    if (force_new_line)
    {
        if ((M5Lcd.getCursorY() - SUBMENU_Y_OFFSET) < M5Lcd.fontHeight() && (infoscr_mode == INFOSCR_LANDSCAPE_BLACK || infoscr_mode == INFOSCR_LANDSCAPE_WHITE)) {
            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX() - INFOSCR_CORNER_MARGIN, M5Lcd.fontHeight(), infoscr_backcolour);
        }
        else {
            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), infoscr_backcolour);
        }
    }
    else if (infoscr_mode == INFOSCR_PORTRAIT_WHITE || infoscr_mode == INFOSCR_PORTRAIT_BLACK)
    {
        if (M5Lcd.getCursorY() - SUBMENU_Y_OFFSET < M5Lcd.fontHeight()) {
            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX() - INFOSCR_CORNER_MARGIN, M5Lcd.fontHeight(), infoscr_backcolour);
        }
        else {
            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), infoscr_backcolour);
        }
    }
    else
    {
        if (M5Lcd.getCursorX() < ((M5Lcd.width() / 2) - INFOSCR_MIDDIV_MARGIN))
        {
            M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), (M5Lcd.width() / 2) - M5Lcd.getCursorX(), M5Lcd.fontHeight(), infoscr_backcolour);
        }
        else
        {
            if ((M5Lcd.getCursorY() - SUBMENU_Y_OFFSET) < M5Lcd.fontHeight() && (infoscr_mode == INFOSCR_LANDSCAPE_BLACK || infoscr_mode == INFOSCR_LANDSCAPE_WHITE)) {
                M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX() - INFOSCR_CORNER_MARGIN, M5Lcd.fontHeight(), infoscr_backcolour);
            }
            else {
                M5Lcd.fillRect(M5Lcd.getCursorX(), M5Lcd.getCursorY(), M5Lcd.width() - M5Lcd.getCursorX(), M5Lcd.fontHeight(), infoscr_backcolour);
            }
        }
    }
}

void infoscr_nextPos(bool force_new_line)
{
    infoscr_blankLine(force_new_line);

    if ((infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) && (infoscr_itmIdx % 2) == 0 && M5Lcd.getCursorX() >= ((M5Lcd.width() / 2) - INFOSCR_MIDDIV_MARGIN)) // if exceeded allotted space
    {
        infoscr_itmIdx++; // skip a space (infoscr_blankLine has detected this already)
    }

    int ny;
    infoscr_itmIdx++; // go to next space
    if (infoscr_mode == INFOSCR_PORTRAIT_WHITE || infoscr_mode == INFOSCR_PORTRAIT_BLACK)
    {
        // portrait mode is always new line
        ny = M5Lcd.getCursorY() + M5Lcd.fontHeight() + INFOSCR_LINESPACE;
        M5Lcd.setCursor(SUBMENU_X_OFFSET, ny);
    }
    else
    {
        if ((infoscr_itmIdx % 2) == 0 || force_new_line) // this should be a new line
        {
            ny = M5Lcd.getCursorY() + M5Lcd.fontHeight() + INFOSCR_LINESPACE;
            M5Lcd.setCursor(SUBMENU_X_OFFSET, ny);
            if (force_new_line && (infoscr_itmIdx % 2) != 0) {
                infoscr_itmIdx++; // if forcing but ending up in the middle, then skip a space
            }
        }
        else // this should start from the middle
        {
            M5Lcd.setCursor(M5Lcd.width() / 2, M5Lcd.getCursorY());
        }
    }
}

void infoscr_clearRestOfLines()
{
    // clear the rest of the screen by simply covering up the screen until the status bar
    int16_t y = M5Lcd.getCursorY();
    M5Lcd.fillRect(0, y, M5Lcd.width(), M5Lcd.height() - y - 14, infoscr_backcolour);
}

void infoscr_changeVal(int8_t tilt)
{
    if (tilt == 0 || fairycam.isOperating() == false) {
        return;
    }
    int cur_idx, next_idx;
    uint32_t x;
    int32_t sx;
    switch (infoscr_editItem)
    {
        case EDITITEM_SHUTTER:
            cur_idx = fairycam.getIdx_shutter(infoscr_reqShutter);
            if (cur_idx < 0) {
                break;
            }
            next_idx = cur_idx + (tilt > 0 ? 1 : -1);
            x = fairycam.getVal_shutter(next_idx);
            fairycam.cmd_ShutterSpeedSet(x);
            infoscr_reqShutter = x;
            infoscr_reqTime = millis();
            break;
        case EDITITEM_APERTURE:
            cur_idx = fairycam.getIdx_aperture(infoscr_reqAperture);
            if (cur_idx < 0) {
                break;
            }
            next_idx = cur_idx + (tilt > 0 ? 1 : -1);
            x = fairycam.getVal_aperture(next_idx);
            fairycam.cmd_ApertureSet(x);
            infoscr_reqAperture = x;
            infoscr_reqTime = millis();
            break;
        case EDITITEM_ISO:
            cur_idx = fairycam.getIdx_iso(infoscr_reqIso);
            if (cur_idx < 0) {
                break;
            }
            next_idx = cur_idx + (tilt > 0 ? 1 : -1);
            x = fairycam.getVal_iso(next_idx);
            fairycam.cmd_IsoSet(x);
            infoscr_reqIso = x;
            infoscr_reqTime = millis();
            break;
        case EDITITEM_EXPOCOMP:
            cur_idx = fairycam.getIdx_expoComp(infoscr_reqExpoComp);
            if (cur_idx < 0) {
                break;
            }
            next_idx = cur_idx + (tilt > 0 ? 1 : -1);
            sx = fairycam.getVal_expoComp(next_idx);
            fairycam.cmd_ExpoCompSet(sx);
            infoscr_reqExpoComp = sx;
            infoscr_reqTime = millis();
            break;
    }
}

void infoscr_printEditIndicator()
{
    int tilt = imu.getTilt();

    // center the text
    M5Lcd.print("    ");

    if (tilt < 0) {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
    }
    else {
        M5Lcd.setTextColor(TFT_DARKGREY, infoscr_backcolour);
    }
    M5Lcd.print("<<- ");

    M5Lcd.setTextColor(TFT_DARKGREY, infoscr_backcolour);
    M5Lcd.print("EDITING");

    if (tilt > 0) {
        M5Lcd.setTextColor(TFT_GREEN, infoscr_backcolour);
    }
    else {
        M5Lcd.setTextColor(TFT_DARKGREY, infoscr_backcolour);
    }
    M5Lcd.print(" +>>");

    M5Lcd.setTextFont(4);
    M5Lcd.setTextColor(infoscr_forecolour, infoscr_backcolour);
    M5Lcd.print(" ");
    infoscr_blankLine(true);

    if (btnBig_hasPressed())
    {
        btnBig_clrPressed();

        if (tilt == 0) {
            return;
        }

        infoscr_changeVal(tilt);
    }
}

void infoscr_startEdit()
{
    infoscr_editItem = 0; // start edit mode

    // the edit tilting makes no sense in portrait mode
    if (infoscr_mode == INFOSCR_PORTRAIT_WHITE) {
        infoscr_mode = INFOSCR_LANDSCAPE_WHITE;
    }
    if (infoscr_mode == INFOSCR_PORTRAIT_BLACK) {
        infoscr_mode = INFOSCR_LANDSCAPE_BLACK;
    }

    // do a draw before button release
    infoscr_setup(infoscr_mode, true);
    infoscr_print();

    app_waitAllRelease();
    while (true)
    {
        if (app_poll() == false) {
            continue;
        }

        if (redraw_flag) {
            infoscr_setup(infoscr_mode, true);
        }

        infoscr_print();

        // big button is handled in infoscr_printEditIndicator(), which is inside infoscr_print()

        if (btnSide_hasPressed())
        {
            btnSide_clrPressed();

            infoscr_editItem++;
            while (true)
            {
                if (infoscr_editItem == EDITITEM_APERTURE && fairycam.isOperating() && (infoscr_expomode != SONYALPHA_EXPOMODE_M && infoscr_expomode != SONYALPHA_EXPOMODE_MovieM && infoscr_expomode != SONYALPHA_EXPOMODE_A && infoscr_expomode != SONYALPHA_EXPOMODE_MovieA)) {
                    infoscr_editItem++;
                }
                if (infoscr_editItem == EDITITEM_SHUTTER && fairycam.isOperating() && (infoscr_expomode != SONYALPHA_EXPOMODE_M && infoscr_expomode != SONYALPHA_EXPOMODE_MovieM && infoscr_expomode != SONYALPHA_EXPOMODE_S && infoscr_expomode != SONYALPHA_EXPOMODE_MovieS)) {
                    infoscr_editItem++;
                }
                if (infoscr_editItem >= EDITITEM_END) {
                    infoscr_editItem = 0;
                    continue;
                }
                else {
                    break;
                }
            }
        }

        if (btnPwr_hasPressed())
        {
            // btnPwr_clrPressed(); // do not clear, quit all layers
            break;
        }

        // exit if disconnected
        if (fairycam.isOperating() == false) {
            break;
        }

        pwr_sleepCheck();
        redraw_flag = false;
    }

    infoscr_editItem = -1; // disable edit mode
}