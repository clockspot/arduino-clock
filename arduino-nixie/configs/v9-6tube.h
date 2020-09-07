//UNDB v9, relay disabled, buttons as labeled, with 6-digit display.
//Also for v8 modified to v9 spec (Sel/Alt on A6/A7, Up/Down on A0/A1, relay on A3, led on 9, and cathode B4 on A2)

#define DISPLAY_SIZE 6 //number of tubes in display module. Small display adjustments are made for 4-tube clocks

// Which functionality is enabled in this clock?
// Related options will also be enabled in the options menu.
#define ENABLE_DATE_FN true // Date function, optionally including pages below
#define ENABLE_DATE_COUNTER true // Adds date page with an anniversary counter
#define ENABLE_DATE_RISESET true // Adds date pages with sunrise/sunset times. Requires DM Kichi's Dusk2Dawn library to be installed in IDE.
#define ENABLE_ALARM_FN true
#define ENABLE_ALARM_AUTOSKIP true
#define ENABLE_ALARM_FIBONACCI true
#define ENABLE_TIMER_FN true
#define ENABLE_TIME_CHIME true
#define ENABLE_SHUTOFF_NIGHT true // If disabled, tubes will be full brightness all the time.
#define ENABLE_SHUTOFF_AWAY true // Requires night shutoff.
#define ENABLE_TEMP_FN false //Temperature per DS3231 - will read high – leave false for production
#define ENABLE_TUBETEST_FN false //Cycles through all tubes – leave false for production

// These are the RLB board connections to Arduino analog input pins.
// S1/PL13 = Reset
// S2/PL5 = A1
// S3/PL6 = A0
// S4/PL7 = A6
// S5/PL8 = A3
// S6/PL9 = A2
// S7/PL14 = A7
// A6-A7 are analog-only pins that aren't quite as responsive and require a physical pullup resistor (1K to +5V), and can't be used with rotary encoders because they don't support pin change interrupts.

// What input is associated with each control?
#define CTRL_SEL A6 //main select button - must be equipped
#define CTRL_UP A0 //main up/down buttons or rotary encoder - must be equipped
#define CTRL_DN A1
#define CTRL_ALT A7 //alt select button - if unequipped, set to 0

// What type of up/down controls are equipped?
// 1 = momentary buttons. 2 = quadrature rotary encoder: requires Paul Stoffregen's Encoder library to be installed in IDE.
#define CTRL_UPDN_TYPE 1
#define ROT_VEL_START 80 //Required if CTRL_UPDN_TYPE==2. If step rate falls below this, kick into high velocity set (x10)
#define ROT_VEL_STOP 500 //Required if CTRL_UPDN_TYPE==2. If encoder step rate rises above this, drop into low velocity set (x1)

// How long (in ms) are the button hold durations?
#define CTRL_HOLD_SHORT_DUR 1000 //for entering setting mode, or hold-setting at low velocity (x1)
#define CTRL_HOLD_LONG_DUR 3000 //for entering options menu, or hold-setting at high velocity (x10)

//What are the signal pin(s) connected to?
#define PIEZO_PIN 10
#define RELAY_PIN -1 // -1 to disable feature (no relay item equipped); A3 if equipped (UNDB v9)
#define RELAY_MODE 0 //If relay is equipped, what does it do?
// 0 = switched mode: the relay will be switched to control an appliance like a radio or light fixture. If used with timer, it will switch on while timer is running (like a "sleep" function). If used with alarm, it will switch on when alarm trips; specify duration of this in SWITCH_DUR.
// 1 = pulsed mode: the relay will be pulsed, like the beeper is, to control an intermittent signaling device like a solenoid or indicator lamp. Specify pulse duration in RELAY_PULSE.
#define SIGNAL_DUR 180 //sec - when pulsed signal is going, pulses are sent once/sec for this period (e.g. 180 = 3min)
#define SWITCH_DUR 7200 //sec - when alarm triggers switched relay, it's switched on for this period (e.g. 7200 = 2hr)
#define PIEZO_PULSE 250 //ms - used with piezo via tone()
#define RELAY_PULSE 200 //ms - used with pulsed relay

//Soft power switches
#define ENABLE_SOFT_ALARM_SWITCH 1
// 1 = yes. Alarm can be switched on and off when clock is displaying the alarm time (fnIsAlarm).
// 0 = no. Alarm will be permanently on. Use with switched relay if the appliance has its own switch on this relay circuit.
#define ENABLE_SOFT_POWER_SWITCH 1 //works with switched relay only
// 1 = yes. Relay can be switched on and off directly with Alt button at any time (except in options menu). This is useful if connecting an appliance (e.g. radio) that doesn't have its own switch, or if replacing the clock unit in a clock radio where the clock does all the switching (e.g. Telechron).
// 0 = no. Use if the connected appliance has its own power switch (independent of this relay circuit) or does not need to be manually switched. In this case (and/or if there is no switched relay) Alt will act as a function preset.

//LED circuit control
#define LED_PIN 9 // -1 to disable feature; 9 if equipped (UNDB v9)

//When display is dim/off, a press will light the tubes for how long?
#define UNOFF_DUR 10 //sec

// What is the "frame rate" of the tube cleaning and display scrolling? up to 65535 ms
#define CLEAN_SPEED 200 //ms
#define SCROLL_SPEED 100 //ms - e.g. scroll-in-and-out date at :30

// What are the timeouts for setting and temporarily-displayed functions? up to 65535 sec
#define SETTING_TIMEOUT 300 //sec
#define FN_TEMP_TIMEOUT 5 //sec
#define FN_PAGE_TIMEOUT 3 //sec

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
#define OUT_A1 2
#define OUT_A2 3
#define OUT_A3 4
#define OUT_A4 5
#define OUT_B1 6
#define OUT_B2 7
#define OUT_B3 8
#define OUT_B4 16 //aka A2
//3 pins out to anode channel switches
#define ANODE_1 11
#define ANODE_2 12
#define ANODE_3 13