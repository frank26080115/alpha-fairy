#include "AlphaFairy.h"

extern bool airplane_mode;

void remote_shutter(uint8_t time_delay, bool use_gui)
{
    uint32_t tstart, now, tdiff;
    bool quit = false;

    tstart = millis();

    cpufreq_boost();

    if (use_gui && time_delay <= 2) {
        draw_borderRect(5, TFT_RED);
    }

    if (fairycam.isOperating() == false || airplane_mode != false)
    {
        bool can_still_shoot = false;
        // no camera connected via wifi so use infrared or GPIO if possible
        if (config_settings.infrared_enabled && time_delay <= 2)
        {
            if (time_delay == 2) {
                dbg_ser.println("IR - shoot 2s");
                SonyCamIr_Shoot2S();
            }
            else {
                dbg_ser.println("IR - shoot");
                SonyCamIr_Shoot();
            }

            can_still_shoot = true;
        }

        if (config_settings.pin_shutter != PINCFG_NONE)
        {
            dbg_ser.print("GPIO rmt ");
            if (time_delay > 0)
            {
                dbg_ser.print("wait ");
                tstart = millis();
                now = tstart;
                quit = false;

                // wait the countdown time
                while (((tdiff = ((now = millis()) - tstart)) < (time_delay * 1000))) {
                    if (app_poll()) {
                        if (use_gui && time_delay > 2) {
                            gui_drawVerticalDots(0, 40, -1, 5, time_delay, tdiff / 1000, false, TFT_GREEN, TFT_RED);
                        }
                    }
                    if (time_delay > 2)
                    {
                        if (btnSide_hasPressed()) {
                            btnSide_clrPressed();
                            quit = true;
                            break;
                        }
                        if (btnPwr_hasPressed()) {
                            btnPwr_clrPressed();
                            quit = true;
                            break;
                        }
                    }
                }
            }
            if (quit == false)
            {
                dbg_ser.println("shoot");
                //cam_shootQuickGpio();
                cam_shootOpen();
                tstart = millis();
                while (((tdiff = ((now = millis()) - tstart))) < config_settings.shutter_press_time_ms || btnBig_isPressed()) {
                    app_poll();
                }
                cam_shootClose();
            }
            can_still_shoot = true;
        }

        if (can_still_shoot == false) {
            // show user that the camera isn't connected
            dbg_ser.println("remote_shutter but no camera connected");
            app_waitAllReleaseConnecting();
        }
        return;
    }

    bool starting_mf = fairycam.is_manuallyfocused() == SHCAM_FOCUSMODE_MF;
    bool starting_mf_ignore = false;
    if (httpcam.isOperating() && httpcam.is_manuallyfocused() == SHCAM_FOCUSMODE_NONE) {
        starting_mf_ignore = true;
    }

    // start focusing at the beginning of the countdown
    if (starting_mf == false && starting_mf_ignore == false) {
        dbg_ser.printf("rmtshutter AF\r\n");
        fairycam.cmd_AutoFocus(true);
    }

    now = tstart;
    quit = false;
    if (time_delay > 0)
    {
        dbg_ser.printf("rmtshutter wait delay %u... ", time_delay);
        // wait the countdown time
        while (((tdiff = ((now = millis()) - tstart)) < (time_delay * 1000)) && fairycam.isOperating()) {
            if (app_poll()) {
                cpufreq_boost();
                if (use_gui && time_delay > 2) {
                    gui_drawVerticalDots(0, 40, -1, 5, time_delay, tdiff / 1000, false, TFT_GREEN, TFT_RED);
                }
            }
            if (btnSide_hasPressed()) {
                btnSide_clrPressed();
                quit = true;
                break;
            }
            if (btnPwr_hasPressed()) {
                btnPwr_clrPressed();
                quit = true;
                break;
            }
        }

        // if user cancelled
        if (quit) {
            dbg_ser.printf(" user cancelled\r\n");
            // end autofocus
            if (starting_mf == false && starting_mf_ignore == false && fairycam.isOperating()) {
                fairycam.cmd_AutoFocus(false);
            }
            app_waitAllRelease();
            return;
        }
        else {
            dbg_ser.printf(" done\r\n");
        }
    }

    tstart = millis();
    app_poll();
    if ((ptpcam.isOperating() && ptpcam.is_focused) || starting_mf)
    {
        // if the camera is focused or MF, then the shutter should immediately take the picture
        ptpcam.cmd_Shoot(config_settings.shutter_press_time_ms);
        dbg_ser.printf("rmtshutter shoot\r\n");
    }
    else if ((httpcam.isOperating() && httpcam.is_focused) || starting_mf)
    {
        httpcam.cmd_Shoot();
        dbg_ser.printf("rmtshutter shoot\r\n");
    }
    else if (btnBig_isPressed() || time_delay == 0)
    {
        // if the camera is not focused, we try to take the shot anyways if the user is holding the button
        if (ptpcam.isOperating())
        {
            ptpcam.cmd_Shutter(true);
            dbg_ser.printf("rmtshutter shutter open\r\n");
            while ((btnBig_isPressed() || (starting_mf == false && ptpcam.is_focused == false && ((now = millis()) - tstart) < 2000)) && ptpcam.isOperating())
            {
                app_poll();
            }
            if (ptpcam.is_focused) {
                dbg_ser.printf("rmtshutter got focus\r\n");
                ptpcam.wait_while_busy(config_settings.shutter_press_time_ms, DEFAULT_BUSY_TIMEOUT, NULL);
            }
            ptpcam.cmd_Shutter(false);
            dbg_ser.printf("rmtshutter shutter close\r\n");
        }
        else if (httpcam.isOperating())
        {
            httpcam.cmd_Shoot();
        }
    }
    else if (fairycam.isOperating())
    {
        fairycam.cmd_Shoot(config_settings.shutter_press_time_ms);
        dbg_ser.printf("rmtshutter shoot\r\n");
    }

    if (starting_mf == false && starting_mf_ignore == false) {
        fairycam.cmd_AutoFocus(false);
        dbg_ser.printf("rmtshutter disable AF\r\n");
    }

    app_waitAllRelease();
}

void record_movie()
{
    if (fairycam.isOperating() == false)
    {
        if (config_settings.infrared_enabled) {
            dbg_ser.println("IR - movie");
            SonyCamIr_Movie();
            return;
        }
        else {
            dbg_ser.println("record_movie but no camera connected");
            app_waitAllReleaseConnecting();
            return;
        }
    }

    fairycam.cmd_MovieRecordToggle();
    app_waitAllRelease();
}

#include "FairyMenu.h"

class AppRemoteShutter : public FairyMenuItem
{
    public:
        AppRemoteShutter() : FairyMenuItem("/remoteshutter_d.png") {
        };

        uint8_t _delay = 2;

        void draw_delay(void)
        {
            gui_startMenuPrint();
            M5Lcd.setCursor(80, 188);
            M5Lcd.printf("%us   ", _delay);
        }

        virtual void on_redraw(void)
        {
            draw_mainImage();
            draw_delay();
        };

        virtual void on_spin(int8_t x)
        {
            if (x > 0)
            {
                if (_delay == 2) {
                    _delay = 5;
                }
                else if (_delay == 5) {
                    _delay = 10;
                }
                else {
                    _delay = 5;
                }
                draw_delay();
            }
            else if (x < 0)
            {
                if (_delay == 5) {
                    _delay = 2;
                }
                else if (_delay == 10) {
                    _delay = 5;
                }
                else {
                    _delay = 2;
                }
                draw_delay();
            }
        };

        virtual bool on_execute(void)
        {
            cpufreq_boost();
            remote_shutter(_delay, true);
            set_redraw();
            return false;
        };
};

extern FairySubmenu menu_remote;
void setup_remoteshutter()
{
    static AppRemoteShutter app;
    menu_remote.install(&app);
}
