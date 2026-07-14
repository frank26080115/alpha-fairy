#ifndef BUTTON_PINS_H
#define BUTTON_PINS_H

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define PIN_BTN_SIDE     12
#define PIN_BTN_BIG      11
#define GPIO_BTN_SIDE    GPIO_NUM_12
#define GPIO_BTN_BIG     GPIO_NUM_11
#else
#define PIN_BTN_SIDE     39
#define PIN_BTN_BIG      37
#define GPIO_BTN_SIDE    GPIO_NUM_39
#define GPIO_BTN_BIG     GPIO_NUM_37
#endif

#endif
