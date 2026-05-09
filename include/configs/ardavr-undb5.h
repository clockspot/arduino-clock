// Arduino AVR with UNDBv5 (6-digit nixie), DS3231
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDAVR_UNDB5_H
#define CONFIG_ARDAVR_UNDB5_H

#define ENABLE_ALARM2 false

#define RTC_DS3231

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL A2 //UNDB S6/PL9
#define CTRL_ALT -1 //no Alt button
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A1 //UNDB S2/PL5
#define CTRL_DN A0 //UNDB S3/PL6

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

#endif //CONFIG_ARDAVR_UNDB5_H
