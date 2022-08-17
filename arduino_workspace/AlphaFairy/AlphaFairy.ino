#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>
#include <SpriteMgr.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <SonyHttpCamera.h>
#include "AlphaFairyCamera.h"
#include <AlphaFairy_NetMgr.h>
#include <AlphaFairyImu.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>

PtpIpSonyAlphaCamera ptpcam((char*)"ALPHA-FAIRY", NULL);
SonyHttpCamera httpcam;
AlphaFairyCamera fairycam(&ptpcam, &httpcam);

void remote_shutter (void* mip);
void focus_stack    (void* mip);
void focus_9point   (void* mip);
void focus_pull     (void* mip);
void zoom_pull      (void* mip);
void shutter_step   (void* mip);
void wifi_info      (void* mip);
void record_movie   (void* mip);
void dual_shutter   (void* mip);
void sound_shutter  (void* mip);
void conf_settings  (void* mip);
void submenu_enter  (void* mip);
void intervalometer_config(void* mip);
void focusfrust_execute(void* mip);
void wifi_config(void* mip);

const menuitem_t menu_items_main[] = {
    // ID                   , FILE-NAME            , FUNCTION POINTER
    { MENUITEM_REMOTE,        "/main_remote.png"   , submenu_enter         },
    { MENUITEM_FOCUS,         "/main_focus.png"    , submenu_enter         },
    { MENUITEM_INTERVAL,      "/main_interval.png" , intervalometer_config },
    { MENUITEM_ASTRO,         "/main_astro.png"    , intervalometer_config },
    { MENUITEM_UTILS,         "/main_utils.png"    , submenu_enter         },
    { MENUITEM_END_OF_TABLE , ""                   , NULL                  }, // menu length is counted at run-time
};

const menuitem_t menu_items_remote[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_REMOTESHUTTER_NOW, "/remoteshutter.png"    , remote_shutter },
    { MENUITEM_REMOTESHUTTER_DLY, "/remoteshutter_d.png"  , remote_shutter },
    { MENUITEM_RECORDMOVIE      , "/recordmovie.png"      , record_movie   },
    { MENUITEM_FOCUS_PULL       , "/focus_pull.png"       , focus_pull     },
    { MENUITEM_ZOOM_PULL        , "/zoom_adjust.png"      , zoom_pull      },
    { MENUITEM_SOUNDSHUTTER     , "/soundshutter.png"     , sound_shutter  },
    { MENUITEM_DUALSHUTTER_REG  , "/dualshutter_reg.png"  , dual_shutter   },
    { MENUITEM_DUALSHUTTER_SHOOT, "/dualshutter_shoot.png", dual_shutter   },
    #if !defined(USE_PWR_BTN_AS_EXIT) || defined(USE_PWR_BTN_AS_BACK)
    { MENUITEM_BACK             , "/back.png"             , NULL           },
    #endif
    { MENUITEM_END_OF_TABLE     , ""                      , NULL           }, // menu length is counted at run-time
};

const menuitem_t menu_items_focus[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_FOCUSSTACK_FAR_1 , "/focusstack_far_1.png" , focus_stack     },
    { MENUITEM_FOCUSSTACK_FAR_2 , "/focusstack_far_2.png" , focus_stack     },
    { MENUITEM_FOCUS_9POINT     , "/focus_9point.png"     , focus_9point    },
    { MENUITEM_SHUTTERSTEP      , "/shutter_step.png"     , shutter_step    },
    { MENUITEM_FOCUS_PULL       , "/focus_pull.png"       , focus_pull      },
    { MENUITEM_FOCUSFRUSTRATION , "/focus_frust.png"      , focusfrust_execute },
    #if !defined(USE_PWR_BTN_AS_EXIT) || defined(USE_PWR_BTN_AS_BACK)
    { MENUITEM_BACK             , "/back.png"             , NULL            },
    #endif
    { MENUITEM_END_OF_TABLE     , ""                      , NULL            }, // menu length is counted at run-time
};

const menuitem_t menu_items_utils[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_WIFIINFO         , "/wifiinfo.png"         , wifi_info       },
    { MENUITEM_CONFIG           , "/config.png"           , conf_settings   },
    { MENUITEM_WIFICONFIG       , "/wifi_config.png"      , wifi_config     },
    #ifdef MENU_INCLUDE_ABOUT
    { MENUITEM_ABOUT            , "/about.png"            , NULL            },
    #endif
    #if !defined(USE_PWR_BTN_AS_EXIT) || defined(USE_PWR_BTN_AS_BACK)
    { MENUITEM_BACK             , "/back.png"             , NULL            },
    #endif
    { MENUITEM_END_OF_TABLE     , ""                      , NULL            }, // menu length is counted at run-time
};

menustate_t menustate_main;
menustate_t menustate_remote;
menustate_t menustate_focus;
menustate_t menustate_utils;

menustate_t menustate_intval;
menustate_t menustate_astro;

menustate_t menustate_soundshutter;

extern menustate_t menustate_wificonfig;
extern menustate_t menustate_wifiinfo;

menustate_t* curmenu = &menustate_main;

DebuggingSerial dbg_ser(&Serial);

uint32_t gpio_time = 0; // keeps track of the GPIO shutter activation time so it doesn't get stuck

bool redraw_flag = false; // forces menu redraw
#ifdef USE_SPRITE_MANAGER
SpriteMgr* sprites;
#endif

AlphaFairyImu imu;

void setup()
{
    settings_init();
    ledblink_init();
    btns_init();

    SonyCamIr_Init();
    Serial.begin(115200);
    dbg_ser.enabled = true;
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
    mictrig_init();
    cmdline.print_prompt();

    #ifdef USE_SPRITE_MANAGER
    sprites = new SpriteMgr(&M5Lcd);
    #endif

    #ifdef QUICK_HTTP_TEST
    wifi_config(NULL);
    #endif

    httpcam.borrowBuffer(ptpcam.donateBuffer(), DATA_BUFFER_SIZE);

    cam_cb_setup();
    wifi_init();

    guimenu_init(MENUITEM_MAIN  , &menustate_main  , (menuitem_t*)menu_items_main  );
    guimenu_init(MENUITEM_REMOTE, &menustate_remote, (menuitem_t*)menu_items_remote);
    guimenu_init(MENUITEM_FOCUS , &menustate_focus , (menuitem_t*)menu_items_focus );
    guimenu_init(MENUITEM_UTILS , &menustate_utils , (menuitem_t*)menu_items_utils );

    dbg_ser.printf("finished setup() at %u ms\r\n", millis());

    // clear the button flags
    btnAll_clrPressed();

    imu.poll();

    welcome(); // splash screen for a few seconds
    pwr_tick();

    #ifdef DISABLE_ALL_MSG
    dbg_ser.enabled = false;
    ptpcam.set_debugflags(0);
    #endif

    btnAll_clrPressed();
}

void loop()
{
    if (app_poll())
    {
        // can do more low priority tasks
        // the graphics is very IO-intensive and blocking so we want to do it when the network is not busy

        if (guimenu_task(&menustate_main))
        {
            #if defined(USE_PWR_BTN_AS_EXIT) && defined(USE_PWR_BTN_AS_PWROFF)
            pwr_shutdown();
            #endif
        }
    }
    pwr_sleepCheck();
}

extern volatile uint32_t btnPwr_cnt;

bool app_poll()
{
    uint32_t now = millis();
    static uint32_t btn_last_time = 0;

    // high priority tasks
    ledblink_task();
    #ifdef TRY_CATCH_MISSED_GPIO_ISR
    btns_poll();
    #endif
    NetMgr_task();
    ptpcam.task();
    httpcam.task();
    httpsrv_poll();

    if (gpio_time != 0)
    {
        // release the GPIO after a timeout
        uint32_t telapsed = now - gpio_time;
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

    // do low priority tasks if the networking is not busy
    if (ptpcam.isKindaBusy() == false) {
        imu.poll();
        cmdline.task();

        if (imu.hasMajorMotion) {
            // do not sleep if the user is moving the device
            imu.hasMajorMotion = false;
            pwr_tick();
        }

        if ((now - btn_last_time) > 100 || btn_last_time == 0) {
            btn_last_time = now;
            uint8_t b = M5.Axp.GetBtnPress();
            if (b != 0) {
                btnPwr_cnt++;
                dbg_ser.printf("user pressed power button\r\n");
                pwr_tick();
            }
        }

        yield();
        return true; // can do more low priority tasks
    }

    return false; // should not do more low priority tasks
}

void app_sleep(uint32_t x, bool forget_btns)
{
    uint32_t tstart = millis();
    uint32_t now;
    while (((now = millis()) - tstart) < x) {
        app_poll();
    }
    if (forget_btns) {
        btnAll_clrPressed();
    }
}

void wifi_onConnect()
{
    dbg_ser.printf("application wifi event handler called\r\n");
    pwr_tick();
    if (ptpcam.canNewConnect()) {
        uint32_t newip = NetMgr_getConnectableClient();
        if (newip != 0)
        {
            #if 1
            if (NetMgr_getOpMode() == WIFIOPMODE_STA)
            {
                // if the mode is STA then it is more likely that the camera is HTTP
                // it seems like some of those cameras do not like being connected to over the PTP port first
                // so add an extra delay for the PTP initialization just in case
                ptpcam.begin(newip, 5000);
            }
            else
            #endif
            {
                ptpcam.begin(newip);
            }
        }
    }
    if (httpcam.canNewConnect()) {
        uint32_t newip = NetMgr_getConnectableClient();
        if (newip != 0) {
            httpcam.begin(newip);
        }
    }
}

void wifi_onDisconnect()
{
    httpcam.begin(0); // this resets the forbidden flag so it can reconnect again later
}

void ptpcam_onConnect()
{
    dbg_ser.printf("ptpcam_onConnect\r\n");
    pwr_tick();
    if (ptpcam.isOperating()) {
        NetMgr_markClientCameraPtp(ptpcam.getIp());
        httpcam.setForbidden();
    }
}

void httpcam_onConnect()
{
    dbg_ser.printf("httpcam_onConnect\r\n");
    pwr_tick();
    if (httpcam.isOperating()) {
        NetMgr_markClientCameraHttp(httpcam.getIp());
    }
}

void ptpcam_onDisconnect()
{
    pwr_tick();
    NetMgr_markClientDisconnect(ptpcam.getIp());
}

void httpcam_onDisconnect()
{
    pwr_tick();
    NetMgr_markClientDisconnect(httpcam.getIp());
}

void ptpcam_onCriticalError()
{
    pwr_tick();
    if (ptpcam.critical_error_cnt > 2) {
        NetMgr_markClientError(ptpcam.getIp());
        if (NetMgr_shouldReportError() && httpcam.isOperating() == false) {
            critical_error("/crit_error.png");
        }
    }
}

void httpcam_onCriticalError()
{
    pwr_tick();
    if (httpcam.critical_error_cnt > 0) {
        NetMgr_markClientError(httpcam.getIp());
        if (NetMgr_shouldReportError() && ptpcam.isOperating() == false) {
            critical_error("/crit_error.png");
        }
    }
}

void ptpcam_onReject()
{
    critical_error("/rejected.png");
}

void cam_cb_setup()
{
    ptpcam.cb_onConnect = ptpcam_onConnect;
    ptpcam.cb_onDisconnect = ptpcam_onDisconnect;
    ptpcam.cb_onCriticalError = ptpcam_onCriticalError;
    ptpcam.cb_onReject = ptpcam_onReject;

    httpcam.cb_onConnect = ptpcam_onConnect;
    httpcam.cb_onDisconnect = httpcam_onDisconnect;
    httpcam.cb_onCriticalError = httpcam_onCriticalError;
}

void app_waitAnyPress(bool can_sleep)
{
    while (true)
    {
        app_poll();
        if (can_sleep == false) {
            pwr_tick();
        }
        if (btnAll_hasPressed()) {
            break;
        }
    }
    btnAll_clrPressed();
}

void app_waitAllRelease(uint32_t debounce)
{
    btnAll_clrPressed();
    if (btnSide_isPressed() == false && btnBig_isPressed() == false)
    {
        return;
    }

    uint32_t now = millis();
    uint32_t last_time = now;
    do
    {
        app_poll();
        if (btnSide_isPressed() || btnBig_isPressed()) {
            last_time = millis();
        }
    }
    while ((last_time - (now = millis())) < debounce);
}

void app_waitAllReleaseConnecting(uint32_t debounce)
{
    btnAll_clrPressed();
    if (btnSide_isPressed() == false && btnBig_isPressed() == false)
    {
        return;
    }

    gui_drawConnecting(true);

    uint32_t now = millis();
    uint32_t last_time = now;
    do
    {
        if (app_poll()) {
            gui_drawConnecting(false);
        }
        if (ptpcam.isOperating()) {
            return;
        }
        if (btnSide_isPressed() || btnBig_isPressed()) {
            last_time = millis();
            continue;
        }
    }
    while (((now = millis()) - last_time) < debounce);
}

void app_waitAllReleaseUnsupported(uint32_t debounce)
{
    btnAll_clrPressed();
    if (btnSide_isPressed() == false && btnBig_isPressed() == false)
    {
        return;
    }

    M5Lcd.drawPngFile(SPIFFS, "/unsupported.png", 0, 0);

    uint32_t now = millis();
    uint32_t last_time = now;
    do
    {
        if (btnSide_isPressed() || btnBig_isPressed()) {
            last_time = millis();
            continue;
        }
    }
    while (((now = millis()) - last_time) < debounce);
    redraw_flag = true;
}

void critical_error(const char* fp)
{
    pwr_tick();
    M5.Axp.GetBtnPress();
    uint32_t t = millis(), now = t;
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    M5Lcd.setRotation(0);
    M5Lcd.drawPngFile(SPIFFS, fp, 0, 0);
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
            Serial.println("CRITICAL ERROR");
            t = now;
        }
    }
}
