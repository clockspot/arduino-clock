// ESP32 with HT16K33 (7seg), DS3231, VEML7700
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ESP32_7SEG_H
#define CONFIG_ESP32_7SEG_H

#define ENABLE_SUN 0 //TODO fix Dusk2Dawn compile issue on ESP32
#define ENABLE_ALARM2 0
#define ENABLE_AWAYMODE 0
#define ENABLE_TESTY 0

// Disables holding Alt to set preset fn (assuming Alt is not used for power-switching).
// The preset fn is also removed from the fn cycle, so that Alt is the only way to get
// to it. Use this if Alt is labeled specifically for one fn.
#define FORCE_ALT_PRESET FN_ALARM

// Adafruit QT Py ESP32 routes I2C through the QT port (Wire1) rather than the pins (Wire).
// TODO: scrap DS3231 library in favor of RTClib which supports a custom TwoWire interface.
#define Wire Wire1
#define ENABLE_NEOPIXEL

#define RTC_IS_DS3231

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL GPIO_NUM_18 //A0
#define CTRL_ALT GPIO_NUM_17 //A1
#define INPUT_UPDN_BUTTONS
#define CTRL_DN GPIO_NUM_8 //A3
#define CTRL_UP GPIO_NUM_7 //A4

#define BATTERY_MONITOR_PIN GPIO_NUM_9 //A2, when QT Py is equipped with LiPo BFF

#define DISPLAY_HT16K33
#define SEVENSEG //enables display of letters in some cases
#define BRIGHTNESS_FULL 15
#define BRIGHTNESS_SETDIM 5 //when setting, flash alternates between full and this
#define BRIGHTNESS_DIM 3

#define LIGHTSENSOR_VEML7700
#define LIGHTSENSOR

#define PIEZO_PIN GPIO_NUM_6
#define ALARM_SIGNAL 1
#define TIMER_SIGNAL 1
#define CHIME_SIGNAL 1

#define ENABLE_SOFT_POWER_SWITCH 0

#endif //CONFIG_ESP32_7SEG_H
