// Arduino AVR with MAX7219 (matrix), DS3231, VEML7700
// Can be used with inputs/piezos on UNDB but won't drive nixies
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDAVR_MATRIX_H
#define CONFIG_ARDAVR_MATRIX_H

#define ENABLE_ALARM2 false

#define RTC_IS_DS3231

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL A1
#define CTRL_ALT A0
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A2
#define CTRL_DN A3

#define DISPLAY_MAX7219
//7 and 0 make the least noise on this clock
#define CLK_PIN 2 //D2, pin 20
#define CS_PIN 3  //D3, pin 21
#define DIN_PIN 4 //D4, pin 22

#define LIGHTSENSOR_VEML7700
#define LIGHTSENSOR

#define PIEZO_PIN 10

#endif //CONFIG_ARDAVR_MATRIX_H
