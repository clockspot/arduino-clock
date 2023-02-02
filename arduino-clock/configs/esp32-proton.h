//ESP32 driving Proton 320 radio, with I2C for HT16K33, DS3231, VEML7700

#ifndef CONFIG
#define CONFIG


///// Functionality /////

// Which functionality is enabled in this clock?
// Related settings will also be enabled in the settings menu.
// The operating instructions assume all of these are enabled except thermometer and tubetest.
#define ENABLE_DATE true
#define ENABLE_DAY_COUNTER true //requires date
#define ENABLE_SUN true //requires date
#define ENABLE_WEATHER true //requires date
#define ENABLE_ALARM true
#define ENABLE_ALARM2 true //requires alarm
#define ENABLE_ALARM_AUTOSKIP true //requires alarm
#define ENABLE_ALARM_FIBONACCI true //requires alarm //TEST is skipped?
#define ENABLE_TIMER true //required for proton (sleep)
#define ENABLE_CHIME true //TEST is skipped?
#define ENABLE_DIMMING true
#define ENABLE_AWAYMODE true
#define ENABLE_THERMOMETER false //Temperature per DS3231 - will read high – leave false for production //formerly ENABLE_TEMP_FN //TODO test by tagging on end
#define ENABLE_TUBETEST false //Cycles through all tubes – leave false for production //formerly ENABLE_TUBETEST_FN


///// Real-Time Clock /////

#define RTC_IS_DS3231


///// Inputs /////

//TODO add this to the other files, and possibly rename INPUT_BUTTONS, INPUT_UPDN_BUTTONS, INPUT_UPDN_ROTARY, INPUT_IMU to INPUT_SIMPLE_...
//If using Simple controls
// #define INPUT_SIMPLE

//If using Proton 320 radio controls
#define INPUT_PROTON
//down left side of ESP32:
//momentary except for AL1/AL2/AL1R/AL2R/SW1/SW2
//34, 36, 39 are input only
#define CTRL_OFF 36
#define CTRL_ON 39
#define CTRL_AL1 34
#define CTRL_AL2 32
#define CTRL_AL1R 33
#define CTRL_AL2R 25
#define CTRL_SNOOZE 26
#define CTRL_SLEEP 27
#define CTRL_UP 14
#define CTRL_DN 12
#define CTRL_SW0 13
#define CTRL_SW1 15 //on bottom right. Combo of these two, I think, reflect rear switch position (lock/time/alarm/date)

//TODO some of this may end up being specific to just Simple controls
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


///// Outputs /////

//down right side of ESP32:
// #define PIN_SPI_MOSI 23
#define PIN_I2C_SCL 22
// #define PIN_TX 1
// #define PIN_RX 3
#define PIN_I2C_SDA 21
// #define PIN_SPI_MISO 19
// #define PIN_SPI_SCK 18
#define PIN_I2C_IO  5 //TODO is this ok?
// #define PIN_SPI_SS 5 //TODO is this real?
//4 is switch pin below
//0 is unused
//2 is unused

///// Display /////
//If using 4/6-digit 7-segment LED display with HT16K33 (I2C on SDA/SCL pins)
//Requires Adafruit libraries LED Backpack, GFX, and BusIO
//If 6 digits, edit Adafruit_LEDBackpack.cpp to replace "if (d > 4)" with "if (d > 6)"
//and, if desired, in numbertable[], replace 0x7D with 0x7C and 0x6F with 0x67 to remove
//the serifs from 6 and 9 for legibility (see http://www.harold.thimbleby.net/cv/files/seven-segment.pdf)
#define DISPLAY_HT16K33
#define SEVENSEG //enables display of letters in some cases
//#define NUM_MAX 4 //How many digits?
#define BRIGHTNESS_FULL 15 //out of 0-15
#define BRIGHTNESS_DIM 0
#define DISPLAY_ADDR 0x70 //0x70 is the default

//For all display types:
#define DISPLAY_SIZE 6 //number of digits in display module: 6 or 4
#define UNOFF_DUR 10 //sec - when display is off, an input will illuminate for how long?
#define SCROLL_SPEED 100 //ms - "frame rate" of digit scrolling, e.g. date at :30 option


///// Ambient Light Sensor /////
//If using VEML 7700 Lux sensor (I2C on SDA/SCL pins)
//Requires Adafruit library VEML7700
#define LIGHTSENSOR_VEML7700
#define LUX_FULL 400 //lux at/above which display should be at its brightest (per config)
#define LUX_DIM 30 //lux at/below which display should be at its dimmest (per config)

//If any type of light sensor is in use:
#define LIGHTSENSOR


///// Other Outputs /////

//What are the pins for each signal type? -1 to disable that signal type //TODO this should simply be defined in main code if omitted
#define PIEZO_PIN -1 //Drives a piezo beeper
#define SWITCH_PIN 4 //Switched to control an appliance like a radio or light fixture. If used with timer, it will switch on while timer is running (like a "sleep" function). If used with alarm, it will switch on when alarm trips; specify duration of this in SWITCH_DUR. (A3 for UNDB v9)
#define PULSE_PIN -1 //Simple pulses to control an intermittent signaling device like a solenoid or indicator lamp. Specify pulse duration in RELAY_PULSE. Pulse frequency behaves like the piezo signal.
//Default signal type for each function:
//0=piezo, 1=switch, 2=pulse
#define ALARM_SIGNAL 1
#define TIMER_SIGNAL 1
#define CHIME_SIGNAL 1
#define SIGNAL_DUR 180 //sec - when piezo/pulse signal is going, it's pulsed once/sec for this period (e.g. 180 = 3min)
#define SWITCH_DUR 7200 //sec - when alarm triggers switch signal, it's switched on for this period (e.g. 7200 = 2hr)
#define PULSE_LENGTH 200 //ms - length of pulse signal's individual pulses (e.g. to drive a solenoid to ring a bell)

//Soft power switches - required for Proton
#define ENABLE_SOFT_ALARM_SWITCH 1
#define ENABLE_SOFT_POWER_SWITCH 1

//Backlighting control
#define BACKLIGHT_PIN -1 // -1 to disable feature; 9 if equipped (UNDB v9)
#define BACKLIGHT_FADE 0 // 1 to fade via PWM (must use PWM pin and PWM-supportive lighting); 0 to simply switch on and off


#endif