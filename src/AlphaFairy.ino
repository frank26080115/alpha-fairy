#include "AlphaFairy.h"
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

FairySubmenu main_menu(SPRITE_ASSET_NONE, 0);

void setup()
{
    Serial.begin(SERIAL_PORT_BAUDRATE);
    dbg_ser.enabled = true;

    cpufreq_init();

    settings_init();
    btns_init();
    SonyCamIr_Init();

    auto m5cfg = M5.config();
    m5cfg.serial_baudrate = 0;
    m5cfg.clear_display = false;
    m5cfg.output_power = true;
    m5cfg.internal_imu = true;
    m5cfg.internal_rtc = true;
    m5cfg.internal_mic = false;
    m5cfg.internal_spk = false;
    m5cfg.external_imu = false;
    m5cfg.external_rtc = false;
#if defined(M5GFX_BOARD)
    m5cfg.fallback_board = static_cast<m5::board_t>(m5gfx::M5GFX_BOARD);
#endif
    M5.begin(m5cfg);

    M5.Display.setBrightness(0); // turn off the LCD backlight while initializing, avoids junk being shown on the screen
    M5Lcd.begin(); // our own extended LCD object
    M5Lcd.setBrightness(0);
    M5Lcd.fillScreen(TFT_BLACK);
    m5gfx_setBrightness(config_settings.lcd_brightness);

    storage_init();

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
    fairycam.set_debugflags(0);
    #endif
    #ifdef MUTE_NETMSG_ON_BOOT
    fairycam.set_debugflags(0);
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

FairySubmenu menu_remote  (SPRITE_ASSET_MAIN_REMOTE);
FairySubmenu menu_focus   (SPRITE_ASSET_MAIN_FOCUS);
FairyCfgApp  menu_interval(SPRITE_ASSET_MAIN_INTERVAL, SPRITE_ASSET_INTERVALOMETER, MENUITEM_INTERVAL);
FairyCfgApp  menu_astro   (SPRITE_ASSET_MAIN_ASTRO   , SPRITE_ASSET_GALAXY_ICON   , MENUITEM_ASTRO);
FairySubmenu menu_utils   (SPRITE_ASSET_MAIN_UTILS);
FairySubmenu menu_auto    (SPRITE_ASSET_MAIN_AUTO);

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
    setup_timecodeReset();

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
    static uint8_t busy_cnt = 0; // make sure we actually do something at least sometimes

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
    if (ptpcam.isKindaBusy() == false || airplane_mode != false || busy_cnt > 1) {
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

        busy_cnt = 0;

        return true; // can do more low priority tasks
    }
    else {
        busy_cnt++;
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

void critical_error(sprite_asset_id_t asset_id)
{
    prevent_status_bar_thread = true; // critical error can happen from the WiFi thread, so prevent the GUI thread from drawing a status bar over the error screen

    cpufreq_boost();
    pwr_tick(true);
    m5power_getButtonPress(); // clear the button bit
    uint32_t t = millis(), now = t;

    // disconnect
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    M5Lcd.setRotation(0);
    const sprite_asset_t* asset = spriteAsset(asset_id);
    if (asset != NULL) {
        M5Lcd.drawPngData(asset->data, asset->len, 0, 0);
    }

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
        if (m5power_getButtonPress() != 0) {
            show_poweroff();
            m5power_powerOff();
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
        AppAboutMe() : FairyMenuItem(SPRITE_ASSET_ABOUT)
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
                    M5Lcd.fillRect(0, ystart, M5Lcd.width(), M5Lcd.height() - ystart, TFT_WHITE);
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

void storage_init(void)
{
    if (ALFY_FS.begin(true, ALFY_FS_BASE_PATH, ALFY_FS_MAX_OPEN_FILES, ALFY_FS_LABEL)) {
        return;
    }

    Serial.println("LittleFS Mount Failed");

    gui_startAppPrint();
    M5Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5Lcd.setTextFont(4);
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET);
    M5Lcd.printf("ERROR!!!");
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET + 25);
    M5Lcd.printf("Storage Failed");
    M5Lcd.setTextFont(2);
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET + 50);
    M5Lcd.printf("LittleFS mount failed");
    M5Lcd.setCursor(SUBMENU_X_OFFSET, SUBMENU_Y_OFFSET + 68);
    M5Lcd.printf("Power to shut down");

    // We should still let the user power off... No sense killing the battery.
    while (true)
    {
        yield();
        Serial.println("LittleFS Mount Failed");
        if (m5power_getButtonPress() != 0) {
            pwr_shutdown();
        }
        delay(1000);
    }
}
