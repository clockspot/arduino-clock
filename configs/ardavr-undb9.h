// Arduino AVR with UNDBv9 (6-digit nixie), DS3231, relay disabled
// Also for v8 modified to v9 spec (Sel/Alt on A6/A7, Up/Down on A0/A1, relay on A3, led on 9, and cathode B4 on A2)
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDAVR_UNDB9_H
#define CONFIG_ARDAVR_UNDB9_H

#define ENABLE_ALARM2 false

#define RTC_DS3231

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL A6 //UNDB S4/PL7
#define CTRL_ALT A7 //UNDB S7/PL14
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A0 //UNDB S3/PL6
#define CTRL_DN A1 //UNDB S2/PL5
//Unused: A3 (UNDB S5/PL8), A2 (UNDB S6/PL9)

#define DISPLAY_NIXIE
#define OUT_A1 2
#define OUT_A2 3
#define OUT_A3 4
#define OUT_A4 5
#define OUT_B1 6
#define OUT_B2 7
#define OUT_B3 8
#define OUT_B4 16 //aka A2
#define ANODE_1 11
#define ANODE_2 12
#define ANODE_3 13

#define PIEZO_PIN 10
#define BACKLIGHT_PIN 9 //UNDB v9 backlight LED

#endif //CONFIG_ARDAVR_UNDB9_H
