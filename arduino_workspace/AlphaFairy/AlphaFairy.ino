#include "AlphaFairy.h"
#include <M5StickCPlus.h>
#include <M5DisplayExt.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <AlphaFairy_NetMgr.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>

PtpIpSonyAlphaCamera camera((char*)"Alpha-Fairy", NULL);

void remotes_shutter(void* mip);
void focus_stack    (void* mip);
void focus_9point   (void* mip);
void shutter_step   (void* mip);
void wifi_info      (void* mip);
void record_movie   (void* mip);
void conf_settings  (void* mip);
void submenu_enter  (void* mip);

void on_got_client(uint32_t ip);

const menuitem_t menu_items_main[] = {
    // ID                   , FILE-NAME            , FUNCTION POINTER
    { MENUITEM_REMOTE,        "/main_remote.png"   , submenu_enter },
    { MENUITEM_FOCUS,         "/main_focus.png"    , submenu_enter },
    { MENUITEM_INTERVAL,      "/main_interval.png" , submenu_enter },
    { MENUITEM_ASTRO,         "/main_astro.png"    , submenu_enter },
    { MENUITEM_UTILS,         "/main_utils.png"    , submenu_enter },
    { MENUITEM_END_OF_TABLE , ""                   , NULL          }, // menu length is counted at run-time
};

const menuitem_t menu_items_remote[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_REMOTESHUTTER_NOW, "/remoteshutter.png"    , remotes_shutter },
    { MENUITEM_REMOTESHUTTER_2S , "/remoteshutter_2s.png" , remotes_shutter },
    { MENUITEM_REMOTESHUTTER_5S , "/remoteshutter_5s.png" , remotes_shutter },
    { MENUITEM_REMOTESHUTTER_10S, "/remoteshutter_10.png" , remotes_shutter },
    { MENUITEM_RECORDMOVIE      , "/recordmovie.png"      , record_movie    },
    { MENUITEM_BACK             , "/back.png"             , NULL            },
    { MENUITEM_END_OF_TABLE     , ""                      , NULL            }, // menu length is counted at run-time
};

const menuitem_t menu_items_focus[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_FOCUSSTACK_FAR_1 , "/focusstack_far_1.png" , focus_stack     },
    { MENUITEM_FOCUSSTACK_FAR_2 , "/focusstack_far_2.png" , focus_stack     },
    { MENUITEM_FOCUSSTACK_FAR_3 , "/focusstack_far_3.png" , focus_stack     },
    { MENUITEM_FOCUS_9POINT     , "/focus_9point.png"     , focus_9point    },
    { MENUITEM_SHUTTERSTEP      , "/shutter_step.png"     , shutter_step    },
    { MENUITEM_BACK             , "/back.png"             , NULL            },
    { MENUITEM_END_OF_TABLE     , ""                      , NULL            }, // menu length is counted at run-time
};

const menuitem_t menu_items_utils[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_WIFIINFO         , "/wifiinfo.png"         , wifi_info       },
    { MENUITEM_CONFIG           , "/config.png"           , conf_settings   },
    { MENUITEM_BACK             , "/back.png"             , NULL            },
    { MENUITEM_END_OF_TABLE     , ""                      , NULL            }, // menu length is counted at run-time
};

menustate_t menustate_main;
menustate_t menustate_remote;
menustate_t menustate_focus;
menustate_t menustate_utils;

menustate_t menustate_intval;
menustate_t menustate_astro;

menustate_t* curmenu = &menustate_main;

DebuggingSerial dbg_ser(&Serial);

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
    M5.Axp.begin();
    M5Lcd.begin(); // our own extended LCD object
    while (!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        delay(500);
    }
    cmdline.print_prompt();
    NetMgr_begin((char*)"afairywifi", (char*)"1234567890", on_got_client);

    guimenu_init(MENUITEM_MAIN  , &menustate_main  , (menuitem_t*)menu_items_main  );
    guimenu_init(MENUITEM_REMOTE, &menustate_remote, (menuitem_t*)menu_items_remote);
    guimenu_init(MENUITEM_FOCUS , &menustate_focus , (menuitem_t*)menu_items_focus );
    guimenu_init(MENUITEM_UTILS , &menustate_utils , (menuitem_t*)menu_items_utils );

    dbg_ser.printf("finished setup() at %u ms\r\n", millis());

    welcome(); // splash screen for a few seconds
}

void loop()
{
    if (app_poll())
    {
        // can do more low priority tasks
        // the graphics is very IO-intensive and blocking so we want to do it when the network is not busy

        guimenu_task(&menustate_main);
    }
}

bool app_poll()
{
    // high priority tasks
    ledblink_task();
    NetMgr_task();
    camera.task();
    if (camera.getState() >= PTPSTATE_DISCONNECTED) {
        NetMgr_reset();
    }

    // do low priority tasks if the networking is not busy
    if (camera.isKindaBusy() == false) {
        app_anglePoll();
        cmdline.task();
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
        btns_clear();
    }
}

void on_got_client(uint32_t ip)
{
    if (camera.canNewConnect())
    {
        camera.begin(ip);
    }
}

void app_waitAnyPress()
{
    while (true)
    {
        app_poll();
        if (btnSide_hasPressed(true) || btnBig_hasPressed(true))
        {
            break;
        }
    }
    btns_clear();
}

void app_waitAllRelease(uint32_t debounce)
{
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
        if (btnSide_isPressed() || btnBig_isPressed()) {
            last_time = millis();
        }
        if (camera.isOperating()) {
            return;
        }
    }
    while ((last_time - (now = millis())) < debounce);
}
