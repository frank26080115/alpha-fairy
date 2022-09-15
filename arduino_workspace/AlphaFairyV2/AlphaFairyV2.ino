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

uint32_t gpio_time = 0; // keeps track of the GPIO shutter activation time so it doesn't get stuck

bool redraw_flag = false; // forces menu redraw
#ifdef USE_SPRITE_MANAGER
SpriteMgr* sprites;
#endif

//extern bool http_is_active;

AlphaFairyImu imu;
FairyEncoder  fencoder;

FairySubmenu main_menu(NULL, 0);

void setup()
{
    settings_init();
    btns_init();

    SonyCamIr_Init();
    Serial.begin(115200);

    dbg_ser.enabled = true;

    setup_menus();

    M5.begin(false); // do not initialize the LCD, we have our own extended M5Lcd class to initialize later
    M5.IMU.Init();
    M5.IMU.SetGyroFsr(M5.IMU.GFS_500DPS);
    M5.IMU.SetAccelFsr(M5.IMU.AFS_4G);
    M5.Axp.begin();
    M5.Axp.ScreenSwitch(false); // turn off the LCD backlight while initializing, avoids junk being shown on the screen
    M5Lcd.begin(); // our own extended LCD object
    M5Lcd.fillScreen(TFT_BLACK);
    while (!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        delay(500);
    }
    M5.Axp.ScreenBreath(config_settings.lcd_brightness);
    cmdline.print_prompt();

    #ifdef USE_SPRITE_MANAGER
    sprites = new SpriteMgr(&M5Lcd);
    #endif

    httpcam.borrowBuffer(ptpcam.donateBuffer(), DATA_BUFFER_SIZE);

    cam_cb_setup();
    wifi_init();

    dbg_ser.printf("finished setup() at %u ms\r\n", millis());

    // clear the button flags
    btnAll_clrPressed();

    imu.poll();
    fencoder.begin();

    pwr_tick(true);

    #ifdef DISABLE_ALL_MSG
    dbg_ser.enabled = false;
    ptpcam.set_debugflags(0);
    #endif

    btnAll_clrPressed();
}

void loop()
{
    main_menu.on_execute();
}

FairySubmenu         menu_remote  ("/main_remote.png"  , MENUITEM_REMOTE);
FairySubmenu         menu_focus   ("/main_focus.png"   , MENUITEM_FOCUS);
FairyCfgApp          menu_interval("/main_interval.png", "/intervalometer.png", MENUITEM_INTERVAL);
FairyCfgApp          menu_astro   ("/main_astro.png"   , "/galaxy_icon.png", MENUITEM_ASTRO);
FairySubmenu         menu_utils   ("/main_utils.png"   , MENUITEM_UTILS);
FairySubmenu         menu_auto    ("/main_auto.png"    , MENUITEM_AUTOCONN);

void setup_menus()
{
    main_menu.install(&menu_remote  );
    main_menu.install(&menu_focus   );
    setup_intervalometer();
    main_menu.install(&menu_utils   );
    setup_autoconnect();

    setup_qikrmt();
    setup_remoteshutter();
    setup_soundshutter();
    setup_dualshutter();

    setup_focusstack();
    setup_shutterstep();
    setup_focuspull();
    setup_focusfrustration();

    setup_focuscalib();

    setup_wifimenus();
}

bool app_poll()
{
    // high priority tasks
    NetMgr_task();
    ptpcam.task();
    httpcam.task();
    //httpsrv_poll();

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

        yield();

        pwr_lightSleepEnter();

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
            #ifdef SHUTTER_GPIO_ACTIVE_HIGH
            digitalWrite(SHUTTER_GPIO, LOW);
            #endif
            pinMode(SHUTTER_GPIO, INPUT);
            gpio_time = 0;
        }
    }
}

extern int wifi_err_reason;
extern bool prevent_status_bar_thread;

void critical_error(const char* fp)
{
    prevent_status_bar_thread = true;
    pwr_tick(true);
    M5.Axp.GetBtnPress();
    uint32_t t = millis(), now = t;
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, fp, 0, 0);

    if (wifi_err_reason != 0)
    {
        M5Lcd.setTextFont(2);
        M5Lcd.highlight(true);
        M5Lcd.setTextWrap(true);
        M5Lcd.setHighlightColor(TFT_BLACK);
        M5Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Lcd.setCursor(5, M5Lcd.height() - 16);
        M5Lcd.printf("REASON: %d", wifi_err_reason);
    }

    while (true)
    {
        pwr_sleepCheck();
        if (btnBoth_hasPressed()) {
            #if 0
            delay(100);
            btnBoth_clrPressed();
            while (btnBig_isPressed() || btnSide_isPressed()) {
                delay(100);
            }
            NetMgr_reboot();
            redraw_flag = true;
            return;
            #else
            ESP.restart();
            #endif
        }
        if (M5.Axp.GetBtnPress() != 0) {
            show_poweroff();
            M5.Axp.PowerOff();
        }
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
