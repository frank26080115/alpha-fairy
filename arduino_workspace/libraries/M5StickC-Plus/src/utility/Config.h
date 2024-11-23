#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef ARDUINO_M5Stick_C_Plus2
#define M5_IR          9
#define M5_LED         10
#else
#define M5_IR          19
#define M5_LED         19
#define POWER_HOLD_PIN 4
#define LCD_BL_PIN 27
#define LCD_PWM_CHAN 2
// NOTE: PWM HZ and BITS has to be carefully chosen https://lastminuteengineers.com/esp32-pwm-tutorial/
#define LCD_BL_HZ 500
#define LCD_BL_RESOLUTION_BITS 12
const uint32_t gamma_lut[LCD_BL_RESOLUTION_BITS + 1] = {0,516,920,1289,1639,1974,2298,2613,2920,3222,3517,3808,4095};
#endif
/*
import matplotlib.pyplot as plt
import numpy as np

max_bits = int(12)
gamma = float(2.2)

x = np.arange(0, max_bits + 1)
y = np.power(x, 1. / gamma)
y = y / np.max(y) * (2 ** max_bits - 1)
plt.plot(x, y)
# plt.show()
print(",".join(y.astype(int).astype(str)))
*/
#define M5_BUTTON_HOME 37
#define M5_BUTTON_RST  39

#define BUTTON_A_PIN 37
#define BUTTON_B_PIN 39

#define SPEAKER_PIN      2
#define TONE_PIN_CHANNEL 0

// UART
#define USE_SERIAL Serial

#endif /* SETTINGS_C */
