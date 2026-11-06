#include "AlphaFairy.h"
#include "FairyMenu.h"
#include <stdio.h>
#include <string.h>

static void time_menu_format_offset(int64_t deciseconds, char* text, size_t size)
{
    if (deciseconds == 0) {
        snprintf(text, size, "=");
        return;
    }
    struct Unit { uint64_t duration; const char* suffix; };
    static const Unit units[] = {
        {365ULL * 24 * 60 * 60 * 10, "Y"}, {30ULL * 24 * 60 * 60 * 10, "Mo"},
        {24ULL * 60 * 60 * 10, "D"}, {60ULL * 60 * 10, "H"}, {60 * 10, "min"}, {10, "sec"}
    };
    uint64_t magnitude = deciseconds < 0 ? (uint64_t)(-(deciseconds + 1)) + 1 : (uint64_t)deciseconds;
    const Unit* unit = &units[5];
    for (const Unit& candidate : units) {
        if (magnitude >= candidate.duration) { unit = &candidate; break; }
    }
    uint64_t count = magnitude / unit->duration + (magnitude % unit->duration != 0);
    snprintf(text, size, "%c%llu%s", deciseconds < 0 ? '-' : '+', (unsigned long long)count, unit->suffix);
}

class AppTimeMenu : public FairyMenuItem
{
    public:
        AppTimeMenu() : FairyMenuItem(SPRITE_ASSET_CLOCK) {}

        void on_navTo(void) override
        {
            selecting = false;
            time_clock_view_active(true);
            on_redraw();
        }

        void on_navOut(void) override
        {
            selecting = false;
            time_clock_view_active(false);
        }

        void on_redraw(void) override
        {
            draw_mainImage();
            draw_live(true);
            if (selecting) {
                if (selected == 0) M5Lcd.drawRect(26, 81, 29, 44, TFT_BLACK);
                else if (selected == 1) M5Lcd.drawRect(75, 81, 29, 44, TFT_BLACK);
                else M5Lcd.drawRect(8, 205, 102, 16, TFT_BLACK);
            }
        }

        void on_eachFrame(void) override
        {
            if ((uint32_t)(millis() - last_draw_ms) >= 500) draw_live(false);
        }

        bool on_execute(void) override
        {
            selecting = true;
            selected = 0;
            btnAny_clrPressed();
            on_redraw();
            while (true) {
                if (app_poll()) {
                    if (redraw_flag) { redraw_flag = false; on_redraw(); }
                    on_eachFrame();
                    draw_statusBar();
                    pwr_sleepCheck();
                }
                if (btnPwr_hasPressed()) {
                    btnPwr_clrPressed();
                    selecting = false;
                    on_redraw();
                    return false; // Stay on this menu item, consuming Back only for selection mode.
                }
                if (btnSide_hasPressed()) {
                    btnSide_clrPressed();
                    selected = (selected + 1) % 3;
                    on_redraw();
                }
                if (btnBig_hasPressed()) {
                    btnBig_clrPressed();
                    if (selected == 0) time_request_sync_camera_from_rtc();
                    else if (selected == 1) time_request_sync_rtc_from_camera();
                    else {
                        config_settings.time_auto_sync = !config_settings.time_auto_sync;
                        settings_save();
                    }
                    draw_live(true);
                }
            }
        }

    private:
        bool selecting = false;
        uint8_t selected = 0;
        uint32_t last_draw_ms = 0;
        char last_date[11] = {}, last_time[9] = {}, last_offset[24] = {};
        int last_auto_sync = -1;

        void draw_live(bool force)
        {
            last_draw_ms = millis();
            M5Lcd.setRotation(0);
            M5Lcd.setTextFont(2);
            M5Lcd.setTextSize(1);
            M5Lcd.setTextWrap(false);
            M5Lcd.highlight(false);
            M5Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
            M5Lcd.setTextDatum(MC_DATUM);

            char date[11], clock[9];
            ptpip_datetime_t rtc;
            if (time_get_rtc(&rtc, false)) {
                snprintf(date, sizeof(date), "%02u/%02u/%04u", rtc.month, rtc.day, rtc.year);
                snprintf(clock, sizeof(clock), "%02u:%02u:%02u", rtc.hour, rtc.minute, rtc.second);
            } else {
                strcpy(date, "??" "/??" "/????");
                strcpy(clock, "??:??:??");
            }
            if (force || strcmp(date, last_date) || strcmp(clock, last_time)) {
                M5Lcd.fillRect(14, 47, 108, 32, TFT_WHITE);
                M5Lcd.drawString(date, 68, 55);
                M5Lcd.drawString(clock, 68, 71);
                strcpy(last_date, date);
                strcpy(last_time, clock);
            }

            char offset[24];
            int64_t difference;
            if (time_get_camera_offset(&difference)) time_menu_format_offset(difference, offset, sizeof(offset));
            else strcpy(offset, "?????");
            if (force || strcmp(offset, last_offset)) {
                M5Lcd.fillRect(14, 171, 108, 32, TFT_WHITE);
                M5Lcd.drawString(offset, 68, 187);
                strcpy(last_offset, offset);
            }

            int enabled = config_settings.time_auto_sync != 0;
            if (force || enabled != last_auto_sync) {
                M5Lcd.fillRect(10, 207, 98, 12, TFT_WHITE);
                const sprite_asset_t* asset = spriteAsset(enabled ? SPRITE_ASSET_AUTOSYNC_YES : SPRITE_ASSET_AUTOSYNC_NO);
                if (asset) M5Lcd.drawPngData(asset->data, asset->len, 10, 207);
                last_auto_sync = enabled;
            }
            M5Lcd.setTextDatum(TL_DATUM);
        }
};

extern FairySubmenu menu_utils;
void setup_timemenu(void)
{
    static AppTimeMenu app;
    menu_utils.install(&app);
}
