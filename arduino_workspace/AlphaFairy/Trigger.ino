#include "AlphaFairy.h"
#include "FairyMenu.h"

int32_t trigger_source = TRIGSRC_MIC;
int32_t trigger_action = TRIGACT_PHOTO;

extern volatile bool mictrig_hasTriggered;
extern bool gui_microphoneActive;
extern bool tallylite_enable;

int exinput_bar    = 0;
int exinput_thresh = 0;

float imutrig_prev_accelX;
float imutrig_prev_accelY;
float imutrig_prev_accelZ;

int imutrig_accelBar    = 0;
int imutrig_accelThresh = 0;
int imutrig_gyroBar     = 0;
int imutrig_gyroThresh  = 0;

#define IMUTRIG_BAR_DIV 15
int imutrig_accelBarPeak = 0;
int imutrig_gyroBarPeak  = 0;

bool extinput_poll()
{
    int gpio = get_pinCfgGpio(config_settings.pin_exinput);
    if (gpio < 0) {
        exinput_bar    = 0;
        return false;
    }
    int x = analogRead(gpio);
    if (config_settings.trigger_edge != 0) { // invert the sensor input
        x = 4095 - x;
    }

    exinput_bar    = map(x                               , 0, 4095, 10, 170);
    exinput_thresh = map(config_settings.trigger_siglevel, 0,  100, 10, 170);

    return (exinput_bar >= exinput_thresh);
}

bool imutrig_poll()
{
    float dAX = imu.accX - imutrig_prev_accelX;
    float dAY = imu.accY - imutrig_prev_accelY;
    float dAZ = imu.accZ - imutrig_prev_accelZ;

    // check for motion via accelerometer
    // remember that 1G is always present due to gravity, we compare against 1G later
    // the code will also compare the previous vector and the current vector, to determine lateral movement perpendicular to gravity

    int accelMag  = lround(sqrt((imu.accX * imu.accX) + (imu.accY * imu.accY) + (imu.accZ * imu.accZ)) * 100);
    int accelDMag = lround(sqrt((dAX * dAX) + (dAY * dAY) + (dAZ * dAZ)) * 100);
    int accelGMag = accelMag > 100 ? (accelMag - 100) : (100 - accelMag); // absolute magnitude will always be around 1G while not in motion, due to gravity
    int accelMaxMag = accelGMag;
    accelMaxMag = max(accelDMag, accelGMag);

    imutrig_prev_accelX = imu.accX;
    imutrig_prev_accelY = imu.accY;
    imutrig_prev_accelZ = imu.accZ;

    // check for motion via gyroscope spin
    // simply find the axis with highest spin
    // note: AHRS algorithm cannot be used for this as the values don't seem to be continuous

    int absGyroX = abs(lround(imu.gyroX));
    int absGyroY = abs(lround(imu.gyroY));
    int absGyroZ = abs(lround(imu.gyroZ));
    int gyroMax = 0;
    gyroMax = max(gyroMax, absGyroX);
    gyroMax = max(gyroMax, absGyroY);
    gyroMax = max(gyroMax, absGyroZ);

    // note: the accel range is 0G to 4G and gyro range is 0DPS to 500DPS, but we add a bit of overhead just so the user can disable a particular option

    imutrig_accelBar    = map(accelGMag                       , 0, 500, 10, 170);
    imutrig_accelThresh = map(config_settings.trigger_imuaccel, 0, 500, 10, 170);
    imutrig_gyroBar     = map(gyroMax                         , 0, 600, 10, 170);
    imutrig_gyroThresh  = map(config_settings.trigger_imurot  , 0, 600, 10, 170);

    // show a decaying peak, because it's hard to read the screen while it is moving

    if (imutrig_accelBar > (imutrig_accelBarPeak / IMUTRIG_BAR_DIV)) {
        imutrig_accelBarPeak = imutrig_accelBar * IMUTRIG_BAR_DIV;
    }
    else {
        imutrig_accelBarPeak -= 1;
    }
    if (imutrig_gyroBar > (imutrig_gyroBarPeak / IMUTRIG_BAR_DIV)) {
        imutrig_gyroBarPeak = imutrig_gyroBar * IMUTRIG_BAR_DIV;
    }
    else {
        imutrig_gyroBarPeak -= 1;
    }

    return (imutrig_accelBar >= imutrig_accelThresh) || (imutrig_gyroBar >= imutrig_gyroThresh);
}

void extinput_drawLevel()
{
    gui_drawLevelBar(exinput_bar, -1, exinput_thresh, -1);
}

void imutrig_drawLevel()
{
    gui_drawLevelBar(imutrig_accelBarPeak / IMUTRIG_BAR_DIV, imutrig_gyroBarPeak / IMUTRIG_BAR_DIV, imutrig_accelThresh, imutrig_gyroThresh);
}

#ifdef ENABLE_BUILD_LEPTON
extern int lepton_trigBar;
extern int lepton_trigThresh;
void lepton_drawLevel()
{
    gui_drawLevelBar(lepton_trigBar, -1, lepton_trigThresh, -1);
}
#endif

void trigger_drawLevel()
{
    if (trigger_source == TRIGSRC_ALL) {
        gui_drawLevelBar(-1, -1, -1, -1); // draws a black region
    }
    else if (trigger_source == TRIGSRC_MIC) {
        mictrig_drawLevel();
    }
    else if (trigger_source == TRIGSRC_EXINPUT) {
        extinput_drawLevel();
    }
    else if (trigger_source == TRIGSRC_IMU) {
        imutrig_drawLevel();
    }
    #ifdef ENABLE_BUILD_LEPTON
    else if (trigger_source == TRIGSRC_THERMAL) {
        lepton_drawLevel();
    }
    #endif
    else {
        gui_drawLevelBar(-1, -1, -1, -1); // draws a black region
    }
}

bool trigger_all_poll()
{
    bool triggered = false;
    if (trigger_source == TRIGSRC_MIC || trigger_source == TRIGSRC_ALL) {
        mictrig_poll();
        triggered |= mictrig_hasTriggered;
    }
    if (trigger_source == TRIGSRC_EXINPUT || trigger_source == TRIGSRC_ALL) {
        triggered |= extinput_poll();
    }
    if (trigger_source == TRIGSRC_IMU || trigger_source == TRIGSRC_ALL) {
        triggered |= imutrig_poll();
    }
    #ifdef ENABLE_BUILD_LEPTON
    #ifdef ENABLE_BUILD_LEPTON_TRIGGER_COMPLEX
    if (trigger_source == TRIGSRC_THERMAL || trigger_source == TRIGSRC_ALL)
    {
        lepton_poll(true);
        triggered |= lepton_checkTrigger();
    }
    #endif
    #endif
    return triggered;
}

void trigger_drawActionIcon(int16_t y)
{
    int16_t w = GENERAL_ICON_WIDTH;
    int16_t x = M5Lcd.width() - w;
    if (trigger_action == TRIGACT_PHOTO)
    {
        M5Lcd.drawPngFile(SPIFFS, "/camera_icon.png", x, y);
    }
    else if (trigger_action == TRIGACT_VIDEO)
    {
        M5Lcd.drawPngFile(SPIFFS, "/vid_icon.png", x, y);
    }
    else if (trigger_action == TRIGACT_INTERVAL)
    {
        M5Lcd.drawPngFile(SPIFFS, "/intervalometer_icon.png", x, y);
    }
    else
    {
        M5Lcd.fillRect(x, y, w, w, TFT_BLACK);
    }
}

class PageTrigger : public FairyCfgItem
{
    public:
        PageTrigger(const char* disp_name, int32_t* linked_var, int32_t val_min, int32_t val_max, int32_t step_size, uint32_t fmt_flags) : FairyCfgItem(disp_name, linked_var, val_min, val_max, step_size, fmt_flags)
        { this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = true; };
        PageTrigger(const char* disp_name, bool (*cb)(void*), const char* icon) : FairyCfgItem(disp_name, cb, icon)
        { this->_margin_y = MICTRIG_LEVEL_MARGIN; this->_autosave = false; };

        virtual void on_drawLive (void)
        {
            trigger_drawLevel();
        };

        virtual void on_extraPoll(void) {
            trigger_all_poll();
        };
};

class PageTriggerSource : public PageTrigger
{
    public:
        PageTriggerSource() : PageTrigger("Trigger Source", (int32_t*)&(trigger_source), 0,
        #if defined(ENABLE_BUILD_LEPTON) && defined(ENABLE_BUILD_LEPTON_TRIGGER_COMPLEX)
            4
        #else
            3
        #endif
            , 1, TXTFMT_TRIGSRC)
        {
            this->_autosave = false;
        };

        virtual void on_readjust(void)
        {
            draw_icon();
        };
};

class PageTriggerAction : public PageTrigger
{
    public:
        PageTriggerAction() : PageTrigger("Trigger Action", (int32_t*)&(trigger_action), 0, 2, 1, TXTFMT_TRIGACT)
        {
            this->_autosave = false;
        };

        virtual void on_readjust(void)
        {
            draw_val_icon();
        };

        virtual void draw_icon(void)
        {
            FairyCfgItem::draw_icon();
            draw_val_icon();
        };

    protected:
        void draw_val_icon(void)
        {
            trigger_drawActionIcon(0);
        };
};

class PageSoundTriggerLevel : public PageTrigger
{
    public:
        PageSoundTriggerLevel() : PageTrigger("Sound Trig Level", (int32_t*)&(config_settings.mictrig_level), 0, 100, 1, TXTFMT_NONE) {
        };

        virtual void on_drawLive (void)
        {
            mictrig_drawLevel();
        };

        virtual bool can_navTo(void)
        {
            return trigger_source == TRIGSRC_MIC || trigger_source == TRIGSRC_ALL;
        };
};

class PageTriggerSigEdge : public PageTrigger
{
    public:
        PageTriggerSigEdge() : PageTrigger("Sig Invert", (int32_t*)&(config_settings.trigger_edge), 0, 1, 1, TXTFMT_BOOL)
        {
        };

        virtual void on_drawLive (void)
        {
            extinput_drawLevel();
        };

        virtual bool can_navTo(void)
        {
            return trigger_source == TRIGSRC_EXINPUT || trigger_source == TRIGSRC_ALL;
        };
};

class PageTriggerSigLevel : public PageTrigger
{
    public:
        PageTriggerSigLevel() : PageTrigger("Sig Level", (int32_t*)&(config_settings.trigger_siglevel), 0, 100, 1, TXTFMT_NONE)
        {
        };

        virtual void on_drawLive (void)
        {
            extinput_drawLevel();
        };

        virtual bool can_navTo(void)
        {
            return trigger_source == TRIGSRC_EXINPUT || trigger_source == TRIGSRC_ALL;
        };
};

class PageTriggerImuAccel : public PageTrigger
{
    public:
        PageTriggerImuAccel() : PageTrigger("IMU Accel Level", (int32_t*)&(config_settings.trigger_imuaccel), 0, 500, 1, TXTFMT_BYTENS | TXTFMT_DIVHUNDRED) // note: the accel range is 0G to 4G and gyro range is 0DPS to 500DPS, but we add a bit of overhead just so the user can disable a particular option
        {
        };

        virtual void on_drawLive (void)
        {
            imutrig_drawLevel();
        };

        virtual bool can_navTo(void)
        {
            return trigger_source == TRIGSRC_IMU || trigger_source == TRIGSRC_ALL;
        };

        virtual void on_navTo(void)
        {
            imutrig_accelBarPeak = 0;
        };
};

class PageTriggerImuRot : public PageTrigger
{
    public:
        PageTriggerImuRot() : PageTrigger("IMU Rot Level", (int32_t*)&(config_settings.trigger_imurot), 0, 600, 1, TXTFMT_BYTENS) // note: the accel range is 0G to 4G and gyro range is 0DPS to 500DPS, but we add a bit of overhead just so the user can disable a particular option
        {
        };

        virtual void on_drawLive (void)
        {
            imutrig_drawLevel();
        };

        virtual bool can_navTo(void)
        {
            return trigger_source == TRIGSRC_IMU || trigger_source == TRIGSRC_ALL;
        };
};

class PageTriggerArmDelay : public PageTrigger
{
    public:
        PageTriggerArmDelay() : PageTrigger("Arm Delay", (int32_t*)&(config_settings.trigger_armtime), 3, 10000, 1, TXTFMT_TIME)
        {
        };
};

class PageTriggerDelay : public PageTrigger
{
    public:
        PageTriggerDelay() : PageTrigger("Start Delay", (int32_t*)&(config_settings.trigger_delay), 0, 1000, 1, TXTFMT_TIME)
        {
        };
};

class PageTriggerRetrigger : public PageTrigger
{
    public:
        PageTriggerRetrigger() : PageTrigger("Retrigger", (int32_t*)&(config_settings.trigger_retrigger), -1, 10000, 1, TXTFMT_TIME | TXTFMT_NEGOFF)
        {
        };

        virtual bool can_navTo(void)
        {
            return trigger_action != TRIGACT_INTERVAL;
        };
};

class PageTriggerVideoTime : public PageTrigger
{
    public:
        PageTriggerVideoTime() : PageTrigger("Video Length", (int32_t*)&(config_settings.trigger_vidtime), 0, 10000, 1, TXTFMT_TIME | TXTFMT_ZEROINF)
        {
        };

        virtual bool can_navTo(void)
        {
            return trigger_action == TRIGACT_VIDEO;
        };
};

// never actually called
bool trigger_nullfunc(void* ptr)
{
    return false;
}

class PageTriggerArm : public PageTrigger
{
    public:
        PageTriggerArm() : PageTrigger("Arm?", trigger_nullfunc, "/go_icon.png") {
        };

        virtual void on_redraw(void)
        {
            PageTrigger::on_redraw();
            M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5Lcd.setTextFont(4);
            _linenum = 1;
            draw_info();
            M5Lcd.setCursor(_margin_x, get_y(_linenum));
            if (config_settings.trigger_delay <= 0 && config_settings.trigger_armtime > 0)
            {
                M5Lcd.print("Arm Dly: ");
                M5Lcd.print(config_settings.trigger_armtime, DEC);
                M5Lcd.print("s");
            }
            else if (config_settings.trigger_delay > 0 && config_settings.trigger_armtime <= 0)
            {
                M5Lcd.print("Delay: ");
                M5Lcd.print(config_settings.trigger_delay, DEC);
                M5Lcd.print("s");
            }
            else
            {
                M5Lcd.print("T: ");
                M5Lcd.print(config_settings.trigger_armtime, DEC);
                M5Lcd.print(" -> ");
                M5Lcd.print(config_settings.trigger_delay, DEC);
            }
            blank_line();
        };

        virtual bool on_execute(void)
        {
            if (fairycam.isOperating() == false) {
                pwr_airplaneModeEnter();
            }

            app_waitAllRelease();
            execute();
            sprites->unload_all();
            on_redraw();
            app_waitAllRelease();
            return false;
        };

    protected:

        int _linenum = 0;

        void draw_parent_icon(void)
        {
            FairyCfgApp* p = dynamic_cast<FairyCfgApp*>((FairyCfgApp*)get_parent());
            if (p != NULL)
            {
                p->draw_icon();
            }
        };

        void execute(void)
        {
            gui_microphoneActive = true;

            int shot_cnt = 0;
            uint32_t t = 0;

            M5Lcd.fillScreen(TFT_BLACK);

            sprites->unload_all();

            // enforce minimum arming delay, gives the user a chance to run away, also the button seems to trigger the mic when pressed
            config_settings.trigger_armtime = (config_settings.trigger_armtime < 3) ? 3 : config_settings.trigger_armtime;

            if (intervalometer_wait(config_settings.trigger_armtime, millis(), 0, "Arming Dly...", false, 0))
            {
                return; // user quit
            }
            pwr_tick(true);

            do // this loop is for auto-retriggering
            {
                app_poll();
                pwr_tick(false);

                // put some indicator on the screen
                M5Lcd.fillScreen(TFT_BLACK);
                gui_startAppPrint();
                M5Lcd.setTextColor(TFT_RED, TFT_BLACK);
                M5Lcd.setTextFont(4);
                _linenum = 0;
                M5Lcd.setCursor(_margin_x, get_y(_linenum));
                M5Lcd.print("!!!ARMED!!!");
                _linenum = 1;
                if (shot_cnt <= 0)
                {
                    // first run, show info
                    draw_info();
                }
                else
                {
                    // we have some shots already, show the counter
                    M5Lcd.setCursor(_margin_x, get_y(_linenum));
                    M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    M5Lcd.printf("CNT = %u", shot_cnt);
                }

                // draw some icons so the screen doesn't look boring
                draw_parent_icon();
                trigger_drawActionIcon(0);

                int dot_idx = 0;

                mictrig_hasTriggered = false;
                bool triggered = false;
                while (triggered == false)
                {
                    app_poll();
                    pwr_tick(false);

                    triggered |= trigger_all_poll();

                    // quit on button press
                    if (btnSide_hasPressed())
                    {
                        btnSide_clrPressed();
                        return;
                    }
                    if (btnPwr_hasPressed())
                    {
                        btnPwr_clrPressed();
                        return;
                    }

                    // intentional trigger
                    if (btnBig_hasPressed())
                    {
                        btnBig_clrPressed();
                        triggered |= true;
                    }

                    // just so the screen doesn't look boring
                    trigger_drawLevel();
                    gui_drawStatusBar(true);

                    // animate dots just to show we haven't frozen
                    if ((millis() - t) >= 200 || t == 0)
                    {
                        gui_drawVerticalDots(50, 40, 5, 5, 5, dot_idx++, true, TFT_GREEN, TFT_DARKGREEN);
                        t = millis();
                    }
                }

                // triggered == true here

                M5Lcd.fillScreen(TFT_BLACK);
                pwr_tick(true);

                if (config_settings.trigger_delay > 0)
                {
                    // wait if needed
                    if (intervalometer_wait(config_settings.trigger_armtime, millis(), 0, "Trig'd Wait...", false, 0))
                    {
                        return; // user quit
                    }
                    pwr_tick(true);
                }

                t = millis();

                M5Lcd.fillScreen(TFT_BLACK);
                M5Lcd.setTextColor(TFT_RED, TFT_BLACK);
                M5Lcd.setTextFont(4);
                _linenum = 0;
                M5Lcd.setCursor(_margin_x, get_y(_linenum));

                // do the action when triggered

                if (trigger_action == TRIGACT_PHOTO)
                {
                    M5Lcd.print("SHOOT");
                    cam_shootQuick();
                }
                else if (trigger_action == TRIGACT_VIDEO)
                {
                    if (config_settings.trigger_vidtime <= 0) {
                        M5Lcd.print("FILM");
                    }
                    cam_videoStart();
                    t = millis();
                    bool vid_quit = false;

                    if (config_settings.trigger_vidtime > 0)
                    {
                        if (intervalometer_wait(config_settings.trigger_vidtime, t, 0, "Vid Rec...", false, 0))
                        {
                            vid_quit = true; // user quit
                        }
                        pwr_tick(true);
                        cam_videoStop();
                        t = millis();
                    }

                    if (vid_quit)
                    {
                        return;
                    }
                }
                else if (trigger_action == TRIGACT_INTERVAL)
                {
                    if (intervalometer_func(this))
                    {
                        return;
                    }
                    t = millis();
                    shot_cnt++;
                    break; // if this ends without exiting, it means the intervalometer ended due to finishing all of the shots
                }

                _linenum++;

                shot_cnt++;

                M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

                if (config_settings.trigger_retrigger > 0)
                {
                    if (intervalometer_wait(config_settings.trigger_retrigger, t, 0, "Re-arming...", false, 0))
                    {
                        return;
                    }
                    pwr_tick(true);
                }

                // quit on button press
                if (btnSide_hasPressed())
                {
                    btnSide_clrPressed();
                    return;
                }
                if (btnPwr_hasPressed())
                {
                    btnPwr_clrPressed();
                    return;
                }
            }
            while (config_settings.trigger_retrigger >= 0);

            // put some indicator on the screen that something has happened
            M5Lcd.fillScreen(TFT_BLACK);
            gui_startAppPrint();
            M5Lcd.setTextColor(TFT_RED, TFT_BLACK);
            M5Lcd.setTextFont(4);
            _linenum = 0;
            M5Lcd.setCursor(_margin_x, get_y(_linenum));
            M5Lcd.print("DONE");
            _linenum++;
            if (config_settings.trigger_retrigger >= 0 && trigger_action != TRIGACT_INTERVAL)
            {
                // show the number of triggers only if it can be more than 1
                M5Lcd.setCursor(_margin_x, get_y(_linenum));
                M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                M5Lcd.printf("CNT = %u", shot_cnt);
            }
            draw_parent_icon();

            // wait for user to acknowledge event
            while (true)
            {
                app_poll();
                pwr_tick(false);
                gui_drawStatusBar(true);

                // quit on button press
                if (btnSide_hasPressed())
                {
                    btnSide_clrPressed();
                    return;
                }
                if (btnBig_hasPressed())
                {
                    btnBig_clrPressed();
                    return;
                }
                if (btnPwr_hasPressed())
                {
                    btnPwr_clrPressed();
                    return;
                }
            }
        };

        void draw_info(void)
        {
            M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            M5Lcd.setCursor(_margin_x, get_y(_linenum));
            if (trigger_action != TRIGACT_PHOTO) {
                gui_showVal(trigger_action, TXTFMT_TRIGACT, (Print*)&M5Lcd);
                blank_line();
                _linenum++;
            }
            M5Lcd.setCursor(_margin_x, get_y(_linenum));
            if (trigger_source == TRIGSRC_MIC
                #ifndef ENABLE_BUILD_LEPTON
                    || trigger_source == TRIGSRC_EXINPUT
                #endif
                )
            {
                M5Lcd.print("Level: ");
                if (trigger_source == TRIGSRC_MIC) {
                    gui_showVal(config_settings.mictrig_level, TXTFMT_NONE, (Print*)&M5Lcd);
                }
                #ifndef ENABLE_BUILD_LEPTON
                else {
                    if (config_settings.trigger_edge != 0) {
                        M5Lcd.print("!");
                    }
                    gui_showVal(config_settings.trigger_siglevel, TXTFMT_NONE, (Print*)&M5Lcd);
                }
                #endif
                blank_line();
                _linenum++;
            }

            if (_linenum == 1)
            {
                // if nothing interesting to write, then just show the trigger source
                M5Lcd.print("Src: ");
                gui_showVal(trigger_source, TXTFMT_TRIGSRC | TXTFMT_ALLCAPS | TXTFMT_SMALL, (Print*)&M5Lcd);
                blank_line();
                _linenum++;
            }
        };
};

class AppShutterTrigger : public FairyCfgApp
{
    public:
        AppShutterTrigger() : FairyCfgApp("/shuttertrigger.png", "/trap_icon.png", MENUITEM_TRIGGER) {
            this->install(new PageTriggerSource());
            this->install(new PageTriggerAction());

            this->install(new PageTriggerArmDelay());
            this->install(new PageTriggerDelay());
            this->install(new PageTriggerRetrigger());
            this->install(new PageTriggerVideoTime());

            this->install(new PageSoundTriggerLevel());
            this->install(new PageTriggerSigEdge());
            this->install(new PageTriggerSigLevel());
            this->install(new PageTriggerImuAccel());
            this->install(new PageTriggerImuRot());

            #ifdef ENABLE_BUILD_LEPTON
            install_lepton_trigger(this);
            #endif

            this->install(new PageTriggerArm());
        };

        virtual bool on_execute(void)
        {
            mictrig_unpause();
            tallylite_enable = false;
            bool ret = FairyCfgApp::on_execute();
            tallylite_enable = true;
            mictrig_pause();
            pwr_airplaneModeExit();
            return ret;
        };

        virtual void draw_icon(void)
        {
            int16_t w = GENERAL_ICON_WIDTH;
            int16_t x = M5Lcd.width() - w, y = M5Lcd.height() - w;
            if (trigger_source == TRIGSRC_MIC)
            {
                M5Lcd.drawPngFile(SPIFFS, "/mic_icon.png", x, y);
            }
            else if (trigger_source == TRIGSRC_EXINPUT)
            {
                M5Lcd.drawPngFile(SPIFFS, "/extinput_icon.png", x, y);
            }
            else if (trigger_source == TRIGSRC_IMU)
            {
                M5Lcd.drawPngFile(SPIFFS, "/imu_icon.png", x, y);
            }
            #ifdef ENABLE_BUILD_LEPTON
            else if (trigger_source == TRIGSRC_THERMAL)
            {
                M5Lcd.drawPngFile(SPIFFS, "/lepton_icon.png", x, y);
            }
            #endif
            else
            {
                FairyCfgApp::draw_icon();
            }
        };
};

extern FairySubmenu menu_remote;
void setup_shuttertrigger()
{
    static AppShutterTrigger app;
    menu_remote.install(&app);
}
