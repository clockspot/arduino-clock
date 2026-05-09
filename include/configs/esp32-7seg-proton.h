// ESP32 with HT16K33 (7seg), DS3231
// Alternate controls for retrofitting Proton clock radio
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ESP32_7SEG_PROTON_H
#define CONFIG_ESP32_7SEG_PROTON_H

#define ENABLE_DAY_COUNTER 0
#define ENABLE_SUN 0 //TODO fix Dusk2Dawn compile issue on ESP32
#define ENABLE_ALARM_AUTOSKIP 0
#define ENABLE_ALARM_FIBONACCI 0
#define ENABLE_CHIME 0
#define ENABLE_DIMMING 0
#define ENABLE_AWAYMODE 0

// Adafruit QT Py ESP32 routes I2C through the QT port (Wire1) rather than the pins (Wire).
// TODO: scrap DS3231 library in favor of RTClib which supports a custom TwoWire interface.
#define Wire Wire1
#define ENABLE_NEOPIXEL

#define RTC_IS_DS3231

#define INPUT_PROTON
//For Adafruit QT Py ESP32:
#define CTRL_OFF GPIO_NUM_18 //A0
#define CTRL_ON GPIO_NUM_17 //A1
#define CTRL_AL1 GPIO_NUM_9 //A2 - toggle, inverted (skip if using LiPo BFF)
#define CTRL_AL2 GPIO_NUM_8 //A3 - toggle, inverted
#define CTRL_SNOOZE GPIO_NUM_7 //SDA
#define CTRL_SLEEP GPIO_NUM_6 //SCL
#define CTRL_UP GPIO_NUM_35 //MOSI
#define CTRL_DN GPIO_NUM_37 //MISO
//GPIO_NUM_5 TX available
//GPIO_NUM_36 SCK used for output (radio on/sleep)
//GPIO_NUM_16 RX used for output (alarm pulse, which Proton can make radio or buzzer)

#define DISPLAY_HT16K33
#define SEVENSEG //enables display of letters in some cases
#define BRIGHTNESS_FULL 15
#define BRIGHTNESS_SETDIM 5 //when setting, flash alternates between full and this

#define SWITCH_PIN GPIO_NUM_36 //SCK
#define PULSE_PIN GPIO_NUM_16 //RX - drives Proton alarm circuitry (expects 2Hz beep pattern)
#define ALARM_SIGNAL 2
#define TIMER_SIGNAL 1
#define CHIME_SIGNAL 1
#define PULSE_LENGTH 500 //must be 500 for Proton
#define SELECTABLE_SIGNAL_TYPE 0 //must be 0 for Proton

#endif //CONFIG_ESP32_7SEG_PROTON_H
