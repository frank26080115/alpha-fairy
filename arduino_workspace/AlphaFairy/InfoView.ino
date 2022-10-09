#include "AlphaFairy.h"
#include "FairyMenu.h"

#define INFOSCR_CORNER_MARGIN 65
#define INFOSCR_MIDDIV_MARGIN 5
#define INFOSCR_LINESPACE 2

uint8_t infoscr_mode;
uint16_t infoscr_forecolour, infoscr_backcolour;
uint8_t infoscr_itmIdx = 0;
uint32_t infoscr_tickTime = 0;

extern bool tallylite_enable;
extern uint8_t qikrmt_imuState;

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
    uint16_t expomode = fairycam.get_exposureMode();

    infoscr_setup(config_settings.infoview_mode, false);

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
        infoscr_nextPos(false);
        infoscr_printFocus();
        infoscr_nextPos(true);
        infoscr_printExpoMode(expomode);
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
        infoscr_nextPos(false);
        infoscr_printFocus();
        infoscr_nextPos(true);
        infoscr_printExpoMode(expomode);
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
        infoscr_nextPos(false);
        infoscr_printFocus();
        infoscr_nextPos(true);
        infoscr_printExpoMode(expomode);
    }

    // finalize the screen
    infoscr_clearRestOfLines();
    gui_drawStatusBar(infoscr_backcolour == TFT_BLACK);
}

void infoscr_printShutterSpeed()
{
    bool changed_font = false;
    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2); // landscape mode is a bit cramped for space, so make the label smaller
        changed_font = true;
    }
    M5Lcd.print("Tv ");
    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }
    uint32_t x;
    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ShutterSpeed)) {
        gui_showVal(ptpcam.get_property(SONYALPHA_PROPCODE_ShutterSpeed), TXTFMT_SHUTTER, &M5Lcd);
    }
    else if (httpcam.isOperating() && (x = httpcam.get_shutterspd_32()) != 0) {
        gui_showVal(x, TXTFMT_SHUTTER, &M5Lcd);
    }
    else {
        M5Lcd.print("?");
    }
}

void infoscr_printAperture()
{
    bool changed_font = false;
    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2);
        changed_font = true;
    }
    M5Lcd.print("Av ");
    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }
    uint32_t x;
    float fx;
    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_Aperture)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_Aperture);
        fx = x;
        M5Lcd.printf("f/%0.1f", fx / 100.0f);
    }
    else if (httpcam.isOperating()) {
        M5Lcd.print("f/");
        M5Lcd.print(httpcam.get_str_aperture());
    }
    else {
        M5Lcd.print("?");
    }
}

void infoscr_printIso()
{
    bool changed_font = false;
    if (infoscr_mode == INFOSCR_LANDSCAPE_WHITE || infoscr_mode == INFOSCR_LANDSCAPE_BLACK) {
        M5Lcd.setTextFont(2);
        changed_font = true;
    }
    M5Lcd.print("ISO");
    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }
    uint32_t x;
    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ISO)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_ISO);

        // the word "auto" I wanted to format in a specific way here
        if (x != 0 && x != 0xFFFFFF) {
            gui_showVal(x, TXTFMT_ISO, &M5Lcd);
        }
        else {
            M5Lcd.print(" Auto");
        }
    }
    else if (httpcam.isOperating()) {
        char* iso_str = httpcam.get_iso_str();
        if (iso_str[0] < '0' || iso_str[0] > '9') {
            M5Lcd.print(" ");
        }
        M5Lcd.print(iso_str);
    }
    else {
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
    M5Lcd.print("Ev ");
    if (changed_font) {
        M5Lcd.setTextFont(4);
        changed_font = false;
    }
    int32_t x;
    float fx;
    if (ptpcam.isOperating() && ptpcam.has_property(SONYALPHA_PROPCODE_ExpoComp)) {
        x = ptpcam.get_property(SONYALPHA_PROPCODE_ExpoComp);
    }
    else if (httpcam.isOperating()) {
        x = httpcam.get_expocomp();
    }
    else {
        M5Lcd.print("?");
        return;
    }

    fx = x;
    if (x >= 0) {
        M5Lcd.print("+");
    }
    M5Lcd.printf("%0.1f", fx / 1000.0f);
}

void infoscr_printExpoMode(uint16_t expomode)
{
    static bool was_rec = false;
    int16_t old_y = M5Lcd.getCursorY();
    int16_t next_y = old_y + M5Lcd.fontHeight() + INFOSCR_LINESPACE; // calculate the next line so the other line clearing functions work correctly
    if (infoscr_mode == INFOSCR_PORTRAIT_WHITE || infoscr_mode == INFOSCR_PORTRAIT_BLACK) {
        // in portrait mode, the top right corner will not have room, so put this in the bottom right corner
        M5Lcd.setCursor(M5Lcd.width() - INFOSCR_CORNER_MARGIN, old_y);
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
