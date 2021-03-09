//UNDB v9, relay disabled, buttons as labeled, with 6-digit display.
//Also for v8 modified to v9 spec (Sel/Alt on A6/A7, Up/Down on A0/A1, relay on A3, led on 9, and cathode B4 on A2)

#ifndef CONFIG
#define CONFIG


///// Functionality /////

// Which functionality is enabled in this clock?
// Related settings will also be enabled in the settings menu.
// The operating instructions assume all of these are enabled except temp and tubetest.
#define ENABLE_DATE_FN true // Date function, optionally including pages below
#define ENABLE_DATE_COUNTER false // Adds date page with an anniversary counter
#define ENABLE_DATE_RISESET false // Adds date pages with sunrise/sunset times. Requires DM Kichi's Dusk2Dawn library to be installed in IDE.
#define ENABLE_ALARM_FN true
#define ENABLE_ALARM_AUTOSKIP false
#define ENABLE_ALARM_FIBONACCI false
#define ENABLE_TIMER_FN false
#define ENABLE_TIME_CHIME true
#define ENABLE_SHUTOFF_NIGHT true // If disabled, tubes will be full brightness all the time.
#define ENABLE_SHUTOFF_AWAY false // Requires night shutoff.
#define ENABLE_TEMP_FN false //Temperature per DS3231 - will read high – leave false for production
#define ENABLE_TUBETEST_FN false //Cycles through all tubes – leave false for production


///// Real-Time Clock /////
//These are mutually exclusive

//If using DS3231 (via I2C):
//Requires Wire library (standard Arduino)
//Requires DS3231 library by NorthernWidget to be installed in your IDE.
#define RTC_DS3231


///// Inputs /////

//If using buttons for Select and optionally Alt:
#define INPUT_BUTTONS
#define CTRL_SEL A6 //UNDB S4/PL7
#define CTRL_ALT A7 //UNDB S7/PL14 - if not using Alt, set to 0

//Up and Down can be buttons OR a rotary control:

//If using buttons for Up and Down:
#define INPUT_UPDN_BUTTONS
#define CTRL_UP A0 //UNDB S3/PL6
#define CTRL_DN A1 //UNDB S2/PL5

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
//A3 //UNDB S5/PL8
//A2 //UNDB S6/PL9


///// Display /////
//These are mutually exclusive

//If using nixie array:
#define DISP_NIXIE
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
#define OUT_B4 16 //aka A2
//3 pins out to anode channel switches
#define ANODE_1 11
#define ANODE_2 12
#define ANODE_3 13

//For all display types:
#define DISPLAY_SIZE 6 //number of digits in display module: 6 or 4
#define UNOFF_DUR 10 //sec - when display is off, an input will illuminate for how long?
#define SCROLL_SPEED 100 //ms - "frame rate" of digit scrolling, e.g. date at :30 option


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
// 1 = yes. Alarm can be switched on and off when clock is displaying the alarm time (fnIsAlarm).
// 0 = no. Alarm will be permanently on. Use with switch signal if the appliance has its own switch on this circuit (and note that, if another signal type(s) is available and selected for the alarm, the user won't be able to switch it off). Also disables skip feature. Note that the instructions do not reflect this option.
#define ENABLE_SOFT_POWER_SWITCH 1 //switch signal only
// 1 = yes. Switch signal can be toggled on and off directly with Alt button at any time (except in settings menu). This is useful if connecting an appliance (e.g. radio) that doesn't have its own switch, or if replacing the clock unit in a clock radio where the clock does all the switching (e.g. Telechron).
// 0 = no. Use if the connected appliance has its own power switch (independent of this circuit, e.g. some Sony Digimatic clock radios) or does not need to be manually switched. In this case (and/or if there is no switch signal option, and if no Wi-Fi support) Alt will act as a function preset. Note that the instructions do not reflect this option.

//Backlighting control
#define BACKLIGHT_PIN 9 // -1 to disable feature; 9 if equipped (UNDB v9)
#define BACKLIGHT_FADE 0
// 0 = no fading; simply switches on and off.
// >0 = backlight fades on and off via PWM (must use PWM pin and PWM-supportive lighting, such as LEDs). This value is the amount the PWM is increased/decreased per loop cycle. 10 is a good starting choice.


#endif