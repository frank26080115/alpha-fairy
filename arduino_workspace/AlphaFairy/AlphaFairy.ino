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

PtpIpSonyAlphaCamera ptpcam((char*)"ALPHA-FAIRY", NULL);
SonyHttpCamera       httpcam;
AlphaFairyCamera     fairycam(&ptpcam, &httpcam);
DebuggingSerial      dbg_ser(&Serial);

#if defined(SERDBG_DISABLE) && !defined(DISABLE_ALL_MSG)
#error serial port disabled via library but messages are enabled
#endif

uint32_t gpio_time = 0; // keeps track of the GPIO shutter activation time so it doesn't get stuck

bool redraw_flag = false; // forces menu redraw
SpriteMgr* sprites;

AlphaFairyImu imu;
FairyEncoder  fencoder;

FairySubmenu main_menu(NULL, 0);

void setup()
{
    Serial.begin(SERIAL_PORT_BAUDRATE);
    dbg_ser.enabled = true;

    cpufreq_init();
    //cpufreq_boost();

    settings_init();
    btns_init();
    SonyCamIr_Init();

    M5.begin(false); // do not initialize the LCD, we have our own extended M5Lcd class to initialize later
    M5.IMU.Init();
    M5.IMU.SetGyroFsr(M5.IMU.GFS_500DPS);
    M5.IMU.SetAccelFsr(M5.IMU.AFS_4G);
    M5.Axp.begin();
    M5.Axp.ScreenSwitch(false); // turn off the LCD backlight while initializing, avoids junk being shown on the screen
    //M5Lcd.cb_needboost = cpufreq_boost;
    M5Lcd.begin(); // our own extended LCD object
    M5Lcd.fillScreen(TFT_BLACK);
    while (!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        delay(500);
    }
    M5.Axp.ScreenBreath(config_settings.lcd_brightness);

    #ifdef PMIC_LOG_ON_BOOT
    pmic_startCoulombCount();
    #endif

    setup_menus();

    cmdline.print_prompt();

    sprites = new SpriteMgr(&M5Lcd);
    //sprites->cb_needboost = cpufreq_boost;

    httpcam.borrowBuffer(ptpcam.donateBuffer(), DATA_BUFFER_SIZE);

    cam_cb_setup();
    wifi_init();

    dbg_ser.printf("finished setup() at %u ms\r\n", millis());

    // clear the button flags
    btnAny_clrPressed();

    imu.poll();
    fencoder.begin();

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

    main_menu.install(&menu_remote  );
    main_menu.install(&menu_focus   );
    setup_intervalometer();
    main_menu.install(&menu_utils   );
    setup_autoconnect();

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
    NetMgr_task();
    ptpcam.task();
    httpcam.task();
    #ifdef HTTP_ON_BOOT
    httpsrv_poll();
    #endif

    // do low priority tasks if the networking is not busy
    if (ptpcam.isKindaBusy() == false) {
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
};

void setup_aboutme(void)
{
    static AppAboutMe app;
    menu_utils.install(&app);
}
