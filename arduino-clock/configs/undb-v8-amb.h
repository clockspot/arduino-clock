//Unmodified UNDB v8 with LED and relay disabled, and buttons as labeled, with 6-digit display, with ambient light sensor

#ifndef CONFIG
#define CONFIG


///// Functionality /////

// Which functionality is enabled in this clock?
// Related settings will also be enabled in the settings menu.
// The operating instructions assume all of these are enabled except thermometer and tubetest.
#define ENABLE_DATE true
#define ENABLE_DAY_COUNTER false //requires date
#define ENABLE_SUN false //requires date and Dusk2Dawn library
#define ENABLE_WEATHER false //requires date and network //WIP
#define ENABLE_ALARM false
#define ENABLE_ALARM2 false //requires alarm
#define ENABLE_ALARM_AUTOSKIP false //requires alarm
#define ENABLE_ALARM_FIBONACCI false //requires alarm
#define ENABLE_TIMER false
#define ENABLE_CHIME false
#define ENABLE_DIMMING true
#define ENABLE_AWAYMODE true
#define ENABLE_THERMOMETER false //Temperature per DS3231 - will read high – leave false for production
#define ENABLE_TUBETEST false //Cycles through all tubes – leave false for production


///// Real-Time Clock /////
//These are mutually exclusive

//If using DS3231 (via I2C):
//Requires Wire library (standard Arduino)
//Requires DS3231 library by NorthernWidget to be installed in your IDE.
#define RTC_DS3231

//If using no RTC (a fake RTC based on millis()):
// #define RTC_MILLIS
// #define ANTI_DRIFT -700 //msec to add/remove per second - or seconds to add/remove per day divided by 86.4 - to compensate for natural drift. Ifusing wifinina, it really only needs to be good enough for a decent timekeeping display until the next ntp sync. TIP: setting to a superhigh value is helpful for testing! e.g. 9000 will make it run 10x speed


///// Inputs /////

//If using buttons for Select and optionally Alt:
#define INPUT_BUTTONS
#define CTRL_SEL A1 //UNDB S2/PL5
#define CTRL_ALT A0 //UNDB S3/PL6 - if not using Alt, set to -1

//Up and Down can be buttons OR a rotary control:

//If using buttons for Up and Down:
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A2 //UNDB S6/PL9
#define CTRL_DN A3 //UNDB S5/PL8

//For all input types:
//How long (in ms) are the hold durations?
#define CTRL_HOLD_SHORT_DUR 1000 //for entering setting mode, or hold-setting at low velocity (x1)
#define CTRL_HOLD_LONG_DUR 3000 //for entering settings menu, or hold-setting at high velocity (x10)
#define CTRL_HOLD_VERYLONG_DUR 5000 //for wifi info / admin start (Nano IoT without Alt only)
#define CTRL_HOLD_SUPERLONG_DUR 10000 //for wifi disconnect (Nano IoT) or EEPROM reset on startup
//What are the timeouts for setting and temporarily-displayed functions? up to 65535 sec
#define SETTING_TIMEOUT 300 //sec
#define FN_TEMP_TIMEOUT 5 //sec
#define FN_PAGE_TIMEOUT 3 //sec

//Unused inputs
//A7 //UNDB S7/PL14
//A6 //UNDB S4/PL7


///// Display /////
//These are mutually exclusive

//If using nixie array:
#define DISPLAY_NIXIE
#define CLEAN_SPEED 200 //ms - "frame rate" of tube cleaning
//Which output pins?
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
#define OUT_B4 9
//3 pins out to anode channel switches
#define ANODE_1 11
#define ANODE_2 12
#define ANODE_3 13

//For all display types:
#define DISPLAY_SIZE 6 //number of digits in display module: 6 or 4
#define UNOFF_DUR 10 //sec - when display is off, an input will illuminate for how long?
#define SCROLL_SPEED 100 //ms - "frame rate" of digit scrolling, e.g. date at :30 option


///// Ambient Light Sensor /////
// If using VEML 7700 Lux sensor (I2C on SDA/SCL pins)
// Requires Adafruit library VEML7700
#define LIGHTSENSOR_VEML7700
#define LUX_FULL 400 //lux at/above which display should be at its brightest (per config)
#define LUX_DIM 30 //lux at/below which display should be at its dimmest (per config)

// If any type of light sensor is in use:
#define LIGHTSENSOR


///// Other Outputs /////

//What are the pins for each signal type? -1 to disable that signal type
#define PIEZO_PIN 10 //Drives a piezo beeper
#define SWITCH_PIN -1 //Switched to control an appliance like a radio or light fixture. If used with timer, it will switch on while timer is running (like a "sleep" function). If used with alarm, it will switch on when alarm trips; specify duration of this in SWITCH_DUR. (A3 for UNDB v9)
#define PULSE_PIN -1 //Simple pulses to control an intermittent signaling device like a solenoid or indicator lamp. Specify pulse duration in RELAY_PULSE. Pulse frequency behaves like the piezo signal.
//Default signal type for each function:
//0=piezo, 1=switch, 2=pulse
#define ALARM_SIGNAL 0
#define TIMER_SIGNAL 0
#define CHIME_SIGNAL 0
#define SIGNAL_DUR 180 //sec - when piezo/pulse signal is going, it's pulsed once/sec for this period (e.g. 180 = 3min)
#define SWITCH_DUR 7200 //sec - when alarm triggers switch signal, it's switched on for this period (e.g. 7200 = 2hr)
#define PULSE_LENGTH 200 //ms - length of pulse signal's individual pulses (e.g. to drive a solenoid to ring a bell)

//Soft power switches
#define ENABLE_SOFT_ALARM_SWITCH 1
// 1 = yes. Alarm can be switched on and off when clock is displaying the alarm time (FN_ALARM).
// 0 = no. Alarm will be permanently on. Use with switch signal if the appliance has its own switch on this circuit (and note that, if another signal type(s) is available and selected for the alarm, the user won't be able to switch it off). Also disables skip feature. Note that the instructions do not reflect this option.
#define ENABLE_SOFT_POWER_SWITCH 1 //switch signal only
// 1 = yes. Switch signal can be toggled on and off directly with Alt button at any time (except in settings menu). This is useful if connecting an appliance (e.g. radio) that doesn't have its own switch, or if replacing the clock unit in a clock radio where the clock does all the switching (e.g. Telechron).
// 0 = no. Use if the connected appliance has its own power switch (independent of this circuit, e.g. some Sony Digimatic clock radios) or does not need to be manually switched. In this case (and/or if there is no switch signal option, and if no Wi-Fi support) Alt will act as a function preset. Note that the instructions do not reflect this option.

//Backlighting control
#define BACKLIGHT_PIN -1 // -1 to disable feature; 9 if equipped (UNDB v9)
#define BACKLIGHT_FADE 0 // 1 to fade via PWM (must use PWM pin and PWM-supportive lighting); 0 to simply switch on and off


#endif