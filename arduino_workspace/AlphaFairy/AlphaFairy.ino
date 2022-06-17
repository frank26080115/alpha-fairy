#include "AlphaFairy.h"
#include <M5DisplayExt.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <AlphaFairy_NetMgr.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>

PtpIpSonyAlphaCamera camera((char*)"Alpha-Fairy");

void remotes_shutter(void* mip);
void focus_stack    (void* mip);
void focus_9point   (void* mip);
void wifi_info      (void* mip);
void record_movie   (void* mip);
void conf_settings  (void* mip);

void on_got_client(uint32_t ip);

uint8_t menuitem_cnt = 0; // menu length is counted at run-time
uint8_t menuitem_idx = 0; // currently shown menu item

const menuitem_t menu_items[] = {
    // ID                       , FILE-NAME               , FUNCTION POINTER
    { MENUITEM_REMOTESHUTTER_NOW, "/remoteshutter.jpg"    , remotes_shutter },
    { MENUITEM_REMOTESHUTTER_2S , "/remoteshutter_2s.jpg" , remotes_shutter },
    { MENUITEM_REMOTESHUTTER_5S , "/remoteshutter_5s.jpg" , remotes_shutter },
    { MENUITEM_REMOTESHUTTER_10S, "/remoteshutter_10.jpg" , remotes_shutter },
    { MENUITEM_FOCUSSTACK_FAR_1 , "/focusstack_far_1.jpg" , focus_stack     },
    { MENUITEM_FOCUSSTACK_FAR_2 , "/focusstack_far_2.jpg" , focus_stack     },
    { MENUITEM_FOCUSSTACK_FAR_3 , "/focusstack_far_3.jpg" , focus_stack     },
    { MENUITEM_FOCUS_9POINT     , "/focus_9point.jpg"     , focus_9point    },
    { MENUITEM_RECORDMOVIE      , "/recordmovie.jpg"      , record_movie    },
    { MENUITEM_WIFIINFO         , "/wifiinfo.jpg"         , wifi_info       },
    { MENUITEM_CONFIG           , "/config.jpg"           , conf_settings   },
    { MENUITEM_END_OF_TABLE     , ""                      , NULL            }, // menu length is counted at run-time
};

DebuggingSerial dbg_ser(&Serial);

void setup()
{
    settings_init();
    ledblink_init();
    btns_init();
    guimenu_init();
    SonyCamIr_Init();
    Serial.begin(115200);
    dbg_ser.enabled = true;
    M5.begin(false); // do not initilize the LCD, we have our own extended M5Lcd class to initialize later
    M5.IMU.Init();
    M5Lcd.begin(); // our own extended LCD object
    while (!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        delay(500);
    }
    cmdline.print_prompt();
    NetMgr_begin((char*)"afairywifi", (char*)"1234567890", on_got_client);
    dbg_ser.printf("finished setup() at %u ms\r\n", millis());
    welcome(); // splash screen for a few seconds
}

void loop()
{
    if (app_poll())
    {
        // can do more low priority tasks
        // the graphics is very IO-intensive and blocking so we want to do it when the network is not busy
        guimenu_task();
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

void guimenu_task()
{
    static int last_menu_idx = -1; // remember last item to prevent unneccessary re-draws

    // if the camera is already recording a movie, then automatically move the menu page to the moving recording page
    // the camera won't respond to any other commands in this state anyways, so why not?
    if (camera.isOperating() && camera.is_movierecording()) {
        int x = guimenu_getIdx(MENUITEM_RECORDMOVIE);
        if (x >= 0) {
            menuitem_idx = x;
            dbg_ser.printf("movie is recording, forcing menu idx %u\r\n", menuitem_idx);
        }
    }

    if (btnSide_hasPressed(true))
    {
        // side-button press means go to another menu item
        // go to the previous item if the angle of the device is "down"
        if (imu_angle == ANGLE_IS_DOWN) {
            if (menuitem_idx > 0) {
                menuitem_idx -= 1;
                dbg_ser.printf("menu prev idx %u\r\n", menuitem_idx);
            }
        }
        else { // go to the next item if the angle is not "down"
            if (menuitem_idx < (menuitem_cnt - 1)) {
                menuitem_idx += 1;
            }
            else {
                menuitem_idx = 0;
            }
            dbg_ser.printf("menu next idx %u\r\n", menuitem_idx);
        }
    }

    if (last_menu_idx != menuitem_idx) { // prevent unneccessary re-draws
        guimenu_drawScreen();
        last_menu_idx = menuitem_idx;
        app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
    }
    else if (imu_hasChange) { // prevent unneccessary re-draws
        guimenu_drawPages(); // this draws the scroll dots that indicate which page we are on
        imu_hasChange = false;
    }

    if (btnBig_hasPressed(true))
    {
        menuitem_t* menuitm = (menuitem_t*)&(menu_items[menuitem_idx]);
        dbg_ser.printf("menu idx %u - %u calling func\r\n", menuitem_idx, menuitm->id);
        menuitm->func((void*)menuitm);
        guimenu_drawScreen(); // any sort of indicator while in the function need to be removed, so just draw the whole menu here again
        ledblink_setMode(LEDMODE_NORMAL);
        app_sleep(50, true); // kinda sorta a debounce and rate limit, don't think I need this here
    }
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
