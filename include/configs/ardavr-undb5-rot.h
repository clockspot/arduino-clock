// Arduino AVR with UNDBv5 (6-digit nixie), DS3231, rotary control
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDAVR_UNDB5_ROT_H
#define CONFIG_ARDAVR_UNDB5_ROT_H

#define ENABLE_ALARM2 false

#define RTC_IS_DS3231

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL A2
#define CTRL_ALT -1 //no Alt button
#define INPUT_UPDN_ROTARY //requires Encoder library by Paul Stoffregen
#define CTRL_UP A0
#define CTRL_DN A1
//Unused: A3 (UNDB S5/PL8)

#define DISPLAY_NIXIE
#define OUT_A1 2
#define OUT_A2 3
#define OUT_A3 4
#define OUT_A4 5
#define OUT_B1 6
#define OUT_B2 7
#define OUT_B3 8
#define OUT_B4 9
#define ANODE_1 11
#define ANODE_2 12
#define ANODE_3 13

#define PIEZO_PIN 10

#endif //CONFIG_ARDAVR_UNDB5_ROT_H
