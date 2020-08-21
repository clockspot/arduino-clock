//UNDB v5, 4-tube display

const byte displaySize = 4; //number of tubes in display module. Small display adjustments are made for 4-tube clocks

// Which functionality is enabled in this clock?
// Related options will also be enabled in the options menu.
const bool enableDate = true;
const bool enableDateCounter = true; // Adds a "page" to the date with an anniversary counter
const bool enableDateSunriseSunset = true; // Adds "pages" to the date with sunrise/sunset times
const bool enableAlarm = true;
const bool enableAlarmAutoskip = true;
const bool enableAlarmFibonacci = true;
const bool enableTimer = true;
const bool enableChime = true;
const bool enableNightShutoff = true; // If disabled, tubes will be full brightness all the time.
const bool enableAwayShutoff = true; // Requires night shutoff.
const bool enableTemp = false; //Temperature per DS3231 - will read high – leave false for production
const bool enableTest = false; //Cycles through all tubes – leave false for production

// These are the UNDB v5 board connections to Arduino analog input pins.
// S1/PL13 = Reset
// S2/PL5 = A1
// S3/PL6 = A0
// S4/PL7 = A6
// S5/PL8 = A3
// S6/PL9 = A2
// S7/PL14 = A7
// A6-A7 are analog-only pins that aren't quite as responsive and require a physical pullup resistor (1K to +5V), and can't be used with rotary encoders because they don't support pin change interrupts.

// What input is associated with each control?
const byte mainSel = A2; //main select button - must be equipped
const byte mainAdjUp = A1; //main up/down buttons or rotary encoder - must be equipped
const byte mainAdjDn = A0;
const byte altSel = 0; //alt select button - if unequipped, set to 0

// What type of adj controls are equipped?
// 1 = momentary buttons. 2 = quadrature rotary encoder.
const byte mainAdjType = 2;

//What are the signal pin(s) connected to?
const char piezoPin = 10;
const char relayPin = -1; //don't change - not available until UNDB v8
const byte relayMode = 0; //don't change - not available until UNDB v8
const word signalDur = 180; //sec - when pulsed signal is going, pulses are sent once/sec for this period (e.g. 180 = 3min)
const word switchDur = 7200; //sec - when alarm triggers switched relay, it's switched on for this period (e.g. 7200 = 2hr)
const word piezoPulse = 250; //ms - used with piezo via tone()
const word relayPulse = 200; //ms - used with pulsed relay

//Soft power switches
const byte enableSoftAlarmSwitch = 1;
// 1 = yes. Alarm can be switched on and off when clock is displaying the alarm time (fnIsAlarm).
// 0 = no. Alarm will be permanently on. Use with switched relay if the appliance has its own switch on this relay circuit.
const byte enableSoftPowerSwitch = 0; //don't change - not available until UNDB v8

//LED circuit control
const char ledPin = -1; //don't change - not available until UNDB v8

//When display is dim/off, a press will light the tubes for how long?
const byte unoffDur = 10; //sec

// How long (in ms) are the button hold durations?
const word btnShortHold = 1000; //for entering setting mode, or hold-setting at low velocity
const word btnLongHold = 3000; //for entering options menu, or hold-setting at high velocity
const word velThreshold = 0; //ms
// When an adj up/down input (btn or rot) follows another in less than this time, value will change more (10 vs 1).
// 0 to disable. Recommend ~150 for rotaries. If you want to use this feature with buttons, extend to ~300.

// What is the "frame rate" of the tube cleaning and display scrolling? up to 65535 ms
const word cleanSpeed = 200; //ms
const word scrollSpeed = 100; //ms - e.g. scroll-in-and-out date at :30 - to give the illusion of a slow scroll that doesn't pause, use (timeoutTempFn*1000)/(displaySize+1) - e.g. 714 for displaySize=6 and timeoutTempFn=5

// What are the timeouts for setting and temporarily-displayed functions? up to 65535 sec
const unsigned long timeoutSet = 300; //sec
const unsigned long timeoutTempFn = 5; //sec

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
const char outA1 = 2;
const char outA2 = 3;
const char outA3 = 4;
const char outA4 = 5;
const char outB1 = 6;
const char outB2 = 7;
const char outB3 = 8;
const char outB4 = 9;
//3 pins out to anode channel switches
const char anode1 = 11;
const char anode2 = 12;
const char anode3 = 13;