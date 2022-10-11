#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>
#include <SpriteMgr.h>
#include "FairyMenu.h"
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <SonyHttpCamera.h>
#include "AlphaFairyCamera.h"
#include <AlphaFairy_NetMgr.h>
#include <AlphaFairyImu.h>
#include <FairyKeyboard.h>
#include <FairyEncoder.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>

#ifdef ENABLE_BUILD_LEPTON
#include <Lepton.h>
#endif

PtpIpSonyAlphaCamera ptpcam((char*)"ALPHA-FAIRY", NULL);
SonyHttpCamera       httpcam;
AlphaFairyCamera     fairycam(&ptpcam, &httpcam);

#ifdef DISABLE_ALL_MSG
DebuggingSerialDisabled
#else
DebuggingSerial
#endif
                        dbg_ser(&Serial);

uint32_t gpio_time = 0; // keeps track of the GPIO shutter activation time so it doesn't get stuck

bool airplane_mode = false;

bool redraw_flag = false; // forces menu redraw
SpriteMgr* sprites;

AlphaFairyImu imu;
FairyEncoder  fencoder;

FairySubmenu main_menu(NULL, 0);

void setup()
{
    Serial.begin(SERIAL_PORT_BAUDRATE);
    dbg_ser.enabled = true;

    Wire1.begin(21, 22);
    Wire1.setClock(400000);

    cpufreq_init();

    settings_init();
    btns_init();
    SonyCamIr_Init();

    M5.begin(false); // do not initialize the LCD, we have our own extended M5Lcd class to initialize later
    M5.IMU.Init();
    M5.IMU.SetGyroFsr(M5.IMU.GFS_500DPS);
    M5.IMU.SetAccelFsr(M5.IMU.AFS_4G);

    M5.Axp.begin();
    M5.Axp.ScreenSwitch(false); // turn off the LCD backlight while initializing, avoids junk being shown on the screen
    M5Lcd.begin(); // our own extended LCD object
    M5Lcd.fillScreen(TFT_BLACK);
    M5.Axp.ScreenBreath(config_settings.lcd_brightness);

    spiffs_init();

    #ifdef PMIC_LOG_ON_BOOT
    pmic_startCoulombCount();
    #endif

    setup_menus();

    cmdline.print_prompt();

    sprites = new SpriteMgr(&M5Lcd);

    httpcam.borrowBuffer(ptpcam.donateBuffer(), DATA_BUFFER_SIZE);

    cam_cb_setup();
    wifi_init();

    dbg_ser.printf("finished setup() at %u ms\r\n", millis());

    // clear the button flags
    btnAny_clrPressed();

    imu.poll();

    pwr_tick(true);

    #ifdef DISABLE_ALL_MSG
    dbg_ser.enabled = false;
    ptpcam.set_debugflags(0);
    #endif

    btnAny_clrPressed();

    srand(lroundf(imu.accX) + lroundf(imu.accY) + lroundf(imu.accZ));
}

void loop()
{
    main_menu.on_execute(); // this runs an internal loop, the loop calls app_poll, and app_poll will call yield, which resets the watchdog
    dbg_ser.println("main menu exited");
    // exited
    pwr_shutdown();
}

FairySubmenu menu_remote  ("/main_remote.png");
FairySubmenu menu_focus   ("/main_focus.png");
FairyCfgApp  menu_interval("/main_interval.png", "/intervalometer.png", MENUITEM_INTERVAL);
FairyCfgApp  menu_astro   ("/main_astro.png"   , "/galaxy_icon.png"   , MENUITEM_ASTRO);
FairySubmenu menu_utils   ("/main_utils.png");
FairySubmenu menu_auto    ("/main_auto.png");

void setup_menus()
{
    // install menu items
    // these calls must be in the correct order because internally the menu system uses a linked list

    // taking advantage of Arduino's automatic function prototype generation
    // each *.ino file can have its own setup_xxx function

    main_menu.install(&menu_remote);
    menu_remote.set_enc_nav(false);
    main_menu.install(&menu_focus);
    menu_focus.set_enc_nav(false);
    setup_intervalometer();
    main_menu.install(&menu_utils   );
    setup_autoconnect();
    #ifdef ENABLE_BUILD_LEPTON
    setup_leptonflir();
    #endif

    setup_qikrmt();
    setup_remoteshutter();
    setup_shuttertrigger();
    setup_dualshutter();

    setup_focusstack();
    setup_shutterstep();
    setup_focuspull();
    setup_focusfrustration();

    setup_wifimenus();
    setup_configmenu();
    setup_focuscalib();
    setup_aboutme();
}

bool app_poll()
{
    // high priority tasks
    if (airplane_mode == false)
    {
        NetMgr_task();
        ptpcam.task();
        httpcam.task();
        #ifdef HTTP_ON_BOOT
        httpsrv_poll();
        #endif
    }

    // do low priority tasks if the networking is not busy
    if (ptpcam.isKindaBusy() == false || airplane_mode != false) {
        imu.poll();
        cmdline.task();
        fenc_task();
        btnPwr_poll();
        shutterrelease_task();

        if (imu.hasMajorMotion) {
            // do not sleep if the user is moving the device
            imu.hasMajorMotion = false;
            pwr_tick(true);
        }

        pmic_log();

        yield();

        cpufreq_task();
        pwr_lightSleepEnter(); // this doesn't work yet

        return true; // can do more low priority tasks
    }
    return false; // should not do more low priority tasks
}

void shutterrelease_task()
{
    if (gpio_time != 0)
    {
        // release the GPIO after a timeout
        uint32_t telapsed = millis() - gpio_time;
        int32_t tlimit = config_settings.intv_bulb;
        tlimit = (tlimit <= 0) ? config_settings.astro_bulb : tlimit;
        tlimit *= 1000; // previous units were in seconds, next unit is in milliseconds
        tlimit = (tlimit <= 0) ? config_settings.shutter_press_time_ms : tlimit;
        if (tlimit > 0 && (telapsed >= tlimit)) {
            safe_all_pins();
            gpio_time = 0;
        }
    }
}

extern int wifi_err_reason;
extern bool prevent_status_bar_thread;

void critical_error(const char* fp)
{
    prevent_status_bar_thread = true; // critical error can happen from the WiFi thread, so prevent the GUI thread from drawing a status bar over the error screen

    cpufreq_boost();
    pwr_tick(true);
    M5.Axp.GetBtnPress(); // clear the button bit
    uint32_t t = millis(), now = t;

    // disconnect
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, fp, 0, 0);

    if (wifi_err_reason != 0)
    {
        // indicate the error code if there is one
        M5Lcd.setTextFont(2);
        M5Lcd.highlight(true);
        M5Lcd.setTextWrap(true);
        M5Lcd.setHighlightColor(TFT_BLACK);
        M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Lcd.setCursor(5, M5Lcd.height() - 16); // bottom of screen
        M5Lcd.printf("REASON: %d", wifi_err_reason);
    }

    while (true)
    {
        pwr_sleepCheck();

        // restart on button press
        if (btnBoth_hasPressed()) {
            ESP.restart();
        }

        // shutdown on power button press
        if (M5.Axp.GetBtnPress() != 0) {
            show_poweroff();
            M5.Axp.PowerOff();
        }

        // if debugging over serial port, or allow the user to plug it in now, repeat the message
        if (((now = millis()) - t) > 2000) {
            Serial.print("CRITICAL ERROR");
            if (wifi_err_reason != 0) {
                Serial.printf(", WIFI REASON %d", wifi_err_reason);
            }
            Serial.println();
            t = now;
        }
    }
}

class AppAboutMe : public FairyMenuItem
{
    public:
        AppAboutMe() : FairyMenuItem("/about.png")
        {
        };

        virtual bool on_execute(void)
        {
            int loop_cnt = 0;
            int16_t ystart = 110;
            M5Lcd.fillRect(0, ystart, M5Lcd.width(), M5Lcd.height() - ystart - 14, TFT_WHITE);
            do
            {
                gui_startMenuPrint();
                M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                #ifndef ENABLE_BUILD_LEPTON
                M5Lcd.print("Build");
                ystart += M5Lcd.fontHeight() + 2;
                M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                #endif
                M5Lcd.printf("V %s", ALFY_VERSION); // found in alfy_conf.h , please change with every new build
                ystart += M5Lcd.fontHeight() + 2;
                M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                M5Lcd.setTextFont(2);
                #ifdef ENABLE_BUILD_LEPTON
                M5Lcd.print("LEPTON");
                ystart += M5Lcd.fontHeight() + 2;
                M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                #endif
                M5Lcd.print("Debug ");
                #ifdef DISABLE_ALL_MSG
                    M5Lcd.print("OFF");
                #else
                    M5Lcd.print("ON");
                #endif
                ystart += M5Lcd.fontHeight() + 2;
                M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                M5Lcd.print("CMD-line ");
                #ifdef DISABLE_ALL_MSG
                    M5Lcd.print("OFF");
                #else
                    M5Lcd.print("ON");
                #endif
                ystart += M5Lcd.fontHeight() + 2;
                M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                #ifdef DISABLE_POWER_SAVE
                    M5Lcd.print("PWR-save OFF");
                    ystart += M5Lcd.fontHeight() + 2;
                    M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                #endif
                #ifdef DISABLE_STATUS_BAR
                    M5Lcd.print("STS-bar OFF");
                    ystart += M5Lcd.fontHeight() + 2;
                    M5Lcd.setCursor(SUBMENU_X_OFFSET, ystart);
                #endif

                // if the screen overflows, try redrawing everything but full-screen
                if (ystart >= M5Lcd.height() - 16) {
                    ystart = 0;
                    M5Lcd.fillRect(0, ystart, M5Lcd.width(), M5Lcd.height() - ystart - 14, TFT_WHITE);
                    ystart = SUBMENU_Y_OFFSET;
                    continue;
                }
                else
                {
                    break;
                }
            }
            while ((loop_cnt++) <= 2);
            app_waitAllRelease();
            return false;
        };
};

void setup_aboutme(void)
{
    static AppAboutMe app;
    menu_utils.install(&app);
}

void spiffs_init(void)
{
    uint8_t fail = 0;
    if (!SPIFFS.begin(false))
    {
        Serial.println("SPIFFS Mount Failed");
        fail = 1;
    }
    else if (!SPIFFS.exists("/about.png"))
    {
        // A file that should exist which we can use to quickly test that the files are present.
        // The main case here is that the user only flashed the firmware, and not the FS, so a
        // single file is a sufficient check.
        fail = 2;
    }
    else if (!SPIFFS.exists("/chk0.txt"))
    {
        // use this file to make sure the version matches the files
        // change the file name when files are updated
        fail = 3;
    }

    // If there was any issue finding the images, give the user a helpful message
    if (fail != 0)
    {
        gui_startAppPrint();
        M5Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5Lcd.setTextFont(4);
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
        M5Lcd.printf("ERROR!!!");
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET + 25);
        if (fail != 3) {
            M5Lcd.printf("Image Files Missing");
        }
        else {
            M5Lcd.printf("Files out-of-date");
        }
        M5Lcd.setTextFont(2);
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET + 50);
        M5Lcd.printf("Please use the Arduino IDE");
        M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET + 68);
        if (fail != 3) {
            M5Lcd.printf("to upload the missing files");
        }
        else {
            M5Lcd.printf("to upload the new files");
        }

        // We should still let the user power off... No sense killing the battery.
        while (true)
        {
            yield();
            if (fail == 1) {
                Serial.println("SPIFFS Mount Failed");
            }
            else if (fail == 2) {
                Serial.println("Image files are missing");
            }
            if (M5.Axp.GetBtnPress() != 0) {
                pwr_shutdown();
            }
            delay(100);
        }
    }
}
