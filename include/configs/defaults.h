// Documentation and defaults for arduino-clock configuration options.
//
// This file is included AFTER the user's hardware config (chosen via config.h),
// so anything the hardware config left undefined will pick up the default here.
// Hardware configs override values they need to differ; mutual-exclusion guards
// at the end of each section enforce that hardware identity choices (RTC type,
// display type, etc.) were made.
//
// When adding a new option to the project: add it here first with its default
// and a comment, then reference it from src/. Per-option docs live in this
// file — keep hardware configs lean.

#ifndef DEFAULTS_H
#define DEFAULTS_H


///// Functionality /////
// Which functionality is enabled in this clock?
// Related settings will also be enabled in the settings menu.
// The operating instructions assume all of these are enabled except thermometer and tubetest.

#ifndef ENABLE_DATE
  #define ENABLE_DATE true
#endif
#ifndef ENABLE_DAY_COUNTER
  #define ENABLE_DAY_COUNTER true //requires date
#endif
#ifndef ENABLE_SUN
  #define ENABLE_SUN true //requires date and Dusk2Dawn library
#endif
#ifndef ENABLE_WEATHER
  #define ENABLE_WEATHER false //requires date and network //WIP
#endif
#ifndef ENABLE_ALARM
  #define ENABLE_ALARM true
#endif
#ifndef ENABLE_ALARM2
  #define ENABLE_ALARM2 true //requires alarm
#endif
#ifndef ENABLE_ALARM_AUTOSKIP
  #define ENABLE_ALARM_AUTOSKIP true //requires alarm
#endif
#ifndef ENABLE_ALARM_FIBONACCI
  #define ENABLE_ALARM_FIBONACCI true //requires alarm
#endif
#ifndef ENABLE_TIMER
  #define ENABLE_TIMER true //required for proton (sleep)
#endif
#ifndef ENABLE_CHIME
  #define ENABLE_CHIME true
#endif
#ifndef ENABLE_DIMMING
  #define ENABLE_DIMMING true
#endif
#ifndef ENABLE_AWAYMODE
  #define ENABLE_AWAYMODE true
#endif
#ifndef ENABLE_THERMOMETER
  #define ENABLE_THERMOMETER false //Temperature per DS3231 - will read high - leave false for production
#endif
#ifndef ENABLE_TUBETEST
  #define ENABLE_TUBETEST false //Cycles through all tubes - leave false for production
#endif


///// Real-Time Clock /////
// Mutually exclusive — hardware config must define exactly one:
//   RTC_IS_DS3231 - I2C DS3231 (requires Wire + RTClib)
//   RTC_IS_MILLIS - software clock based on millis() (no hardware required)
//   RTC_IS_ZERO   - RTCZero on Nano 33 IoT (TODO, not yet implemented)

#if (defined(RTC_IS_DS3231) + defined(RTC_IS_MILLIS) + defined(RTC_IS_ZERO)) != 1
  #error "Pick exactly one RTC: RTC_IS_DS3231, RTC_IS_MILLIS, or RTC_IS_ZERO."
#endif

#ifndef ANTI_DRIFT
  #define ANTI_DRIFT 0 //msec to add/remove per second - or seconds to add/remove per day divided by 86.4 - to compensate for natural drift. If using wifinina, it really only needs to be good enough for a decent timekeeping display until the next ntp sync. TIP: setting to a superhigh value is helpful for testing! e.g. 9000 will make it run 10x speed
#endif


///// Inputs /////
// Top-level input scheme (selects which input module to compile):
//   INPUT_SIMPLE - Sel/Alt/Up/Dn via buttons, rotary, or IMU (src/inputSimple.cpp)
//   INPUT_PROTON - Proton 320 clock radio buttons + switches (src/inputProton.cpp)
// (No #error guard yet: legacy configs may not declare either; TODO normalize.)
//
// Within INPUT_SIMPLE, controls are mutually exclusive:
//   INPUT_BUTTONS  - Sel/Alt buttons (with INPUT_UPDN_BUTTONS or INPUT_UPDN_ROTARY for Up/Dn)
//   INPUT_IMU      - tilt-based via Nano 33 IoT IMU (no buttons)

//Hold-button durations and timeouts apply to all input schemes.
#ifndef CTRL_HOLD_SHORT_DUR
  #define CTRL_HOLD_SHORT_DUR 1000 //ms - for entering setting mode, or hold-setting at low velocity (x1)
#endif
#ifndef CTRL_HOLD_LONG_DUR
  #define CTRL_HOLD_LONG_DUR 3000 //ms - for entering settings menu, or hold-setting at high velocity (x10)
#endif
#ifndef CTRL_HOLD_VERYLONG_DUR
  #define CTRL_HOLD_VERYLONG_DUR 5000 //ms - for wifi info / admin start (Nano IoT without Alt only)
#endif
#ifndef CTRL_HOLD_SUPERLONG_DUR
  #define CTRL_HOLD_SUPERLONG_DUR 10000 //ms - for wifi disconnect (Nano IoT) or EEPROM reset on startup
#endif
#ifndef SETTING_TIMEOUT
  #define SETTING_TIMEOUT 300 //sec
#endif
#ifndef FN_TEMP_TIMEOUT
  #define FN_TEMP_TIMEOUT 5 //sec
#endif
#ifndef FN_PAGE_TIMEOUT
  #define FN_PAGE_TIMEOUT 3 //sec
#endif

//Rotary control velocity thresholds (only used when INPUT_UPDN_ROTARY is set):
#ifndef ROT_VEL_START
  #define ROT_VEL_START 80 //If step rate falls below this, kick into high velocity set (x10)
#endif
#ifndef ROT_VEL_STOP
  #define ROT_VEL_STOP 500 //If step rate rises above this, drop into low velocity set (x1)
#endif

// IMU orientation flags (only used when INPUT_IMU is set):
// Include one each of USB_DIR_* and IC_DIR_* to indicate which way the USB
// port and IC (front side) are oriented. For UNDB clocks: USB_DIR_UP and IC_DIR_BACK.
//   USB_DIR_UP / DOWN / LEFT / RIGHT / FRONT / BACK
//   IC_DIR_UP  / DOWN / LEFT / RIGHT / FRONT / BACK


///// Display /////
// Mutually exclusive — hardware config must define exactly one:
//   DISPLAY_NIXIE   - SN74141-multiplexed nixie array (src/displayNixie.cpp)
//   DISPLAY_MAX7219 - SPI MAX7219 8x8 LED matrix (src/displayMAX7219.cpp; requires LedControl library)
//   DISPLAY_HT16K33 - I2C HT16K33 7-segment LED display (src/displayHT16K33.cpp; requires Adafruit LED Backpack/GFX/BusIO)

#if (defined(DISPLAY_NIXIE) + defined(DISPLAY_MAX7219) + defined(DISPLAY_HT16K33)) != 1
  #error "Pick exactly one display: DISPLAY_NIXIE, DISPLAY_MAX7219, or DISPLAY_HT16K33."
#endif

//Nixie-specific (DISPLAY_NIXIE):
//Output pins: 2x3 multiplexed - two tubes powered at a time. Anode channel determines which two
//tubes are powered; the two SN74141 cathode driver chips determine which digits are lit.
//4 pins to each SN74141 representing a binary number with values [1,2,4,8]; 3 pins to anode switches.
//OUT_A1..A4, OUT_B1..B4, ANODE_1..3 must be set per board.
#ifndef CLEAN_SPEED
  #define CLEAN_SPEED 200 //ms - "frame rate" of tube cleaning
#endif

//MAX7219-specific (DISPLAY_MAX7219):
//Output pins (CLK_PIN, CS_PIN, DIN_PIN) must be set per board.
#ifndef NUM_MAX
  #define NUM_MAX 4 //How many modules? 3 for 8x24 (4 digit, untested) or 4 for 8x32 (6 digit)
#endif
#ifndef ROTATE
  #define ROTATE 90
#endif

//HT16K33-specific (DISPLAY_HT16K33):
//If 6 digits, edit Adafruit_LEDBackpack.cpp to replace "if (d > 4)" with "if (d > 6)"
//and, if desired, in numbertable[], replace 0x7D with 0x7C and 0x6F with 0x67 to remove
//the serifs from 6 and 9 for legibility (see http://www.harold.thimbleby.net/cv/files/seven-segment.pdf)
#ifndef DISPLAY_ADDR
  #define DISPLAY_ADDR 0x70 //I2C address - 0x70 is the default
#endif

//Brightness defaults differ between display types; hardware configs typically override these.
#ifndef BRIGHTNESS_FULL
  #define BRIGHTNESS_FULL 7 //out of 0-15 (MAX7219); HT16K33 typically 15
#endif
#ifndef BRIGHTNESS_DIM
  #define BRIGHTNESS_DIM 0
#endif

//For all display types:
#ifndef DISPLAY_SIZE
  #define DISPLAY_SIZE 6 //number of digits in display module: 6 or 4
#endif
#ifndef UNOFF_DUR
  #define UNOFF_DUR 10 //sec - when display is off, an input will illuminate for how long?
#endif
#ifndef SCROLL_SPEED
  #define SCROLL_SPEED 100 //ms - "frame rate" of digit scrolling, e.g. date at :30 option
#endif


///// Ambient Light Sensor /////
// Optional. Currently one type supported:
//   LIGHTSENSOR_VEML7700 - Adafruit VEML7700 lux sensor (I2C, requires Adafruit VEML7700 library)
// If any light sensor is in use, hardware config should also #define LIGHTSENSOR.

#ifndef LUX_FULL
  #define LUX_FULL 400 //lux at/above which display should be at its brightest
#endif
#ifndef LUX_DIM
  #define LUX_DIM 30 //lux at/below which display should be at its dimmest
#endif


///// Other Outputs /////

//Pins for each signal type. -1 disables.
#ifndef PIEZO_PIN
  #define PIEZO_PIN -1 //Drives a piezo beeper
#endif
#ifndef SWITCH_PIN
  #define SWITCH_PIN -1 //Switched to control an appliance like a radio or light fixture. If used with timer, it will switch on while timer is running (like a "sleep" function). If used with alarm, it will switch on when alarm trips; specify duration in SWITCH_DUR. (A3 for UNDB v9)
#endif
#ifndef PULSE_PIN
  #define PULSE_PIN -1 //Simple pulses to control an intermittent signaling device like a solenoid or indicator lamp. Specify pulse duration in PULSE_LENGTH. Pulse frequency behaves like the piezo signal.
#endif

//Default signal type for each function: 0=piezo, 1=switch, 2=pulse
#ifndef ALARM_SIGNAL
  #define ALARM_SIGNAL 0
#endif
#ifndef TIMER_SIGNAL
  #define TIMER_SIGNAL 0
#endif
#ifndef CHIME_SIGNAL
  #define CHIME_SIGNAL 0
#endif

#ifndef SIGNAL_DUR
  #define SIGNAL_DUR 180 //sec - when piezo/pulse signal is going, it's pulsed once/sec for this period (e.g. 180 = 3min)
#endif
#ifndef SWITCH_DUR
  #define SWITCH_DUR 7200 //sec - when alarm triggers switch signal, it's switched on for this period (e.g. 7200 = 2hr)
#endif
#ifndef PULSE_LENGTH
  #define PULSE_LENGTH 200 //ms - length of pulse signal's individual pulses (e.g. to drive a solenoid to ring a bell)
#endif

//Soft power switches
#ifndef ENABLE_SOFT_ALARM_SWITCH
  #define ENABLE_SOFT_ALARM_SWITCH 1
  // 1 = yes. Alarm can be switched on and off when clock is displaying the alarm time (FN_ALARM).
  // 0 = no. Alarm will be permanently on. Use with switch signal if the appliance has its own switch on this circuit (and note that, if another signal type(s) is available and selected for the alarm, the user won't be able to switch it off). Also disables skip feature. Note that the instructions do not reflect this option.
#endif
#ifndef ENABLE_SOFT_POWER_SWITCH
  #define ENABLE_SOFT_POWER_SWITCH 1 //switch signal only
  // 1 = yes. Switch signal can be toggled on and off directly with Alt button at any time (except in settings menu). This is useful if connecting an appliance (e.g. radio) that doesn't have its own switch, or if replacing the clock unit in a clock radio where the clock does all the switching (e.g. Telechron).
  // 0 = no. Use if the connected appliance has its own power switch (independent of this circuit, e.g. some Sony Digimatic clock radios) or does not need to be manually switched. In this case (and/or if there is no switch signal option, and if no Wi-Fi support) Alt will act as a function preset. Note that the instructions do not reflect this option.
#endif

//Backlighting control
#ifndef BACKLIGHT_PIN
  #define BACKLIGHT_PIN -1 // -1 to disable feature; 9 if equipped (UNDB v9)
#endif
#ifndef BACKLIGHT_FADE
  #define BACKLIGHT_FADE 0 // 1 to fade via PWM (must use PWM pin and PWM-supportive lighting); 0 to simply switch on and off
#endif


#endif //DEFAULTS_H
