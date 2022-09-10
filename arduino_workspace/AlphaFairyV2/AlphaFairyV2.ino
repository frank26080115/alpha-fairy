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

AlphaFairyImu imu;
FairyEncoder  fencoder;

void setup()
{
    settings_init();
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

    dbg_ser.printf("finished setup() at %u ms\r\n", millis());

    // clear the button flags
    btnAll_clrPressed();

    imu.poll();
    fencoder.begin();

    welcome(); // splash screen for a few seconds
    pwr_tick(true);

    #ifdef DISABLE_ALL_MSG
    dbg_ser.enabled = false;
    ptpcam.set_debugflags(0);
    #endif

    btnAll_clrPressed();
}

void loop()
{
}
