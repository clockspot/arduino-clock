// Arduino AVR with HT16K33 (7seg), VEML7700, no RTC
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDAVR_7SEG_H
#define CONFIG_ARDAVR_7SEG_H

#define ENABLE_ALARM2 false

#define RTC_IS_MILLIS

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL A1
#define CTRL_ALT -1 //no Alt button
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A2
#define CTRL_DN A3

#define DISPLAY_HT16K33
#define BRIGHTNESS_FULL 15 //HT16K33 supports 0-15

#define LIGHTSENSOR_VEML7700
#define LIGHTSENSOR

#endif //CONFIG_ARDAVR_7SEG_H
