#ifndef _ALFY_CONF_H_
#define _ALFY_CONF_H_

//#define HTTP_SERVER_ENABLE
//#define WIFI_ALL_MODES
// leave the above disabled until cameras that only have wifi-direct mode are supported

//#define WIFI_AP_UNIQUE_NAME
#ifndef WIFI_AP_UNIQUE_NAME
#define WIFI_DEFAULT_SSID "fairywifi"
#else
#define WIFI_DEFAULT_SSID "fairy"
#endif
#define WIFI_DEFAULT_PASS  "1234567890"

//#define DISABLE_STATUS_BAR
//#define DISABLE_POWER_SAVE
#define DISABLE_ALL_MSG
//#define DISABLE_CMD_LINE
//#define QUICK_HTTP_TEST
//#define HTTP_MOCKBTNS_ENABLE

#define USE_SPRITE_MANAGER
#define DISABLE_WELCOME
#define MENU_INCLUDE_ABOUT

#define USE_PWR_BTN_AS_EXIT
//#define USE_PWR_BTN_AS_BACK
#define USE_PWR_BTN_AS_PWROFF

#define LEDBLINK_USE_PWM
#define LEDBLINK_PWM_DUTY_ON  (0xFF - 16)
#define LEDBLINK_PWM_DUTY_OFF 0xFF

#define SHUTTER_GPIO 26
#define SHUTTER_GPIO_ACTIVE_LOW
//#define SHUTTER_GPIO_ACTIVE_HIGH

#define SUBMENU_X_OFFSET 5
#define SUBMENU_Y_OFFSET 5
#define MICTRIG_LEVEL_MARGIN      16

#define PAGINATION_DOT_SIZE      2
#define PAGINATION_SPACE_SIZE    2
#define PAGINATION_BOTTOM_MARGIN 0

#define WELCOME_TIME_MS        10000
#define WELCOME_CONN_TIME_MS   (WELCOME_TIME_MS + 3000)

#define BTN_DEBOUNCE 50

#endif
