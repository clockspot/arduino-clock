// Arduino AVR with UNDBv8 (6-digit nixie), DS3231, VEML7700
// See configs/defaults.h for the full option list and per-option docs.

#ifndef CONFIG_ARDAVR_UNDB8_H
#define CONFIG_ARDAVR_UNDB8_H

#define ENABLE_ALARM2 false

#define RTC_IS_DS3231

#define INPUT_SIMPLE
#define INPUT_BUTTONS
#define CTRL_SEL A1 //UNDB S2/PL5
#define CTRL_ALT A0 //UNDB S3/PL6
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A2 //UNDB S6/PL9
#define CTRL_DN A3 //UNDB S5/PL8
//Unused: A7 (UNDB S7/PL14), A6 (UNDB S4/PL7)

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

#endif //CONFIG_ARDAVR_UNDB8_H
