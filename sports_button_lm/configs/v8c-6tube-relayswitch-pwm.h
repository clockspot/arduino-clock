//UNDB v8 modified to v9 spec (put Sel/Alt on A7/A6, Up/Down on A0/A1, relay on A3, led on 9, and cathode B4 on A2), relay disabled, Sel/Alt buttons reversed, with 6-digit display.

// What input is associated with each control?
const byte mainSel = A6;
const byte mainAdjUp = A0;
const byte mainAdjDn = A1;
const byte altSel = A7; //if not equipped, set to 0

// What type of adj controls are equipped?
// 1 = momentary buttons. 2 = quadrature rotary encoder (not currently supported).
const byte mainAdjType = 1;

//What are the signal pin(s) connected to?
const char piezoPin = 10;

// How long (in ms) are the button hold durations?
const word btnShortHold = 1000; //for setting the displayed feataure
const word btnLongHold = 3000; //for for entering options menu

const char ledPin = 9;

const char beepTone = 75;

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
const char outB4 = 16; //A2 - was 9 before PWM fix pt2
//3 pins out to anode channel switches
const char anode1 = 11;
const char anode2 = 12;
const char anode3 = 13;