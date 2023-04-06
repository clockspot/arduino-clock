// Universal digital clock codebase for Arduino - https://github.com/clockspot/arduino-clock
// Sketch by Luke McKenzie (luke@theclockspot.com)
// Written to support RLB Designs’ Universal Nixie Driver Board
// Inspired by original sketch by Robin Birtles (rlb-designs.com) and Chris Gerekos

#include <arduino.h>
#include "arduino-clock.h"

////////// Software version //////////
const byte vMajor = 2;
const byte vMinor = 1;
const byte vPatch = 0;
const bool vDev = 1;

////////// Includes //////////

// These modules are used per the available hardware and features enabled in the config file.
// The disp and rtc options are mutually exclusive and define the same functions.
// Older (<v2) versions of the Arduino IDE preprocessor seem to #include without regard to #if blocks (see https://forum.arduino.cc/index.php?topic=134226.0), so in addition to these ifdefs (which are needed for IDE v2+), I also have ifdefs around the corresponding cpp code.

#if ENABLE_SUN //this probably doesn't work, per the above, but ¯\_(ツ)_/¯
  #include <Dusk2Dawn.h> //DM Kishi - unlicensed - install in your Arduino IDE if needed - test without
#endif
#include "storage.h" //for persistent storage - supports both AVR EEPROM and SAMD flash (including esp32? TODO find out)
#ifdef DISPLAY_NIXIE
  #include "dispNixie.h" //if DISPLAY_NIXIE is defined in config - for a SN74141-multiplexed nixie array
#endif
#ifdef DISPLAY_MAX7219
  #include "dispMAX7219.h" //if DISPLAY_MAX7219 is defined in config - for a SPI MAX7219 8x8 LED array
#endif
#ifdef DISPLAY_HT16K33
  #include "dispHT16K33.h" //if DISPLAY_HT16K33 is defined in config - for an I2C 7-segment LED display
#endif
#ifdef LIGHTSENSOR_VEML7700
  #include "lightsensorVEML7700.h" //if LIGHTSENSOR_VEML7700 is defined in config - for I2C VEML7700 lux sensor
#endif
#ifdef RTC_IS_DS3231
  #include "rtcDS3231.h" //if RTC_IS_DS3231 is defined in config – for an I2C DS3231 RTC module
#endif
#ifdef RTC_IS_MILLIS
  #include "rtcMillis.h" //if RTC_IS_MILLIS is defined in config – for a fake RTC based on millis
#endif

#if defined(INPUT_SIMPLE)
  #include "inputSimple.h" //for Sel/Alt/Up/Dn - supports buttons, rotary control, and Nano 33 IoT IMU
#elif defined(INPUT_PROTON)
  #include "inputProton.h" //for a more diverse set of controls, namely the buttons and switches on a Proton 320 clock radio
#endif

#if defined(NETWORK_NINA)
  #include "networkNINA.h" //enables WiFi/web-based config/NTP sync on Nano 33 IoT WiFiNINA
#elif defined(NETWORK_ESP32)
  #include "networkESP32.h" //enables WiFi/web-based config/NTP sync on esp32 //TODO
#endif

#ifdef ENABLE_NEOPIXEL
  #include <Adafruit_NeoPixel.h>
  #define NUMPIXELS 1
  Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif


////////// Variables and storage //////////

/*Some variables are backed by persistent storage. These are referred to by their location in that storage. See storage.cpp. 

Values for these are bytes (0 to 255) or signed 16-bit ints (-32768 to 32767) where high byte is loc and low byte is loc+1. In updateDisplay(), special setting formats are deduced from the max value (max 1439 is a time of day, max 156 is a UTC offset, etc) and whether a value is an int or byte is deduced from whether the max value > 255.

IMPORTANT! If adding more variables, be sure to increase STORAGE_SPACE in storage.cpp (and use the ones marked [free] below first).

These ones are set outside the settings menu (defaults defined in initEEPROM() where applicable):
  0-1 Alarm time, mins
  152-153 Alarm2 time, mins //NEW
  2 Alarm on
  3 Alarm2 on //NEW
  4 Day count direction (also settable via settings menu, as below)
  5 Day count month
  6 Day count date
  7 Function preset (done by Alt when not power-switching)
  8 Functions/pages enabled (bitmask, dynamic) TODO
    const unsigned int FN_ENAB_TIMER = 1<<0; //1
    const unsigned int FN_ENAB_DAYCOUNT = 1<<1; //2
    const unsigned int FN_ENAB_SUN = 1<<2; //4
    const unsigned int FN_ENAB_WEATHER = 1<<3; //8
  9 NTP sync on
  15 DST on (last known state - set indirectly via time sets and such)
  51-54 NTP server IP address (4 bytes)
  55-86 Wi-Fi SSID (32 bytes)
  87-150 Wi-Fi WPA passphrase/key or WEP key (64 bytes)
  151 Wi-Fi WEP key index
  (152-153 are used above)

These ones are set inside the settings menu (defaults defined in arrays below).
Some are skipped when they wouldn't apply to a given clock's hardware config, see fnOptScroll(); these ones will also be set at startup to the start= values, see setup(). Otherwise, make sure these ones' defaults work for all configs.
TODO consider adding additional settings for Alarm2 – currently both alarms share same output characteristics (snooze, signal, pitch, pattern, fibonacci)
   4 Day count direction (also settable via direct setter, as above)
  10-11 Latitude
  12-13 Longitude
  14 UTC offset in quarter-hours plus 100 - range is 52 (-12h or -48qh, US Minor Outlying Islands) to 156 (+14h or +56qh, Kiribati)
  16 Time format
  17 Date format
  18 Display date during time
  19 Leading zeros in time hour, calendar, and chrono/timer
  20 Digit fade duration
  21 Strike - piezo or pulse signal only (start=0)
  22 Auto DST
  23 Alarm days
  24 Alarm snooze
  25 Alarm2 days //NEW opt 20 - formerly Timer interval mode (now a volatile var)
  26 Backlight behavior - skipped when no backlight pin
  27 Dimming (formerly night shutoff): 0=none (full on), 1=ambient, 2=dim per off-hours, 3=off per off-hours.
     Note: 1 is default, in case sensor is equipped; but if not, will shift to 2. (If dimming not enabled per config, will shift to 0.)
  28-29 Off-hours start, mins
  30-31 Off-hours end, mins
  32 Away mode: 0=none (full on aside from dim), 1=clock at work (shut off weekends), 2=clock at home (shut off work hours)
  33 First day of workweek
  34 Last day of workweek
  35-36 Work starts at, mins
  37-38 Work ends at, mins
  39 Alarm beeper pitch - piezo signal only
  40 Timer beeper pitch - piezo signal only
  41 Strike beeper pitch - piezo signal only
  42 Alarm signal (0=piezo, 1=switch, 2=pulse)
  43 Timer signal
  44 Strike signal
  45 Temperature format - skipped when !ENABLE_THERMOMETER TODO also useful for weather display
  46 Anti-cathode poisoning
  47 Alarm beeper pattern - piezo signal only
  48 Timer beeper pattern - piezo signal only
  49 Strike beeper pattern - piezo signal only
  50 Alarm Fibonacci mode
*/

//Settings menu numbers (displayed in UI and readme), locs, and default/min/max/current values.
//Setting numbers/order can be changed (though try to avoid for user convenience);
//but locs should be maintained so AVR EEPROM doesn't need reset after an upgrade (SAMD does it anyway).
//                       General                    Alarm                 Timer     Strike       Brightness                        Geo
const byte optsNum[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,20, 21,22,23, 30,31,32,33, 40,  41,  42,43,44,45,  46,  47,    50,   51, 52, 53};
const byte optsLoc[] = {16,17,18,19,20, 4,26,46,45, 23,42,39,47,24,50,25, 43,40,48, 21,44,41,49, 27,  28,  30,32,33,34,  35,  37,    10,   12, 14, 22};
const  int optsDef[] = { 2, 1, 0, 0, 5, 2, 1, 0, 0,  0, 0,76, 4, 9, 0, 0,  0,76, 2,  0, 0,68, 5,  0,1320, 360, 0, 1, 5, 480,1080,     0,    0,100,  0};
const  int optsMin[] = { 1, 1, 0, 0, 0, 0, 0, 0, 0,  0, 0,49, 0, 0, 0, 0,  0,49, 0,  0, 0,49, 0,  0,   0,   0, 0, 0, 0,   0,   0,  -900,-1800, 52,  0};
const  int optsMax[] = { 2, 5, 3, 1,20, 2, 4, 2, 1,  2, 2,88, 5,60, 1, 2,  2,88, 5,  4, 2,88, 5,  3,1439,1439, 2, 6, 6,1439,1439,   900, 1800,156,  6};
//TODO for Proton, may not want opt 6 (optLoc 4) if we tack that setting onto the date setter via the rear switch

//The rest of these variables are not backed by persistent storage, so they are regular named vars.

// Functions and pages
byte fn = 0; //currently displayed fn per above
byte getCurFn() { return fn; } //NEW
void setCurFn(byte val) { fn = val; } //NEW - the only setter we let input*.cpp call
void goToFn(byte val){ fn = val; setInputLast(); } //also sets inputLast as though per human activity
//TODO can we consolidate the above two, or is there a reason not to always setInputLast()?

byte fnSetPg = 0; //whether this function is currently being set, and which page it's on
bool getFnIsSetting() { return fnSetPg>0; }
 int fnSetVal; //the value currently being set, if any
 int fnSetValMin; //min possible
 int fnSetValMax; //max possible
bool fnSetValVel; //whether it supports velocity setting (if max-min > 30)
 int fnSetValDate[3]; //holder for newly set date, so we can set it in 3 stages but set the RTC only once
bool fnSetValDid; //false when starting a set; true when value is changed - to detect if value was not changed

// Volatile running values used throughout the code. (Others are defined right above the method that uses them)
bool alarmSkip = false;
bool alarm2Skip = false;
byte signalSource = 0; //which function triggered the signal - FN_TOD (chime), FN_ALARM, FN_ALARM2, or FN_TIMER (alert or snooze)
byte getSignalSource() { return signalSource; } //NEW
// void setSignalSource(byte val) { signalSource = val; } //TODO need this?
byte signalPattern = 0; //the pattern for that source
word signalRemain = 0; //alarm/timer signal timeout counter, seconds
word getSignalRemain() { return signalRemain; } //NEW
// void setSignalRemain... //TODO need this?
word snoozeRemain = 0; //snooze timeout counter, seconds
word getSnoozeRemain() { return snoozeRemain; } //NEW
// void setSnoozeRemain... //TODO need this? prob not bc startSnooze()
byte timerState = 0; //bit 0 is stop/run, bit 1 is down/up, bit 2 is runout repeat / short signal, bit 3 is runout chrono, bit 4 is lap display
bool getTimerRun() { return timerState&1; } //NEW
bool getTimerDir() { return (timerState>>1)&1; } //NEW
bool getTimerRunoutRepeat() { return (timerState>>2)&1; } //NEW
bool getTimerRunoutChrono() { return (timerState>>3)&1; } //NEW
bool getTimerLapDisplay() { return (timerState>>4)&1; } //NEW
word timerInitialMins = 0; //timer original duration setting, minutes - up to 99h 59m (5999m)
word timerInitialSecs = 0; //timer original duration setting, seconds - up to 59s (could be a byte, but I had trouble casting to unsigned int when doing the math to set timerTime)
unsigned long timerTime = 0; //timestamp of timer/chrono origin (while running) or duration (while stopped)
unsigned long timerLapTime = 0;
unsigned long getTimerDuration() { return (getTimerRun()? convertTimerTime(false): timerTime); } //NEW
const byte millisCorrectionInterval = 30; //used to calibrate millis() to RTC for timer/chrono purposes
unsigned long millisAtLastCheck = 0;
word unoffRemain = 0; //un-off (briefly turn on display during off-hours/away shutoff) timeout counter, seconds
byte displayBrightness = 2; //dim per display or function: 2=normal, 1=dim, 0=off
byte getDisplayBrightness() { return displayBrightness; } //NEW //TODO separate display vs function functionality
#ifdef LIGHTSENSOR
byte ambientLightLevelActual = 0; //if equipped with an ambient light sensor, and if configured to do so, checkRTC will check the ambient light and store it here
byte ambientLightLevel = 0; //the tweening mechanism (below) will move this value to meet the actual, and cycleDisplay will change the brightness of the display accordingly
//see also backlightNow / backlightTarget below
#endif

//If we need to temporarily display a value (or values in series), we can put them here. Can't be zero.
//This is used by network to display IP addresses, and various other bits.
//TODO make FNs for these as well?
int tempValDispQueue[4];
const int tempValDispDur = 2500; //ms
unsigned int tempValDispLast = 0;

#define SHOW_IRRELEVANT_OPTIONS 0 //whether to show everything in settings menu and page (network)

#define SHOW_SERIAL 1 //for debugging


////////// Main code control //////////

void setup(){
#ifdef ENABLE_NEOPIXEL
  #if defined(NEOPIXEL_POWER)
    // If this board has a power control pin, we must set it to output and high
    // in order to enable the NeoPixels. We put this in an #if defined so it can
    // be reused for other boards without compilation errors
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, HIGH);
  #endif
  
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(20); // not so bright

  pixels.fill(0xFFFF00);
  pixels.show();
#endif
  delay(5000); //for development, just in case it boot loops
  if(SHOW_SERIAL) {
    Serial.begin(115200);
    #ifdef SAMD_SERIES
      while(!Serial);
    #else
      delay(1);
    #endif
    Serial.println(F("Hello world"));
  }
  rtcInit();
  initStorage(); //pulls persistent storage data into volatile vars - see storage.cpp
  byte changed = initEEPROM(false); //do a soft init to make sure vals in range
  #ifdef LIGHTSENSOR //TODO do it like NETWORK_H below
    initLightSensor();
  #endif
  initDisplay();
  initOutputs(); //depends on some EEPROM settings
  if(initInputs()){ //inits inputs and returns true if CTRL_SEL is held
    fn = FN_VERSION;
    //skip network for now, since wifi connect hangs - we'll do it after version display is done
  } else {
    #ifdef NETWORK_H
      initNetwork();
    #endif
  }
  
  //Some settings need to be set to a fixed value per the configuration.
  //These settings will also be skipped in fnOptScroll so the user can't change them.
  
  //Signals: if the current eeprom selection is not available,
  //try to use the default specified in the config, failing to pulse and then switch (alarm/timer only)
  if((readEEPROM(42,false)==0 && PIEZO_PIN<0) || (readEEPROM(42,false)==1 && SWITCH_PIN<0) || (readEEPROM(42,false)==2 && PULSE_PIN<0))
    changed += writeEEPROM(42,(ALARM_SIGNAL==0 && PIEZO_PIN>=0? 0: (ALARM_SIGNAL==2 && PULSE_PIN>=0? 2: 1)),false,false); //alarm
  if((readEEPROM(43,false)==0 && PIEZO_PIN<0) || (readEEPROM(43,false)==1 && SWITCH_PIN<0) || (readEEPROM(43,false)==2 && PULSE_PIN<0))
    changed += writeEEPROM(43,(TIMER_SIGNAL==0 && PIEZO_PIN>=0? 0: (TIMER_SIGNAL==2 && PULSE_PIN>=0? 2: 1)),false,false); //timer
  if((readEEPROM(44,false)==0 && PIEZO_PIN<0) || (readEEPROM(44,false)==1 && SWITCH_PIN<0) || (readEEPROM(44,false)==2 && PULSE_PIN<0))
    changed += writeEEPROM(44,(CHIME_SIGNAL==0 && PIEZO_PIN>=0? 0: 2),false,false); //chime
  
  if((PIEZO_PIN<0 && SWITCH_PIN<0 && PULSE_PIN<0) || !ENABLE_ALARM){ //can't do alarm (or alarm2)
    changed += writeEEPROM(2,0,false,false); //force alarm off
    changed += writeEEPROM(3,0,false,false); //force alarm2 off
    changed += writeEEPROM(23,0,false,false); //force autoskip off
    changed += writeEEPROM(50,0,false,false); //force fibonacci off
  } else { //ok to do alarm (and possibly alarm2)
    if(!ENABLE_SOFT_ALARM_SWITCH) changed += writeEEPROM(2,1,false,false); //no soft alarm switch: force alarm on
    if(!ENABLE_ALARM2) { //can't do alarm2
      changed += writeEEPROM(3,0,false,false); //force alarm2 off
    } else {
      if(!ENABLE_SOFT_ALARM_SWITCH) changed += writeEEPROM(3,1,false,false); //no soft alarm switch: force alarm2 on
    }
    if(!ENABLE_SOFT_ALARM_SWITCH || !ENABLE_ALARM_AUTOSKIP) changed += writeEEPROM(23,0,false,false); //no soft switch or no autoskip: force autoskip off
    if((PIEZO_PIN<0 && PULSE_PIN<0) || !ENABLE_ALARM_FIBONACCI) changed += writeEEPROM(50,0,false,false); //no fibonacci, or no piezo or pulse: force fibonacci off
  }
  
  if((PIEZO_PIN<0 && PULSE_PIN<0) || !ENABLE_CHIME){ //can't do chime
    changed += writeEEPROM(21,0,false,false); //force chime off
  }
  
  #ifdef FORCE_ALT_PRESET
    if(readEEPROM(7,false)!=FORCE_ALT_PRESET) changed += writeEEPROM(7,FORCE_ALT_PRESET,false,false);
  #endif
  switch(readEEPROM(7,false)){ //if the preset is set to a function that is no longer enabled, use alarm or date if enabled, else use time
    case FN_DATE: if(!ENABLE_DATE) changed += writeEEPROM(7,(ENABLE_ALARM?FN_ALARM:FN_TOD),false,false); break;
    case FN_DAY_COUNTER: if(!ENABLE_DAY_COUNTER) changed += writeEEPROM(7,(ENABLE_DATE?FN_DATE:FN_TOD),false,false); break;
    case FN_SUN_LAST: if(!ENABLE_SUN) changed += writeEEPROM(7,(ENABLE_DATE?FN_DATE:FN_TOD),false,false); break;
    case FN_SUN_NEXT: if(!ENABLE_SUN) changed += writeEEPROM(7,(ENABLE_DATE?FN_DATE:FN_TOD),false,false); break;
    case FN_WEATHER_LAST: if(!ENABLE_WEATHER) changed += writeEEPROM(7,(ENABLE_DATE?FN_DATE:FN_TOD),false,false); break;
    case FN_WEATHER_NEXT: if(!ENABLE_WEATHER) changed += writeEEPROM(7,(ENABLE_DATE?FN_DATE:FN_TOD),false,false); break;
    case FN_DATE_AUTO: if(!ENABLE_DATE) changed += writeEEPROM(7,FN_TOD,false,false); break;
    case FN_ALARM: if(!ENABLE_ALARM) changed += writeEEPROM(7,FN_TOD,false,false); break;
    case FN_ALARM2: if(!ENABLE_ALARM2) changed += writeEEPROM(7,(ENABLE_ALARM?FN_ALARM:FN_TOD),false,false); break;
    case FN_TIMER: if(!ENABLE_TIMER) changed += writeEEPROM(7,(ENABLE_ALARM?FN_ALARM:FN_TOD),false,false); break;
    case FN_THERMOMETER: if(!ENABLE_THERMOMETER) changed += writeEEPROM(7,(ENABLE_ALARM?FN_ALARM:FN_TOD),false,false); break;
    case FN_TUBETEST: if(!ENABLE_TUBETEST) changed += writeEEPROM(7,(ENABLE_ALARM?FN_ALARM:FN_TOD),false,false); break;
    default: changed += writeEEPROM(7,(ENABLE_ALARM?FN_ALARM:FN_TOD),false,false); break;
  }
  if(!ENABLE_DIMMING) changed += writeEEPROM(27,0,false,false); //display always on
#ifndef LIGHTSENSOR
  //for dimming (formerly night shutoff), 1 used to be "dim per off-hours" but now means "ambient" and is the default, in case it's equipped.
  //If there is no light sensor equipped, but it's set to 1 (per defaults or predating an upgrade), bump it to 2, the new "dim per off-hours."
  //In this case we will also skip option 1 during setting.
  else if(readEEPROM(27,false)==1) changed += writeEEPROM(27,2,false,false);
#endif
  if(!ENABLE_AWAYMODE) changed += writeEEPROM(32,0,false,false); //display always on
  //if backlight circuit is not switched (v5.0 board), the backlight menu setting (eeprom 26) doesn't matter
  if(changed) commitEEPROM(); //for SAMD
} //end setup()

void loop(){
  //Every loop cycle, check the RTC and inputs (previously polled, but works fine without and less flicker)
  //checkEffects(false); //cleaning and scrolling display effects - not handled by checkRTC since they have their own timing
  checkRTC(false); //if clock has ticked, decrement timer if running, and updateDisplay
  millisApplyDrift();
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  #ifdef NETWORK_H
    cycleNetwork();
  #endif
  cycleTimer();
  cycleTweening();
  cycleDisplay( //keeps the display hardware multiplexing cycle going
    displayBrightness, //the display normal/dim/off state
#ifdef LIGHTSENSOR
    readEEPROM(27,false)==1, //whether dimming per ambient light is opted for
    ambientLightLevel,
#else
    false,
    0,
#endif
    fnSetPg //if we are setting
  );
  cycleSignal();
}



////////// Input handling and value setting //////////

//ctrlEvt() has moved to input*.cpp, since how it behaves is a function of the input controls available

void fnScroll(byte dir){
  //0=down, 1=up
  //Switch to the next (up) or previous (down) enabled function. This determines the order.
  //We'll use switch blocks *without* breaks to cascade to the next enabled function
  //TODO this should not be used with inputProton since it has dedicated function controls
  bool alarmOK = (PIEZO_PIN>=0 || SWITCH_PIN>=0 || PULSE_PIN>=0) && ENABLE_ALARM; //skip alarm if no signals available
  bool alarm2OK = alarmOK && ENABLE_ALARM2;
  byte skipFn = 255;
  #ifdef FORCE_ALT_PRESET
    skipFn = FORCE_ALT_PRESET; //we will skip this one in the cycle
    if(fn==skipFn) { //if we are on this one already, go to time
      fn = FN_TOD; return;
    }
  #endif
  if(dir) { // up
    switch(fn) {
      case FN_TOD: if(ENABLE_DATE && skipFn!=FN_DATE) { fn = FN_DATE; break; }
      case FN_DATE: if(alarmOK && skipFn!=FN_ALARM) { fn = FN_ALARM; break; }
      //NB: day counter + sun + weather functions follow FN_DATE automatically, NOT manually
      //see checkRTC() > Automatic function change timeout
      case FN_ALARM: if(alarm2OK && skipFn!=FN_ALARM2) { fn = FN_ALARM2; break; }
      case FN_ALARM2: if(ENABLE_TIMER && skipFn!=FN_TIMER) { fn = FN_TIMER; break; }
      case FN_TIMER: if(ENABLE_THERMOMETER && skipFn!=FN_THERMOMETER) { fn = FN_THERMOMETER; break; }
      case FN_THERMOMETER: if(ENABLE_TUBETEST && skipFn!=FN_TUBETEST) { fn = FN_TUBETEST; break; }
      case FN_TUBETEST: default: fn = FN_TOD; break;
    }
  } else { // down
    switch(fn) {
      case FN_TOD: if(ENABLE_TUBETEST && skipFn!=FN_TUBETEST) { fn = FN_TUBETEST; break; } 
      case FN_TUBETEST: if(ENABLE_THERMOMETER && skipFn!=FN_THERMOMETER) { fn = FN_THERMOMETER; break; }
      case FN_THERMOMETER: if(ENABLE_TIMER && skipFn!=FN_TIMER) { fn = FN_TIMER; break; }
      case FN_TIMER: if(alarm2OK && skipFn!=FN_ALARM2) { fn = FN_ALARM2; break; }
      case FN_ALARM2: if(alarmOK && skipFn!=FN_ALARM) { fn = FN_ALARM; break; }
      case FN_ALARM: if(ENABLE_DATE && skipFn!=FN_DATE) { fn = FN_DATE; break; }
      case FN_DATE: default: fn = FN_TOD; break;
    }
  }
}
void fnOptScroll(byte dir){
  //0=down, 1=up
  //Switch to the next setting, looping around at range ends
  byte posLast = FN_OPTS+sizeof(optsLoc)-1;
  if(dir==1) fn = (fn==posLast? FN_OPTS: fn+1);
  if(dir==0) fn = (fn==FN_OPTS? posLast: fn-1);
  //Certain settings don't apply to some configurations; skip those.
  byte optLoc = optsLoc[fn-FN_OPTS];
  if(!SHOW_IRRELEVANT_OPTIONS && ( //see also: network requestType=1
      //Signals disabled
      (PIEZO_PIN<0 && (optLoc==39||optLoc==40||optLoc==41||optLoc==47||optLoc==48||optLoc==49)) //no piezo: skip all pitches and patterns
      || ((PIEZO_PIN<0 || PULSE_PIN<0) && (optLoc==44)) //no piezo or pulse: skip strike signal type
      || ((PIEZO_PIN<0 && PULSE_PIN<0) && (optLoc==21||optLoc==50)) //no piezo and pulse: skip the rest of strike, and alarm fibonacci mode
      || (((PIEZO_PIN>=0)+(SWITCH_PIN>=0)+(PULSE_PIN>=0)<2) && (optLoc==42||optLoc==43)) //fewer than two signal types: skip alarm/timer signal type
      || (((PIEZO_PIN>=0)+(SWITCH_PIN>=0)+(PULSE_PIN>=0)<1) && (optLoc==23||optLoc==24)) //no signal types: skip the rest of alarm (autoskip and snooze)
      || ((BACKLIGHT_PIN<0) && (optLoc==26)) //no backlight pin: no backlight control
      //Functions disabled
      || (!ENABLE_DATE && (optLoc==17||optLoc==18||optLoc==10||optLoc==12||optLoc==4)) //date fn disabled in config: skip date and geography settings - don't skip utc offset as that's now used when setting clock from network ||optLoc==14
      || (!ENABLE_DAY_COUNTER && (optLoc==4)) //date counter fn disabled in config: skip date counter direction
      || (!ENABLE_ALARM && (optLoc==23||optLoc==42||optLoc==39||optLoc==47||optLoc==24||optLoc==50||optLoc==25)) //alarm fn disabled in config: skip alarm settings (including alarm2 days)
      || (!ENABLE_ALARM2 && (optLoc==25)) //alarm2 fn disabled in config: skip alarm2 days
      || (!ENABLE_TIMER && (optLoc==43||optLoc==40||optLoc==48)) //timer fn disabled in config: skip timer settings
      || (!ENABLE_THERMOMETER && (optLoc==45)) //thermometer fn disabled in config: skip temp format TODO good for weather also
      //Other functionality disabled
      || (!ENABLE_SUN && (optLoc==10||optLoc==12)) //date rise/set disabled in config: skip geography - don't skip utc offset as that's now used when setting clock from network ||optLoc==14
      || (!ENABLE_ALARM_AUTOSKIP && (optLoc==23)) //alarm autoskip disabled in config: skip autoskip switch
      || (!ENABLE_ALARM_FIBONACCI && (optLoc==50)) //fibonacci mode disabled in config: skip fibonacci switch
      || (!ENABLE_CHIME && (optLoc==21||optLoc==44||optLoc==41||optLoc==49)) //chime disabled in config: skip chime
      || (!ENABLE_DIMMING && (optLoc==27||optLoc==28||optLoc==30)) //dimming options disabled in config: skip
      || ((!ENABLE_DIMMING || !ENABLE_AWAYMODE) && (optLoc==32||optLoc==35||optLoc==37)) //dimming and away mode disabled in config: skip away (except workweek)
      || ((!ENABLE_DIMMING || !ENABLE_AWAYMODE) && (!ENABLE_ALARM_AUTOSKIP || !ENABLE_ALARM) && (optLoc==33||optLoc==34)) //(dimming or away) and alarm autoskip disabled: skip workweek
      //Nixie-specific
      #ifndef DISPLAY_NIXIE
      || (optLoc==20||optLoc==46) //digit fade and anti-poisoning
      #endif
    )) {
    fnOptScroll(dir);
  }
}

void switchAlarmState(byte dir, byte whichAlarmFn){
  //0=down, 1=up, 2=toggle
  //TODO proton: confirm that skip is only automatic, and canceled by toggling off to on
  if(ENABLE_SOFT_ALARM_SWITCH){
    //There are three alarm states - on, on with skip (skips the next alarm trigger), and off.
    //Currently we use up/down buttons or a rotary control, rather than a binary switch, so we can cycle up/down through these states.
    //On/off is stored in EEPROM to survive power loss; skip is volatile, not least because it can change automatically and I don't like making automated writes to EEPROM if I can help it.
    if(dir==2) dir=(readEEPROM(whichAlarmFn==FN_ALARM?2:3,false)?0:1); //If alarm is off, cycle button goes up; otherwise down.
    if(dir==1) setAlarmState(2,whichAlarmFn); //if off or skip, go straight to on
    if(dir==0) setAlarmState(getAlarmState(whichAlarmFn)-1,whichAlarmFn); //if on, skip; if skip, off
    updateDisplay();
  }
  //TODO don't make alarm permanent until leaving setting to minimize writes to eeprom as user cycles through options?
}
void setAlarmState(byte state, byte whichAlarmFn){
  //0=off, 1=on with skip, 2=on
  if(!ENABLE_SOFT_ALARM_SWITCH || getAlarmState(whichAlarmFn)==state) return; //don't act unless it's different
  writeEEPROM(whichAlarmFn==FN_ALARM?2:3,state>0,false); //on or off
  alarmSkip = (state==1); //on with skip
  quickBeep(state==2? 76: (state==1? 71: 64)); //C7, G6, C6
}
byte getAlarmState(byte whichAlarmFn){
  //0=off, 1=on with skip, 2=on
  return (readEEPROM(whichAlarmFn==FN_ALARM?2:3,false)?2:0)-alarmSkip;
}
void switchPower(byte dir){
  //0=down, 1=up, 2=toggle
  if(snoozeRemain && dir==0) { //Canceling active snooze
    quickBeep(64); //Short signal to indicate the alarm has been silenced until tomorrow
    displayBlink(); //to indicate this as well 
  }
  signalRemain = 0; //in case alarm is going now - alternatively use signalStop()?
  snoozeRemain = 0;  
  //If the timer(sleep) is running down and is using the switch signal, this instruction conflicts with it, so cancel it
  if(getTimerRun() && !getTimerDir() && readEEPROM(43,false)==1) {
    timerClear();
    updateDisplay();
    return;
  }
  //SWITCH_PIN state is the reverse of the appliance state: LOW = device on, HIGH = device off
  // Serial.print(millis(),DEC);
  // Serial.print(F(" Switch pin requested to "));
  if(dir==2) { //toggle
    dir = (digitalRead(SWITCH_PIN)?1:0); //LOW = device on, so this effectively does our dir reversion for us
    //Serial.print(dir==1?F("toggle on"):F("toggle off"));
  } else {
    //Serial.print(dir==1?F("switch on"):F("switch off"));
  }
  digitalWrite(SWITCH_PIN,(dir==1?0:1)); updateBacklight(); //LOW = device on
  //Serial.println(F(", switchPower"));
}

void startSet(int n, int m, int x, byte p){ //Enter set state at page p, and start setting a value
  fnSetVal=n; fnSetValMin=m; fnSetValMax=x; fnSetValVel=(x-m>30?1:0); fnSetPg=p; fnSetValDid=false;
  if(fnSetValMax==59) blankDisplay(0, 3); //setting in seconds area - blank h:m
  else blankDisplay(4, 5); //setting in h:m area - blank seconds
  updateDisplay();
}
void doSet(int delta){
  //Does actual setting of fnSetVal, as told to by ctrlEvt. Makes sure it stays within range.
  bool did = false;
  while(!did){
    if(delta>0) if(fnSetValMax-fnSetVal<delta) fnSetVal-=((fnSetValMax-fnSetValMin)+1-delta); else fnSetVal=fnSetVal+delta;
    if(delta<0) if(fnSetVal-fnSetValMin<abs(delta)) fnSetVal+=((fnSetValMax-fnSetValMin)+1+delta); else fnSetVal=fnSetVal+delta;
    //In some special settings-menu cases, we have to make sure it's a valid value, and if not, doSet again in order to skip it
    if(fn>=FN_OPTS){ //in settings menu
      switch(optsLoc[fn-FN_OPTS]){ //setting loc, per current setting index
        case 42: case 43: case 44: //signal type: only allow those which are equipped
          if( (fnSetVal==0 && PIEZO_PIN>=0)
            ||(fnSetVal==1 && SWITCH_PIN>=0)
            ||(fnSetVal==2 && PULSE_PIN>=0)) did = true;
          //else leave as false
          break;
        case 26: //backlighting: skip "follow switch signal" option if not equipped
          if(fnSetVal<4 || (fnSetVal==4 && SWITCH_PIN>=0)) did = true;
          //else leave as false
          break;
#ifndef LIGHTSENSOR
        case 27: //dimming: skip ambient option 1 if not equipped with light sensor
          if(fnSetVal!=1) did = true;
          //else leave as false, to skip 1
          break;
#endif
        default: did = true; break;
      }
    } else did = true;
  }
  fnSetValDid=true;
  updateDisplay();
}

void clearSet(){ //Exit set state
  startSet(0,0,0,0);
  fnSetValDid=false;
  updateBacklight(); //in case backlight setting was changed
  checkRTC(true); //force an update to tod and updateDisplay()
}

void setByFn() { //NEW
  //Called by input*.cpp when it's time to enter, page, or exit setting
  //We will set rtc time parts directly
  //con: potential for very rare clock rollover while setting; pro: can set date separate from time
  switch(getCurFn()){
    case FN_TOD: setTime(); break;
    case FN_DATE: case FN_DATE_AUTO: setDate(); break;
    case FN_DAY_COUNTER: setDayCounter(); break;
    case FN_SUN_LAST: break;
    case FN_SUN_NEXT: break;
    case FN_WEATHER_LAST: break;
    case FN_WEATHER_NEXT: break;
    case FN_ALARM: setAlarm(1); break;
    case FN_ALARM2: setAlarm(2); break;
    case FN_TIMER: setTimer(); break;
    case FN_THERMOMETER: break;
    default: break;
  }
}

void setDate() { //NEW
  //TODO: for proton: cycle set at case 3? save after every move?
  //TODO: sevenseg should display y/m/d
  switch(fnSetPg) {
    case 0: //start set year
      //prefill the month and date values with current rtc vals, in case it changes while we're in the middle of setting
      fnSetValDate[1]=rtcGetMonth(), fnSetValDate[2]=rtcGetDate();
      startSet(rtcGetYear(),2000,9999,1);
      break;
    case 1: //save year, start set month
      displayBlink(); //to indicate save. Safe b/c we've inputStopped. See below for why
      fnSetValDate[0]=fnSetVal;
      startSet(fnSetValDate[1],1,12,2);
      break;
    case 2: //save month, start set date
      displayBlink(); //to indicate save. Needed if set month == date: without blink, nothing changes.
      fnSetValDate[1]=fnSetVal;
      startSet(fnSetValDate[2],1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3);
      break;
    case 3: //save date, exit set //TODO: for proton, cycle set?
      Serial.print("Calling rtcSetDate with yr=");
      Serial.print(fnSetValDate[0],DEC);
      Serial.print(", mo=");
      Serial.print(fnSetValDate[1],DEC);
      Serial.print(", dt=");
      Serial.print(fnSetVal,DEC);
      Serial.print(", wd=");
      Serial.print(dayOfWeek(fnSetValDate[0],fnSetValDate[1],fnSetVal),DEC);
      Serial.println();
      rtcSetDate(fnSetValDate[0],fnSetValDate[1],fnSetVal,
        dayOfWeek(fnSetValDate[0],fnSetValDate[1],fnSetVal));
      #ifdef NETWORK_H
        clearNTPSyncLast();
      #endif
      calcSun();
      isDSTByHour(fnSetValDate[0],fnSetValDate[1],fnSetVal,rtcGetHour(),true);
      clearSet();
      break;
    default: break;
  }
}

void setDayCounter() { //NEW
  switch(fnSetPg) {
    case 0: //start set month
      startSet(readEEPROM(5,false),1,12,1);
      break;
    case 1: //save month, set date
      displayBlink(); //to indicate save.
      //Needed if set month == date: without blink, nothing changes. Also just good feedback.
      writeEEPROM(5,fnSetVal,false);
      startSet(readEEPROM(6,false),1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),2);
      break;
    case 2: //save date, set direction
      displayBlink(); //to indicate save.
      writeEEPROM(6,fnSetVal,false);
      startSet(readEEPROM(4,false),0,2,3); //NEW min has changed to 0 so you can disable it from here (and reenable from settings menu)
      break;
    case 3: //save date
      displayBlink(); //to indicate save.
      writeEEPROM(4,fnSetVal,false);
      clearSet();
      break;
    default: break;
  }
}

void setTime() { //NEW
  switch(fnSetPg) {
    case 0: //start set mins
      startSet(rtcGetTOD(),0,1439,1);
      break;
    case 1: //save mins
      if(fnSetValDid){ //but only if the value was actually changed
        Serial.print("Calling rtcSetTime with h=");
        Serial.print(fnSetVal/60,DEC);
        Serial.print(", m=");
        Serial.print(fnSetVal%60,DEC);
        Serial.println();
        rtcSetTime(fnSetVal/60,fnSetVal%60,0);
        #ifdef NETWORK_H
          clearNTPSyncLast();
        #endif
        millisAtLastCheck = 0; //see ms()
        calcSun();
        isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),fnSetVal/60,true);
      }
      clearSet();
      break;
    default: break;
  }
}

void setAlarm(byte whichAlarmFn) { //NEW
  switch(fnSetPg) {
    case 0: //start set mins
      startSet(readEEPROM(whichAlarmFn==FN_ALARM?0:152,true),0,1439,1);
      break;
    case 1:
      writeEEPROM(whichAlarmFn==FN_ALARM?0:152,fnSetVal,true);
      clearSet();
      break;
    default: break;
  }
}

void setTimer() {
  switch(fnSetPg) {
    case 0:
      if(getTimerRun()||timerTime) { timerClear(); } // updateDisplay(); break; } //If the timer is nonzero or running, zero it. But rather than stop there, just go straight into setting – since adjDn (or cycling fns) can reset to zero
      startSet(timerInitialMins,0,5999,1);
      break;
    case 1: //save timer mins, set timer secs
      displayBlink(); //to indicate save.
      timerInitialMins = fnSetVal; //minutes, up to 5999 (99m 59s)
      startSet(timerInitialSecs,0,59,2);
      break;
    case 2: //save timer secs
      displayBlink(); //to indicate save.
      timerInitialSecs = fnSetVal;
      timerTime = (timerInitialMins*60000)+(timerInitialSecs*1000); //set timer duration
      if(timerTime!=0){
        bitWrite(timerState,1,0); //set timer direction (bit 1) to down (0)
        //timerStart(); //we won't automatically start, we'll let the user do that
        //TODO: in timer radio mode, skip setting the seconds (display placeholder) and start when done. May even want to skip runout options even if the beeper is there. Or could make it an option in the config file.
      }
      clearSet();
      break;
    default: break;
  }
}

void setOpt(byte opt) { //NEW
  switch(fnSetPg) {
    case 0: //not yet started
      startSet(readEEPROM(optsLoc[opt],optsMax[opt]>255?true:false),optsMin[opt],optsMax[opt],1);
      break;
    case 1: //save value
      displayBlink(); //to indicate save.
      writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]>255?true:false);
      clearSet(); //TODO need this?
      break;
    default: break;
  }
}

void startSnooze() { //NEW
  snoozeRemain = readEEPROM(24,false)*60; //snoozeRemain is seconds, but snooze duration is minutes
}

void stopSnooze() { //NEW
  snoozeRemain = 0;
}

//EEPROM values are bytes (0 to 255) or signed 16-bit ints (-32768 to 32767) where high byte is loc and low byte is loc+1.
bool initEEPROM(bool hard){
  //If hard, set EEPROM and clock to defaults
  //Otherwise, just make sure stuff is in range
  byte changed = 0;
  //if an actual hard init, set the clock
  if(hard) {
    rtcSetDate(2021,1,1,dayOfWeek(2021,1,1));
    rtcSetTime(0,0,0);
    #ifdef NETWORK_H
      clearNTPSyncLast();
    #endif
  }
  //after that, should also be considered a hard init if EEPROM is uninitiated
  hard = (hard || readEEPROM(16,false)==0);
  //The vars outside the settings menu
  if(hard || readEEPROM(0,true)>1439) changed += writeEEPROM(0,420,true,false); //0-1: alarm at 7am
  if(hard || readEEPROM(0,true)>1439) changed += writeEEPROM(152,420,true,false); //152-153: alarm2 at 7am
  //2: alarm on, handled by init
  //3: alarm2 on, handled by init
  //Day counter direction is now also accessible via settings menu, so we'll let that handle the init
  //if(hard || readEEPROM(4,false)<0 || readEEPROM(4,false)>2) changed += writeEEPROM(4,2,false,false); //4: day counter direction: count up...
  if(hard || readEEPROM(5,false)<1 || readEEPROM(5,false)>12) changed += writeEEPROM(5,12,false,false); //5: ...December...
  if(hard || readEEPROM(6,false)<1 || readEEPROM(6,false)>31) changed += writeEEPROM(6,31,false,false); //6: ...31st. (This gives the day of the year)
  if(hard) changed += writeEEPROM(7,
    #ifdef FORCE_ALT_PRESET
      FORCE_ALT_PRESET
    #else
      0
    #endif
  ,false,false); //7: Alt function preset
  //8: TODO functions/pages enabled (bitmask)
  if(hard) changed += writeEEPROM(15,0,false,false); //15: last known DST on flag - clear on hard reset (to match the reset RTC/auto DST/anti-poisoning settings to trigger midnight tubes as a tube test)
  #ifdef NETWORK_H
    if(hard){ //everything in here needs no range testing
      //51-54 NTP server IP address (4 bytes) - e.g. from https://tf.nist.gov/tf-cgi/servers.cgi
      //Serial.println(F("setting IP address in eeprom"));
      //Serial.print(readEEPROM(51,false),DEC); Serial.print(F(".")); Serial.print(readEEPROM(52,false),DEC); Serial.print(F(".")); Serial.print(readEEPROM(53,false),DEC); Serial.print(F(".")); Serial.println(readEEPROM(54,false),DEC);
      changed += writeEEPROM(51,129,false,false);
      changed += writeEEPROM(52,  6,false,false);
      changed += writeEEPROM(53, 15,false,false);
      changed += writeEEPROM(54, 27,false,false);
      //Serial.print(readEEPROM(51,false),DEC); Serial.print(F(".")); Serial.print(readEEPROM(52,false),DEC); Serial.print(F(".")); Serial.print(readEEPROM(53,false),DEC); Serial.print(F(".")); Serial.println(readEEPROM(54,false),DEC);
      //55-86 Wi-Fi SSID (32 bytes)
      //87-150 Wi-Fi WPA passphrase/key or WEP key (64 bytes)
      for(byte i=0; i<96; i++) changed += writeEEPROM(55+i,0,false,false); //Clear out the old values (32+64+1)
    }
    //9 NTP sync enabled
    if(hard || readEEPROM(9,false)>1) changed += writeEEPROM(9,0,false,false);
    //151 Wi-Fi WEP key index
    if(hard || readEEPROM(151,false)>3) changed += writeEEPROM(151,0,false,false);
  #endif //end network supported
  //The vars inside the settings menu
  bool isInt = false;
  for(byte opt=0; opt<sizeof(optsLoc); opt++) {
    isInt = (optsMax[opt]>255?true:false);
    if(hard || readEEPROM(optsLoc[opt],isInt)<optsMin[opt] || readEEPROM(optsLoc[opt],isInt)>optsMax[opt])
      changed += writeEEPROM(optsLoc[opt],optsDef[opt],isInt,false);
  } //end for
  if(changed) commitEEPROM();
  return changed>0; //whether EEPROM was changed
} //end initEEPROM()


////////// Timing and timed events //////////

byte rtcSecLast = 61;
byte rtcDid = 0; //bitmask to indicate whether we just did things that would be bad to overdo, in case checkRTC is called multiple times during the trigger period (such as if fn is changed during second :00). bit 0 = timer drift correction. At this time other things are OK to overdo since they just restart the same action (alarm/chime signal triggering, date/anti-poisoning display routines) or have other means of canceling (DST change, per DST flag). TODO could get away from this if we move the force stuff into another method?
void checkRTC(bool force){
  //Checks display timeouts;
  //checks for new time-of-day second -> decrements timeouts and checks for timed events;
  //updates display for "running" functions.
  unsigned long now = millis();
  
  //Things to do every time this is called: timeouts to reset display. These may force a tick.
  //Setting timeout: if we're in the settings menu, or we're setting a value
  if(fnSetPg || fn>=FN_OPTS){
    if((unsigned long)(now-getInputLast())>=SETTING_TIMEOUT*1000) { fnSetPg = 0; fn = FN_TOD; force=true; } //Time out after 2 mins
  }
  //Automatic function change timeout
  else if(
    //If it's one of these functions
    (
      //One of the date/calendar ones
      fn==FN_DATE || fn==FN_DAY_COUNTER || fn==FN_SUN_LAST || fn==FN_SUN_NEXT || fn==FN_WEATHER_LAST || fn==FN_WEATHER_NEXT || fn==FN_DATE_AUTO
      //Or, if this is inputProton, the alarms or snooze(timer)
      #ifdef INPUT_PROTON
      || FN_ALARM || FN_ALARM2 || FN_TIMER
      #endif
    )
    //And if the time to display it has past
    && (unsigned long)(now-getInputLast())>=FN_PAGE_TIMEOUT*1000
  ) {
    setInputLast(FN_PAGE_TIMEOUT*1000); //but leave inputLastTODMins alone so the subsequent page displays will be based on the same TOD
    //We'll use switch blocks *without* breaks to cascade to the next enabled function
    //cf. fnScroll()
    switch(fn) {
      //Date + day counter + sun + weather cycle
      case FN_DATE: if(ENABLE_DAY_COUNTER && readEEPROM(4,false)) { fn = FN_DAY_COUNTER; break; }
      case FN_DAY_COUNTER: if(ENABLE_SUN) { fn = FN_SUN_LAST; break; }
      case FN_SUN_LAST: if(false) { fn = FN_WEATHER_LAST; break; }
      case FN_WEATHER_LAST: if(ENABLE_SUN) { fn = FN_SUN_NEXT; break; }
      case FN_SUN_NEXT: if(false) { fn = FN_WEATHER_NEXT; break; }
      case FN_WEATHER_NEXT: fn = FN_TOD; break;
      
      //Auto date
      case FN_DATE_AUTO: fn = FN_TOD; break; //TODO scroll
      
      //If you have future additional cycles (e.g. replacing temporary value display queue), place here
      
      default: fn = FN_TOD; break;
    }
    force = true;
  }
  //Temporary-display function timeout: if we're *not* in a permanent one (time, version, or running/signaling timer)
  // Stopped/non-signaling timer shouldn't be permanent, but have a much longer timeout, mostly in case someone is waiting to start the chrono in sync with some event, so we'll give that an hour.
  else if(fn!=FN_TOD && fn!=FN_VERSION && !(fn==FN_TIMER && (getTimerRun() || signalRemain>0))){
    if((unsigned long)(now-getInputLast())>=(fn==FN_TIMER?3600:FN_TEMP_TIMEOUT)*1000) { fnSetPg = 0; fn = FN_TOD; force=true; }
  }
  
  //Temporary value display queue
  if(tempValDispQueue[0] && !tempValDispLast){ //just starting
    tempValDispLast = now; if(!tempValDispLast) tempValDispLast = 1; //can't be zero
    updateDisplay();
  }
  if(tempValDispLast && (unsigned long)(now-tempValDispLast)>=tempValDispDur){ //already going - time to advance?
    for(char i=0; i<=3; i++) tempValDispQueue[i] = (i==3?0:tempValDispQueue[i+1]);
    if(!tempValDispQueue[0]) tempValDispLast = 0; //zero found, time to stop
    else { tempValDispLast = now; if(!tempValDispLast) tempValDispLast = 1; } //can't be zero
    updateDisplay();
  }
  
  //Update things based on RTC
  rtcTakeSnap();
  
  if(rtcSecLast != rtcGetSecond() || force) { //If it's a new RTC second, or we are forcing it
    
    //First run things
    if(rtcSecLast==61) { autoDST(); calcSun(); }
    
    //Things to do every natural second (decrementing real-time counters)
    if(rtcSecLast != rtcGetSecond()) {
      //If alarm snooze has time on it, decrement, and if we reach zero and responsible alarm is still on, resume
      //Won't check alarm skip status here, as it reflects tomorrow
      if(snoozeRemain>0) {
        snoozeRemain--;
        //Serial.print("sr "); Serial.println(snoozeRemain,DEC);
        if(snoozeRemain<=0 && readEEPROM(signalSource==FN_ALARM2?3:2,false)) { //alarm on (check ALARM2 in case the source changed to something else (TODO would this ever happen?), fall back to FN_ALARM signaling)
          fnSetPg = 0; fn = FN_TOD;
          if(readEEPROM(50,false) && readEEPROM(42,false)!=1) fibonacci(rtcGetHour(),rtcGetMinute(),rtcGetSecond(),signalSource); //fibonacci sequence
          else signalStart(signalSource==FN_ALARM2?FN_ALARM2:FN_ALARM,1); //regular alarm
        }
      }
      if(unoffRemain>0) {
        unoffRemain--; //updateDisplay will naturally put it back to off state if applicable
      }
    } //end natural second
  
    //Things to do at specific times
    word todmins = rtcGetHour()*60+rtcGetMinute();
    
    //Timer drift correction: per the millisCorrectionInterval
    if(rtcGetSecond()%millisCorrectionInterval==0){ //if time:
      if(!(rtcDid&1)) millisCheckDrift(); bitWrite(rtcDid,0,1); //do if not done, set as done
    } else bitWrite(rtcDid,0,0); //if not time: set as not done
    
    //DST change check: every 2am
    if(rtcGetSecond()==0 && rtcGetMinute()==0 && rtcGetHour()==2) autoDST();
    
    //Alarm check: at top of minute for normal alarm, or 23 seconds past for fibonacci (which starts 26m37s early)
    //Only do fibonacci if enabled and if the alarm is not using the switch signal - otherwise do regular
    bool fibOK = readEEPROM(50,false) && readEEPROM(42,false)!=1;
    if((rtcGetSecond()==0 && !fibOK) || (rtcGetSecond()==23 && fibOK)){
      int alarmTime = readEEPROM(0,true);
      int alarmTime2 = readEEPROM(152,true);
      if(rtcGetSecond()==23){ //this is a fibonacci situation
        alarmTime-=27; alarmTime2-=27; //set alarm times to match back by 27 mins
        if(alarmTime<0) alarmTime+=1440; //if the above caused a rollover, fix it
        if(alarmTime2<0) alarmTime2+=1440; //if the above caused a rollover, fix it
      }
      //check alarm 1
      if(rtcGetHour()*60+rtcGetMinute()==alarmTime){
        //Serial.println(rtcGetSecond()==23?F("It's fibonacci time"):F("It's regular alarm time"));
        if(readEEPROM(2,false) && !alarmSkip) { //if the alarm is on and not skipped, sound it!
          fnSetPg = 0; fn = FN_TOD;
          if(rtcGetSecond()==23) fibonacci(rtcGetHour(),rtcGetMinute(),rtcGetSecond(),FN_ALARM); //fibonacci sequence
          else signalStart(FN_ALARM,1); //regular alarm
        }
        //set alarmSkip for the next instance of the alarm
        alarmSkip =
          //if alarm is any day of the week
          (readEEPROM(23,false)==0 ||
          //or if alarm is weekday only, and tomorrow is a weekday
          (readEEPROM(23,false)==1 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),(rtcGetWeekday()==6?0:rtcGetWeekday()+1))) ||
          //or if alarm is weekend only, and tomorrow is a weekend
          (readEEPROM(23,false)==2 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),(rtcGetWeekday()==6?0:rtcGetWeekday()+1)))
          ? 0: 1); //then don't skip the next alarm; else skip it
      } //end alarm 1 trigger
      //check alarm 2
      if(rtcGetHour()*60+rtcGetMinute()==alarmTime2){
        //Serial.println(rtcGetSecond()==23?F("It's fibonacci time"):F("It's regular alarm time"));
        if(readEEPROM(3,false) && !alarm2Skip) { //if the alarm is on and not skipped, sound it!
          fnSetPg = 0; fn = FN_TOD;
          if(rtcGetSecond()==23) fibonacci(rtcGetHour(),rtcGetMinute(),rtcGetSecond(),FN_ALARM2); //fibonacci sequence
          else signalStart(FN_ALARM2,1); //regular alarm
        }
        //set alarmSkip for the next instance of the alarm
        alarm2Skip =
          //if alarm is any day of the week
          (readEEPROM(25,false)==0 ||
          //or if alarm is weekday only, and tomorrow is a weekday
          (readEEPROM(25,false)==1 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),(rtcGetWeekday()==6?0:rtcGetWeekday()+1))) ||
          //or if alarm is weekend only, and tomorrow is a weekend
          (readEEPROM(25,false)==2 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),(rtcGetWeekday()==6?0:rtcGetWeekday()+1)))
          ? 0: 1); //then don't skip the next alarm; else skip it
      } //end alarm 2 trigger
    }
    //At bottom of minute, see if we should show the date
    if(rtcGetSecond()==30 && fn==FN_TOD && fnSetPg==0 && unoffRemain==0) { /*cleanRemain==0 && scrollRemain==0 && */ 
      if(readEEPROM(18,false)>=2) { fn = FN_DATE_AUTO; updateDisplay(); }
      //if(readEEPROM(18,false)==3) { startScroll(); }
    }
    //Anti-poisoning routine triggering: start when applicable, normal brightness, and not during off-hours, setting, or after a button press (unoff)
    if(rtcGetSecond()<2 && fnSetPg==0 && unoffRemain==0 && displayBrightness==2 && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins)) {
      //temporarily we'll recalculate the sun stuff every day
      if(readEEPROM(27,false)>1? //is dimming per schedule enabled?
        rtcGetSecond()==0 && rtcGetHour()*60+rtcGetMinute()==readEEPROM(28,true): //if so, at start of dimming schedule (at second :00 before dim is in effect)
        rtcGetSecond()==1 && rtcGetHour()*60+rtcGetMinute()==0) //if not, at 00:00:01
          calcSun(); //take this opportunity to perform a calculation that blanks the display for a bit
      // TODO the below will need to change cleanRemain=x to displayClean(x)
      
      // switch(readEEPROM(46,false)) { //how often should the routine run?
      //   case 0: //every day
      //     if(readEEPROM(27,false)>1? //is dimming per schedule enabled?
      //       rtcGetSecond()==0 && rtcGetHour()*60+rtcGetMinute()==readEEPROM(28,true): //if so, at start of dimming schedule (at second :00 before dim is in effect)
      //       rtcGetSecond()==1 && rtcGetHour()*60+rtcGetMinute()==0) //if not, at 00:00:01
      //         cleanRemain = 151; //run routine for fifteen cycles
      //     break;
      //   case 1: //every hour
      //     if(rtcGetSecond()==1 && rtcGetMinute()==0) //at min/sec :00:01
      //       cleanRemain = 101; //run routine for ten cycles
      //     break;
      //   case 2: //every minute
      //     if(rtcGetSecond()==1) //at second :01
      //       cleanRemain = 21; //run routine for two cycles
      //     break;
      //   default: break;
      // }
    }
    
    //NTP cue at :59:00
    #ifdef NETWORK_H
      if(rtcGetMinute()==59){
        if(rtcGetSecond()==0) cueNTP();
        if(rtcGetSecond()==30 && ntpSyncAgo()>=30000) cueNTP(); //if at first you don't succeed...
      }
    #endif
    
    //Strikes - only if fn=clock, normal brightness, not in off-hours (given ambient lighting is part of normal brightness), not setting, not signaling/snoozing. Setting 21 will be off if signal type is no good
    //The six pips
    if(rtcGetMinute()==59 && rtcGetSecond()==55 && readEEPROM(21,false)==2 && signalRemain==0 && snoozeRemain==0 && fn==FN_TOD && fnSetPg==0 && displayBrightness==2 && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins)) {
      signalStart(FN_TOD,6); //the signal code knows to use pip durations as applicable
    }
    //Strikes on/after the hour
    if(rtcGetSecond()==0 && (rtcGetMinute()==0 || rtcGetMinute()==30) && signalRemain==0 && snoozeRemain==0 && fn==FN_TOD && fnSetPg==0 && displayBrightness==2 && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins)){
      byte hr; hr = rtcGetHour(); hr = (hr==0?12:(hr>12?hr-12:hr));
      switch(readEEPROM(21,false)) {
        case 1: //single beep
          if(rtcGetMinute()==0) signalStart(FN_TOD,0); break;
        case 3: //hour strike via normal signal cycle
          if(rtcGetMinute()==0) signalStart(FN_TOD,hr); break;
        case 4: //ship's bell at :00 and :30 mins via normal signal cycle
          signalStart(FN_TOD,((hr%4)*2)+(rtcGetMinute()==30?1:0)); break;
        default: break;
      } //end strike type
    } //end strike
    
    //Finally, update the display, whether natural tick or not, as long as we're not setting or on a scrolled display (unless forced eg. fn change)
    //This also determines off-hours, which is why strikes will happen if we go into off at top of hour, and not when we come into on at the top of the hour TODO find a way to fix this
    //Also skip updating the display if this is date and not being forced, since its pages take some calculating that cause it to flicker
    //Also sample the ambient lighting here
#ifdef LIGHTSENSOR
    if(readEEPROM(27,false)==1) { //if we have a light sensor,  and we are set to use ambient light, sample it
      ambientLightLevelActual = getRelativeAmbientLightLevel();
    }
#endif
    if(fnSetPg==0 && (true || force) && !((fn==FN_DATE||fn==FN_DATE_AUTO) && !force)) updateDisplay(); /*scrollRemain==0 ||*/
    //TODO not sure what the above is doing with FN_DATE so I threw in FN_DATE_AUTO as well - is this for the :30 thing?
    
    rtcSecLast = rtcGetSecond();
    
#ifdef ENABLE_NEOPIXEL
    switch(rtcSecLast%3) {
      case 0: pixels.fill(0xFF0000); pixels.show(); break;
      case 1: pixels.fill(0x00FF00); pixels.show(); break;
      case 2: pixels.fill(0x0000FF); pixels.show(); break;
      default: break;
    }
#endif
    
  } //end if force or new second
} //end checkRTC()

void fibonacci(byte h, byte m, byte s, byte whichAlarmFn){
  //This powers the alarm fibonacci feature, using snooze and quick beeps.
  //Find difference between alarm time and current time, in minutes, with midnight rollover
  int diff = readEEPROM(whichAlarmFn==FN_ALARM?0:152,true)-(h*60+m); if(diff<0) diff+=1440;
  //Serial.print(F("diff min ")); Serial.print(diff,DEC);
  //If we are within 30 minutes of alarm time, do Fibonacci stuff
  //This is so the difference can stay an int once we convert it to seconds
  if(diff<30){
    signalRemain=0;
    //Find which number we're on, and if we find one, snooze for that
    diff*=60; diff-=s; //convert to seconds and add seconds component
    //Serial.print(F(", diff sec ")); Serial.print(diff,DEC);
    int nnn = 0;
    int nn = 1;
    int n = 1;
    while(n<1600){ //failsafe – diff starts at 1597s (26m37s)
      //Serial.print(F(", ")); Serial.print(n,DEC); Serial.print(F("+")); Serial.print(nn,DEC); Serial.print(F("="));
      n   = n+nn;
      nn  = n-nn;
      nnn = nn-nnn;
      //Serial.print(n,DEC);
      if(diff<=n) {
        if(diff>0) { //Beep and snooze
          signalPattern = 1; //short beep
          signalSource = whichAlarmFn;
          signalStart(-1,0); //Play a signal measure using above pattern and source
          snoozeRemain = nnn;
          //Serial.print(F(" SR")); Serial.print(snoozeRemain,DEC);
        } else { //Time for regular alarm
          //Serial.print(F(" Alarm!"));
          signalStart(whichAlarmFn,1);
        }
        break;
      }
    }
  }
  //Serial.println();
} //end fibonacci()

void autoDST(){
  //Change the clock if the current DST differs from the new one.
  //Call daily when clock reaches 2am, and at first run.
  bool dstNow = isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),rtcGetHour(),false);
  if(dstNow!=readEEPROM(15,false)){
    rtcSetHour(dstNow>readEEPROM(15,false)? 3: 1); //spring forward or fall back
    writeEEPROM(15,dstNow,false);
  }
}
bool isDST(int y, byte m, byte d){
  //returns whether DST is in effect on this date (after 2am shift)
  switch(readEEPROM(22,false)){ //local DST ruleset
    case 1: //second Sunday in March to first Sunday in November (US/CA)
      return (m==3 && d>=nthSunday(y,3,2)) || (m>3 && m<11) || (m==11 && d<nthSunday(y,11,1)); break;
    case 2: //last Sunday in March to last Sunday in October (UK/EU)
      return (m==3 && d>=nthSunday(y,3,-1)) || (m>3 && m<10) || (m==10 && d<nthSunday(y,10,-1)); break;
    case 3: //first Sunday in April to last Sunday in October (MX)
      return (m==4 && d>=nthSunday(y,4,1)) || (m>4 && m<10) || (m==10 && d<nthSunday(y,10,-1)); break;
    case 4: //last Sunday in September to first Sunday in April (NZ)
      return (m==9 && d>=nthSunday(y,9,-1)) || (m>9 || m<4) || (m==4 && d<nthSunday(y,4,1)); break;
    case 5: //first Sunday in October to first Sunday in April (AU)
      return (m==10 && d>=nthSunday(y,10,1)) || (m>10 || m<4) || (m==4 && d<nthSunday(y,4,1)); break;
    case 6: //third Sunday in October to third Sunday in February (BZ)
      return (m==10 && d>=nthSunday(y,10,3)) || (m>10 || m<2) || (m==2 && d<nthSunday(y,2,3)); break;
    default: break;
  }
  return 0;
}
bool isDSTByHour(int y, byte m, byte d, byte h, bool setFlag){
  //Takes isDST() one step further by comparing to the previous day and considering the hour
  bool dstNow = isDST(y,m,d);
  d--; if(d<1){ m--; if(m<1){ y--; m=12; } d=daysInMonth(y,m); }
  if(dstNow!=isDST(y,m,d) && h<2) dstNow=!dstNow;
  if(setFlag){
    writeEEPROM(15,dstNow,false);
    //Serial.print(F("DST is ")); Serial.println(readEEPROM(15,false)?F("on"):F("off"));
  }
  return dstNow;
}
byte nthSunday(int y, byte m, byte nth){
  if(nth>0) return (((7-dayOfWeek(y,m,1))%7)+1+((nth-1)*7));
  if(nth<0) return (dayOfWeek(y,m,1)==0 && daysInMonth(y,m)>28? 29: nthSunday(y,m,1)+21+((nth+1)*7));
  return 0;
}
byte daysInMonth(word y, byte m){
  if(m==2) return (y%4==0 && (y%100!=0 || y%400==0) ? 29 : 28);
  //https://cmcenroe.me/2014/12/05/days-in-month-formula.html
  else return (28 + ((m + (m/8)) % 2) + (2 % m) + (2 * (1/m)));
}
int daysInYear(word y){
  return 337 + daysInMonth(y,2);
}
int dateToDayCount(word y, byte m, byte d){
  int dc = 0;
  for(byte i=1; i<m; i++) dc += daysInMonth(y,i); //add every full month since start of this year
  dc += d-1; //every full day since start of this month
  return dc;
}
byte dayOfWeek(word y, byte m, byte d){
  //Used by nthSunday and in calls to rtcSetDate
  //Calculated per https://en.wikipedia.org/wiki/Zeller%27s_congruence
  byte yb = y%100; //2-digit year
  byte ya = y/100; //century
  //For this formula, Jan and Feb are considered months 11 and 12 of the previous year.
  //So if it's Jan or Feb, add 10 to the month, and set back the year and century if applicable
  if(m<3) { m+=10; if(yb==0) { yb=99; ya-=1; } else yb-=1; }
  else m -= 2; //otherwise subtract 2 from the month
  return (d + ((13*m-1)/5) + yb + (yb/4) + (ya/4) + 5*ya) %7;
}
int dateComp(int y, byte m, byte d, byte mt, byte dt, bool countUp){
  //If m+d is later   { if count up from, use last year, else use this year }: in Feb, count up from last Mar or down to this Mar
  //If m+d is earlier { if count down to, use next year, else use this year }: in Feb, count down to next Jan or up from this Jan
  bool targetDir = mt*100+dt>=m*100+d+(countUp&&!(mt==12&&dt==31)?1:0); //if count up from 12/31 (day of year), show 365/366 instead of 0
  int targetYear = (countUp && targetDir? y-1: (!countUp && !targetDir? y+1: y));
  int targetDayCount; targetDayCount = dateToDayCount(targetYear, mt, dt);
  if(targetYear<y) targetDayCount -= daysInYear(targetYear);
  if(targetYear>y) targetDayCount += daysInYear(y);
  long currentDayCount; currentDayCount = dateToDayCount(y,m,d);
  return abs(targetDir? currentDayCount-targetDayCount: targetDayCount-currentDayCount);
  //For now, this does not indicate negative (eg with leading zeros bc I don't like how it looks here)
  //and since the direction is specified, it's always going to be either negative or positive
  // Serial.print("Today is ");
  // serialPrintDate(y,m,d);
  // Serial.print(" and ");
  // serialPrintDate(targetYear,mt,dt);
  // Serial.print(" is ");
  // Serial.print(abs(targetDir? currentDayCount-targetDayCount: targetDayCount-currentDayCount),DEC);
  // Serial.print(" day(s) ");
  // Serial.println(countUp?"ago":"away");
}
bool isTimeInRange(word tstart, word tend, word ttest) {
  //Times are in minutes since midnight, 0-1439
  //if tstart < tend, ttest is in range if >= tstart AND < tend
  //if tstart > tend (range passes a midnight), ttest is in range if >= tstart OR < tend
  //if tstart == tend, no ttest is in range
  return ( (tstart<tend && ttest>=tstart && ttest<tend) || (tstart>tend && (ttest>=tstart || ttest<tend)) );
}
bool isDayInRange(byte dstart, byte dend, byte dtest) {
  //Similar to isTimeInRange, only the range is inclusive in both ends (always minimum 1 day match)
  return ( (dstart<=dend && dtest>=dstart && dtest<=dend) || (dstart>dend && (dtest>=dstart || dtest<=dend)) );
}

// Chrono/Timer
// There are two timing sources in the UNDB – the Arduino itself (eg millis()), which gives subsecond precision but isn't very accurate, so it's only good for short-term timing and taking action in response to user activity (eg button press hold thresholds); and the rtc, which is very accurate but only gives seconds (unless you're monitoring its square wave via a digital pin, in DS3231's case), so it's only good for long-term timing and taking action in response to time of day. The one place we need both short-term precision and long-term accuracy is in the chrono/timer – so I have based it on millis() but with an offset applied to correct for its drift, periodically adjusted per the rtc. I also use it for the signal, so the 1/sec measure cycle stays in sync with real time; but we don't need to use it for stuff like button polling.
unsigned long millisDriftOffset = 0; //The cumulative running offset. Since it's circular, doesn't matter whether signed or not.
//unsigned long millisAtLastCheck (defined at top, so ctrlEvt can reset it when setting RTC). 0 when unreliable (at start and after RTC set).
//const byte millisCorrectionInterval (defined at top, so checkRTC can see it)
int millisDriftBuffer = 0; // Each time we calculate millis() drift, we add it to this signed buffer, which gets applied to millisDriftOffset slowly to smooth the correction and minimize (eliminate?) the chance of a discontinuity, which prevents unsightly glitches in the chrono/timer and the signal performance, and failures in eg detecting button presses.
// TODO the adjustments are a little noisy in the short term, because of a rolling offset between the loop cycle (slowed down by cycleDisplay's delays) and the rtc ticks. It's kind of academic since the variance is probably only around ±.02sec per adjustment at most (largely the duration of cycleDisplay's delays), which I'd say is within the tolerance of display/button/human limitations, but no point doing it as quickly as once per second I think.
void millisCheckDrift(){
  unsigned long now = millis();
  if(millisAtLastCheck){ // if this has a value, check to see how much millis has drifted since then. If this is 0, it means either the RTC was recently set (or the extremely unlikely case that the last sync occurred exactly at millis rollover) so we will hold off until next drift check.
    long millisDrift = now-(millisAtLastCheck+(millisCorrectionInterval*1000)); // Converting difference to a signed long.
    if(abs((long)(millisDrift+millisDriftBuffer))>32767){} // If adding drift to buffer would make it overflow, ignore it this time
    else {
      millisDriftBuffer -= millisDrift;
      // rtcTakeSnap();
      // if(rtcGetHour()<10) Serial.print(F("0"));
      // Serial.print(rtcGetHour(),DEC);
      // Serial.print(F(":"));
      // if(rtcGetMinute()<10) Serial.print(F("0"));
      // Serial.print(rtcGetMinute(),DEC);
      // Serial.print(F(":"));
      // if(rtcGetSecond()<10) Serial.print(F("0"));
      // Serial.print(rtcGetSecond(),DEC);
      // Serial.print(F("  millis: "));
      // Serial.print(now,DEC);
      // Serial.print(F("  drift: "));
      // Serial.print(millisDrift,DEC);
      // Serial.print(F("  new buffer: "));
      // Serial.print(millisDriftBuffer,DEC);
      // Serial.println();
    }
  }
  millisAtLastCheck = now;
}
void millisApplyDrift(){
  //Applies millisDriftBuffer to millisDriftOffset at the rate of 1ms per loop. See above for details.
  if(millisDriftBuffer){
    millisDriftOffset += (millisDriftBuffer>0? 1: -1);
    millisDriftBuffer -= (millisDriftBuffer>0? 1: -1);
    // tod = rtc.now();
    // if(rtcGetHour()<10) Serial.print(F("0"));
    // Serial.print(rtcGetHour(),DEC);
    // Serial.print(F(":"));
    // if(rtcGetMinute()<10) Serial.print(F("0"));
    // Serial.print(rtcGetMinute(),DEC);
    // Serial.print(F(":"));
    // if(rtcGetSecond()<10) Serial.print(F("0"));
    // Serial.print(rtcGetSecond(),DEC);
    // Serial.print(F("  new offset: "));
    // Serial.print(millisDriftOffset,DEC);
    // Serial.print(F("  new buffer: "));
    // Serial.print(millisDriftBuffer,DEC);
    // Serial.println();
  }
}
void millisReset(){
  millisAtLastCheck = 0; //because setting the RTC makes this unreliable
}
unsigned long ms(){
  // Returns millis() with the drift offset applied, for timer/chrono purposes.
  // WARNING: Since the offset is being periodically adjusted, there is the possibility of a discontinuity in ms() output – if we give out a timestamp and then effectively set the clock back, the next timestamp might possibly be earlier than the last one, which could mess up duration math. I tried to think of a way to monitor for that discontinuity – e.g. if now-then is greater than then-now, due to overflow – but it gets tricky since millis() is effectively circular, and that condition occurs naturally at rollover as well – so I think we would need a flag that millisCheckDrift sets when it sets the offset backward, and ms clears when the real time has caught up.... or something like that.
  // So for now we'll restrict use of ms() to the timer/chrono duration – the only place we really need it, and fortunately it's not a problem here, because that duration never exceeds 100 hours so we can easily detect that overflow. And the only time it might really be an issue is if it occurs immediately after the chrono starts, since it would briefly display the overflowed time – in that case we'll just reset the timer to 0 and keep going.
  return (unsigned long)(millis()+millisDriftOffset);
}
void timerStart(){
  if(!getTimerRun()) {
    bitWrite(timerState,0,1); //set timer running (bit 0) to on (1)
    if(timerTime==0) { //if the timer was idle...
      if(readEEPROM(43,false)==1) { //sleep mode: count down from 30 min //TODO make configurable
        bitWrite(timerState,1,0); //set timer direction (bit 1) to down (0)
        timerTime = 30*60*1000;
      } else { //chrono/timer: count up from 0
        bitWrite(timerState,1,1); //set timer direction (bit 1) to up (1)
      }
    }
    timerTime = convertTimerTime(true); //convert from duration to origin
    if(!(getTimerDir())) timerSleepSwitch(1); //possibly toggle the switch signal, but only if counting down
    quickBeep(69);
  }
} //end timerStart()
void timerStop(){
  if(getTimerRun()) {
    bitWrite(timerState,0,0); //set timer running (bit 0) to off (0)
    timerTime = convertTimerTime(false); //convert from origin to duration
    if(!(getTimerDir())) timerSleepSwitch(0); //possibly toggle the switch signal, but only if counting down
    quickBeep(64);
    bitWrite(timerState,4,0); //set timer lap display (bit 4) to off (0)
    updateDisplay(); //since cycleTimer won't do it
  }
}
unsigned long convertTimerTime(bool mode) {
  //timerTime holds either a duration (when stopped) or an origin (when running).
  if(mode) { //timer is starting: convert from duration to origin
    //If chrono (count up), find origin in the past: now minus duration.
    //If timer (count down), find origin in the future: now plus duration.
    return (getTimerDir()? ms() - timerTime: ms() + timerTime);
  } else { //timer is stopping, or we are "sampling" for display: convert from origin to duration
    //If chrono (count up), timestamp is an origin in the past: duration is now minus timestamp.
    //If timer (count down), timestamp is a target in the future: duration is timestamp minus now.
    return (getTimerDir()? ms() - timerTime: timerTime - ms()); 
  }
}
void timerClear(){
  bitWrite(timerState,0,0); //set timer running (bit 0) to off (0)
  //bitWrite(timerState,1,1); //set timer direction (bit 1) to up (1) TODO is this necessary
  timerTime = 0; //set timer duration
  timerSleepSwitch(0);
  bitWrite(timerState,4,0); //set timer lap display (bit 4) to off (0)
  //updateDisplay is called not long after this
}
void timerLap(){
  timerLapTime = ms();
  bitWrite(timerState,4,1); //set timer lap display (bit 4) to on (1)
  quickBeep(81);
}
void timerRunoutToggle(){
  if(PIEZO_PIN>=0){ //if piezo equipped
    //cycle thru runout options: 00 stop, 01 repeat, 10 chrono, 11 chrono short signal
    timerState ^= (1<<2); //toggle runout repeat bit
    if(!(getTimerRunoutRepeat())) timerState ^= (1<<3); //if it's 0, toggle runout chrono bit
    //do a quick signal to indicate the selection
    signalPattern = ((timerState>>2)&3)+1; //convert 00/01/10/11 to 1/2/3/4
    signalSource = FN_TIMER;
    signalStart(-1,0); //Play a signal measure using above pattern and source
  }
}
void cycleTimer(){
  if(getTimerRun()){ //If the timer is running
    //Check if we've hit a wall
    if(!(getTimerDir())){ //If we are counting down,
      if((unsigned long)(ms()-timerTime)<1000){ //see if now is past timerTime (diff has rolled over)
        //timer has run out
        //runout action and display
        if(getTimerRunoutChrono()){ //runout chrono - keep target, change direction, kill sleep, change display
          bitWrite(timerState,1,1); //set timer direction (bit 1) to up (1)
          timerSleepSwitch(0);
          fnSetPg = 0; fn = FN_TIMER;
        } else {
          if(getTimerRunoutRepeat()){ //runout repeat - keep direction, change target, keep sleep, don't change display
            timerTime += (timerInitialMins*60000)+(timerInitialSecs*1000); //set timer duration ahead by initial setting
          } else { //runout clear - clear timer, change display
            timerClear();
            //If switch signal (radio sleep), go to time of day; otherwise go to empty timer to appear with signal
            fnSetPg = 0; fn = (readEEPROM(43,false)==1 ? FN_TOD: FN_TIMER);
            updateDisplay();
          }
        }
        //piezo or pulse signal
        if(getTimerRunoutRepeat()){ //short signal (piggybacks on runout repeat flag)
          if(readEEPROM(43,false)!=1) signalStart(FN_TIMER,1);
          //using 1 instead of 0, because in signalStart, FN_TIMER "quick measure" has a custom pitch for runout option setting
        } else { //long signal
          if(readEEPROM(43,false)!=1) signalStart(FN_TIMER,SIGNAL_DUR);
        }
      }
    } else { //If we are counting up,
      if((timerState>>4)&1){ //if we're doing a lap display, see if we need to cancel it after 3 seconds
        if((unsigned long)(ms()-timerLapTime)>=3000) bitWrite(timerState,4,0); //set timer lap display (bit 4) to cancel (0)
      }
      if((unsigned long)(ms()-timerTime)>=360000000){ //find out if now is over the amount we can display (100h, or 360M ms)
        //chrono has maxed out
        // if it has maxed out by a LOT, it was probably due to a rare overflow possibility right after starting (see ms()) – so just restart.
        if((unsigned long)(ms()-timerTime)>=370000000) timerTime = ms();
        else timerClear();
        // I thought about restarting anytime the chrono maxes out, but I don't really want to support leaving the chrono running indefinitely, since the display has no good function indicator – an unfamiliar user might accidentally start the chrono, then later be looking for the idle `0` which is the surefire timer/chrono indicator and never find it. This is the same reason I make it reset if the user switches away while it's stopped.
        // Serial.print(F("Chrono has maxed out per "));
        // Serial.print(ms(),DEC);
        // Serial.print(F("-"));
        // Serial.print(timerTime,DEC);
      }
    }
    //If it's on display, update
    if(fn==FN_TIMER) updateDisplay();
  }
} //end cycleTimer()
void timerSleepSwitch(bool on){
  //When timer is set to use switch signal, it's on while timer is running, "radio sleep" style.
  //We won't use the true signal methods so that other signals might not interrupt it. TODO confirm
  if(readEEPROM(43,false)==1) { //start "radio sleep"
    digitalWrite(SWITCH_PIN,(on?LOW:HIGH)); updateBacklight(); //LOW = device on
    // Serial.print(millis(),DEC);
    // if(on) Serial.println(F(" Switch signal on, timerSleepSwitch"));
    // else   Serial.println(F(" Switch signal off, timerSleepSwitch"));
  }
}
byte getTimerState(){ return timerState; }
void setTimerState(char pos, bool val){
  if(val) timerState |= (1<<pos); else timerState &= ~(1<<pos);
}

void startUnoff() {
  //This is called by most input actions, so continued button presses during an unoff keep the display alive
  unoffRemain = UNOFF_DUR;
}

void tempDisplay(int i0, int i1, int i2, int i3){ //TODO can you improve this
  tempValDispQueue[0] = i0;
  tempValDispQueue[1] = i1;
  tempValDispQueue[2] = i2;
  tempValDispQueue[3] = i3;
}
void updateDisplay(){
  //Run as needed to update display when the value being shown on it has changed
  //This formats the new value and puts it in displayNext[] for cycleDisplay() to pick up
  
  // if(scrollRemain==-128) {  //If the current display is flagged to be scrolled out, do it. This is kind of the counterpart to startScroll()
  //   for(byte i=0; i<6; i++) scrollDisplay[i] = displayNext[i]; //cache the current value in scrollDisplay[] just in case it changed
  //   scrollRemain = (0-DISPLAY_SIZE)-1;
  // }
  //
  // if(cleanRemain) { //cleaning tubes
  //   displayBrightness = 2;
  //   byte digit = 10-((cleanRemain-1)%10); //(11-cleanRemain)%10;
  //   editDisplay(digit,0,0,true,false);
  //   editDisplay(digit,1,1,true,false);
  //   editDisplay(digit,2,2,true,false);
  //   editDisplay(digit,3,3,true,false);
  //   editDisplay(digit,4,4,true,false);
  //   editDisplay(digit,5,5,true,false);
  // }
  // /*
  // Scrolling "frames" (ex. on 4-tube clock):
  // To use this, call editDisplay() as usual, then startScroll()
  // 4:              [       ]1 2 3 4      tube[n] (0-index) is source[n-scrollRemain] unless that index < 0, then blank
  // 3:              [      1]2 3 4
  // 2:              [    1 2]3 4
  // 1:              [  1 2 3]4
  // 0/-128:         [1 2 3 4]
  // -4:            1[2 3 4  ]      tube[n] is source[n+DISPLAY_SIZE+scrollRemain+1] unless that index >= DISPLAY_SIZE, then blank
  // -3:          1 2[3 4    ]
  // -2:        1 2 3[4      ]
  // -1:      1 2 3 4[       ]
  // 0: (scroll functions are skipped to display whatever's applicable)
  // */
  // else if(scrollRemain>0) { //scrolling display: value coming in - these don't use editDisplay as we're going array to array
  //   for(byte i=0; i<DISPLAY_SIZE; i++) {
  //     int8_t isrc = i-scrollRemain; //needs to support negative
  //     displayNext[i] = (isrc<0? 15: scrollDisplay[isrc]); //allow to fade
  //   }
  // }
  // else if(scrollRemain<0 && scrollRemain!=-128) { //scrolling display: value going out
  //   for(byte i=0; i<DISPLAY_SIZE; i++) {
  //     byte isrc = i+DISPLAY_SIZE+scrollRemain+1;
  //     displayNext[i] = (isrc>=DISPLAY_SIZE? 15: scrollDisplay[isrc]); //allow to fade
  //   }
  // } //todo move cleanRemain, scrollRemain to dispNixie
  // else
  if(fn==FN_VERSION) {
    editDisplay(vMajor, 0, 1);
    editDisplay(vMinor, 2, 3);
    editDisplay(vPatch, 4, 5);
  }
  else if(tempValDispQueue[0]>0){ //include display fades
    editDisplay(tempValDispQueue[0], 0, 3, false, true);
    blankDisplay(4, 5, true);
  }
  else if(fnSetPg) { //setting value, for either fn or settings menu - no display fades
    // displayBrightness = 2; //taken over by display code? TODO confirm
    // blankDisplay(4, 5); //taken over by startSet
    byte fnOptCurLoc = (fn>=FN_OPTS? optsLoc[fn-FN_OPTS]: 0); //current setting index loc, to tell what's being set
    if(fnSetValMax==1439) { //Time of day (0-1439 mins, 0:00–23:59): show hrs/mins
      editDisplay(fnSetVal/60, 0, 1, readEEPROM(19,false)); //hours with leading zero per settings
      editDisplay(fnSetVal%60, 2, 3, true);
    } else if(fnSetValMax==5999) { //Timer duration mins (0-5999 mins, up to 99:59): show hrs/mins w/regular leading
      editDisplay(fnSetVal/60, 0, 1, readEEPROM(19,false)); //hours with leading zero per settings
      editDisplay(fnSetVal%60, 2, 3, true); //minutes with leading zero always
    } else if(fnSetValMax==59) { //Timer duration secs: show with leading
      //If 6 digits (0-5), display on 4-5
      //If 4 digits (0-3), dislpay on 2-3
      // blankDisplay(0, 3); //taken over by startSet
      editDisplay(fnSetVal, (DISPLAY_SIZE>4? 4: 2), (DISPLAY_SIZE>4? 5: 3), true);
    } else if(fnSetValMax==88) { //A piezo pitch. Play a short demo beep.
      editDisplay(fnSetVal, 0, 3);
      quickBeep(fnSetVal); //Can't use signalStart since we need to specify pitch directly
    } else if(fnOptCurLoc==47 || fnOptCurLoc==48 || fnOptCurLoc==49) { //Signal pattern. Play a demo measure.
      editDisplay(fnSetVal, 0, 3);
      quickBeepPattern((fnOptCurLoc==49?FN_TOD:(fnOptCurLoc==48?FN_TIMER:FN_ALARM)),fnSetVal);
    } else if(fnSetValMax==156) { //Timezone offset from UTC in quarter hours plus 100 (since we're not set up to support signed bytes)
      //Display hours on 0-1 and minutes on 2-3. If sevenseg, omit negative LZs and display - at 0 (when hour>-10).
      #ifdef SEVENSEG
        editDisplay((abs(fnSetVal-100)*25)/100, 0, 1); //hours
        if(fnSetVal<100 && fnSetVal>60) editDisplay(45,0); //negative symbol
        //TODO add support for "-1" in single character for when hour<=-10 - use character 200
      #else
        editDisplay((abs(fnSetVal-100)*25)/100, 0, 1, fnSetVal<100); //hours, leading zero for negatives
      #endif
      editDisplay((abs(fnSetVal-100)%4)*15, 2, 3, true); //minutes, leading zero always
    } else if(fnSetValMax==900 || fnSetValMax==1800) { //Lat/long in tenths of a degree
      //If 6 digits (0-5), display degrees on 0-3 and tenths on 4, with 5 blank. If sevenseg, omit negative LZs and display - at 0.
      //If 4 digits (0-3), display degrees on 0-2 and tenths on 3. If sevenseg, omit negative LZs and display - at 0 (when fnSetVal > -1000).
      #ifdef SEVENSEG
        editDisplay(abs(fnSetVal), 0, (DISPLAY_SIZE>4? 4: 3));
        if(fnSetVal<0 && (fnSetVal>-1000 || DISPLAY_SIZE>4)) editDisplay(45,0); //negative symbol
        //TODO add support for "-1" in single character for when fnSetVal<=-1000 - use character 200
      #else
        editDisplay(abs(fnSetVal), 0, (DISPLAY_SIZE>4? 4: 3), fnSetVal<0);
      #endif
    } else { //some other type of value. If sevenseg, omit negative LZs and display - at 0 (when fnSetVal > -1000).
      #ifdef SEVENSEG
        editDisplay(abs(fnSetVal), 0, 3);
        if(fnSetVal<0 && fnSetVal>-1000) editDisplay(45,0); //negative symbol
        //TODO add support for "-1" in single character for when fnSetVal<=-1000 - use character 200
      #else
        editDisplay(abs(fnSetVal), 0, 3, fnSetVal<0);
      #endif
    }
    #ifdef SEVENSEG
    //Depending on what's being set, display ascii letters to be more intuitive
    //TODO prevent flash of non-custom value
    if(fn<FN_OPTS) { //just setting a regular fn
      switch(fn) {
        case FN_DATE:
          switch(fnSetPg) {
            case 1: editDisplay(121,4); editDisplay(114,5); break; //"yr"
            case 2: editDisplay(109,4); editDisplay(111,5); break; //"mo"
            case 3: editDisplay(100,4); editDisplay(116,5); break; //"dt"
            default: break;
          } break;
        case FN_DAY_COUNTER:
          switch(fnSetPg) {
            case 1: editDisplay(109,4); editDisplay(111,5); break; //"mo"
            case 2: editDisplay(100,4); editDisplay(116,5); break; //"dt"
            case 3: //Day count direction - duplicate of below //TODO is this right?
              switch(fnSetVal) {
                case 0: blankDisplay(0); editDisplay(79,1); editDisplay(102,2); editDisplay(102,3); break; //"_Off"
                case 1: editDisplay(67,0); editDisplay(116,1); editDisplay(100,2); editDisplay(110,3); break; //"Ctdn"
                case 2: editDisplay(67,0); editDisplay(116,1); editDisplay(85,2); editDisplay(80,3); break; //"CtUP"
              }
              blankDisplay(4,5); //nothing on seconds display
              break;
            default: break;
          }
          break;
        #if defined(ENABLE_ALARM2) && ENABLE_ALARM2
          case FN_ALARM:
            editDisplay(65,4); editDisplay(1,5); break; //"A1"
          case FN_ALARM2:
            editDisplay(65,4); editDisplay(2,5); break; //"A2"
        #else
          case FN_ALARM: case FN_ALARM2:
            blankDisplay(4,5); break;
        #endif
        default: break;
      }
    } else { //in settings menu
      switch(fnOptCurLoc) {
        case 4: //Day count direction – duplicate of above
          switch(fnSetVal) {
            case 0: blankDisplay(0); editDisplay(79,1); editDisplay(102,2); editDisplay(102,3); break; //"_Off"
            case 1: editDisplay(67,0); editDisplay(116,1); editDisplay(100,2); editDisplay(110,3); break; //"Ctdn"
            case 2: editDisplay(67,0); editDisplay(116,1); editDisplay(85,2); editDisplay(80,3); break; //"CtUP"
          }
          blankDisplay(4,5); //nothing on seconds display
          break;
        case 16: //Time format
          switch(fnSetVal) {
            case 1: editDisplay(12,0,1); editDisplay(104,2); blankDisplay(3); break; //"12h_"
            case 2: editDisplay(24,0,1); editDisplay(104,2); blankDisplay(3); break; //"24h_"
          }
          blankDisplay(4,5); //nothing on seconds display
          break;
        case 17: //Date format
          switch(fnSetVal) {
            //1 = month/date/weekday<br/>2 = date/month/weekday<br/>3 = month/date/year<br/>4 = date/month/year<br/>5 = year/month/date
            case 1: //month/date/weekday
              editDisplay(109,0); editDisplay(111,1); //"mo"
              editDisplay(100,2); editDisplay(116,3); //"dt"
              editDisplay(100,4); editDisplay(121,5); //"dy"
              break;
            case 2: //date/month/weekday
              editDisplay(100,0); editDisplay(116,1); //"dt"
              editDisplay(109,2); editDisplay(111,3); //"mo"
              editDisplay(100,4); editDisplay(121,5); //"dy"
              break;
            case 3: //month/date/year
              editDisplay(109,0); editDisplay(111,1); //"mo"
              editDisplay(100,2); editDisplay(116,3); //"dt"
              editDisplay(121,4); editDisplay(114,5); //"yr"
              break;
            case 4: //date/month/year
              editDisplay(100,0); editDisplay(116,1); //"dt"
              editDisplay(109,2); editDisplay(111,3); //"mo"
              editDisplay(121,4); editDisplay(114,5); //"yr"
              break;
            case 5: //year/month/date
              editDisplay(121,0); editDisplay(114,1); //"yr"
              editDisplay(109,2); editDisplay(111,3); //"mo"
              editDisplay(100,4); editDisplay(116,5); //"dt"
              break;
          }
          break;
        //add any future value overwrites here
        default: break;
      } //end switch(fnOptCurLoc)
    } //end in settings menu
    #endif //sevenseg
  } //end if setting
  else if(fn >= FN_OPTS){ //settings menu, but not setting a value
    // displayBrightness = 2; //taken over by display code? TODO confirm
    editDisplay(optsNum[fn-FN_OPTS],0,1); //display setting number on hour digits
    blankDisplay(2,5);
    //If sevenseg, could consider displaying a "key" here, but I tried it and it was still pretty obtuse
  }
  else { //fn running
    
    //Set displayBrightness per dimming/away schedule - FN_ALARM may override this
    //issue: moving from off alarm to next fn briefly shows alarm in full brightness. I think because of the display delays. TODO
    word todmins = rtcGetHour()*60+rtcGetMinute();
    //In order of precedence: //TODO can we fade between dim states? 
    //clock at work: away on weekends, all day
    if( readEEPROM(32,false)==1 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),rtcGetWeekday()) )
      displayBrightness = (unoffRemain>0? 2: 0); //unoff overrides this
    //clock at home: away on weekdays, during office hours only
    else if( readEEPROM(32,false)==2 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),rtcGetWeekday()) && isTimeInRange(readEEPROM(35,true), readEEPROM(37,true), todmins) ) displayBrightness = (unoffRemain>0? 2: 0);
    //dimming per schedule - if night end is 0:00, use alarm1 time instead
    else if( readEEPROM(27,false)>1 && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins) ) displayBrightness = (readEEPROM(27,false)==2?1:(unoffRemain>0?2:0)); //dim or (unoff? bright: off)
    //normal
    else displayBrightness = 2;
    //variable brightness will be sampled once per second in checkRTC
    updateBacklight();
    
    switch(fn){
      case FN_TOD:
        byte hr; hr = rtcGetHour();
        if(readEEPROM(16,false)==1) hr = (hr==0?12:(hr>12?hr-12:hr));
        editDisplay(hr, 0, 1, readEEPROM(19,false), true);
        editDisplay(rtcGetMinute(), 2, 3, true, true);
        //Serial.print(millis(),DEC); Serial.println(F("show display per regular (hours/mins at least)"));
        #ifdef NETWORK_H
          if(readEEPROM(9,false) && ntpSyncAgo()>=86400000){ blankDisplay(4,5,true); break; }
        #endif
        if(readEEPROM(18,false)==1) editDisplay(rtcGetDate(), 4, 5, readEEPROM(19,false), true); //date
        else editDisplay(rtcGetSecond(), 4, 5, true, true); //seconds
        break;
      case FN_DATE: case FN_DATE_AUTO:
        byte df; df = readEEPROM(17,false); //1=m/d/w, 2=d/m/w, 3=m/d/y, 4=d/m/y, 5=y/m/d
        if(df<=4) {
          editDisplay((df==1||df==3?rtcGetMonth():rtcGetDate()),0,1,readEEPROM(19,false),true); //month or date first
          editDisplay((df==1||df==3?rtcGetDate():rtcGetMonth()),2,3,readEEPROM(19,false),true); //date or month second
          editDisplay((df<=2?rtcGetWeekday():rtcGetYear()),4,5,(df<=2?false:true),true); //dow or year third - dow never leading zero, year always
        }
        else { //df==5
          editDisplay(rtcGetYear(),0,1,true,true); //year always has leading zero
          editDisplay(rtcGetMonth(),2,3,readEEPROM(19,false),true);
          editDisplay(rtcGetDate(),4,5,readEEPROM(19,false),true);
        }
        break;
      case FN_DAY_COUNTER:
        editDisplay(dateComp(rtcGetYear(),rtcGetMonth(),rtcGetDate(),readEEPROM(5,false),readEEPROM(6,false),readEEPROM(4,false)-1),0,3,false,true);
        #ifdef SEVENSEG
        //on seconds digits, display alpha letters - this time with fades
        if(readEEPROM(4,false)-1) { //count up
          editDisplay(100,4,4,false,true); blankDisplay(5,5,true); //"d_"
        } else { //count down
          editDisplay(116,4,4,false,true); editDisplay(111,5,false,true); //"to"
        }
        #else
        blankDisplay(4,5,true);
        #endif
        break;
      //The sun and weather displays are based on a snapshot of the time of day when the function display was triggered, just in case it's triggered a few seconds before a sun event (sunrise/sunset) and the "prev/now" and "next" displays fall on either side of that event, they'll both display data from before it. If triggered just before midnight, the date could change as well – not such an issue for sun, but might be for weather - TODO create date snapshot also
      case FN_SUN_LAST:
        displaySun(0,rtcGetDate(),getInputLastTODMins());
        break;
      case FN_SUN_NEXT:
        displaySun(1,rtcGetDate(),getInputLastTODMins());
        break;
      case FN_WEATHER_LAST:
        displayWeather(0);
        break;
      case FN_WEATHER_NEXT:
        displayWeather(1);
        break;
      case FN_ALARM: case FN_ALARM2: //alarm1 and alarm2
        displayBrightness = (readEEPROM(fn==FN_ALARM?2:3,false)?2:1); //status normal/dim
        #ifdef SEVENSEG
          if(!readEEPROM(fn==FN_ALARM?2:3,false)) { //alarm off: display "Off"
            blankDisplay(0,0,true); editDisplay(79,1,1,0,true); editDisplay(102,2,2,0,true); editDisplay(102,3,3,0,true); blankDisplay(4,5,true);
          } else {
            //Alarm is on
            //Display time (same as below)
            word almTime; almTime = readEEPROM(fn==FN_ALARM?0:152,true);
            editDisplay(almTime/60, 0, 1, readEEPROM(19,false), true); //hours with leading zero
            editDisplay(almTime%60, 2, 3, true, true);
            #ifdef INPUT_PROTON
              //Display "A1" or "A2" on seconds
              //Not much point displaying the status since the button state indicates it, and toggling will clear autoskip
              editDisplay(65,4,4,0,true); editDisplay(fn==FN_ALARM?1:2,5,5,0,true); break; //"A1" or "A2"
            #else
              //Display "On" or "SP" (skip)
              //Probably won't have A1 and A2 in this case, but keeping logic parity with rest of code
              if(fn==FN_ALARM?alarmSkip:alarm2Skip){ //alarm on+skip
                editDisplay(83,4,4,0,true); editDisplay(80,5,5,0,true); //"SP"
              } else { //alarm fully on
                editDisplay(79,4,4,0,true); editDisplay(110,5,5,0,true); //"On"
              }
            #endif
          }
        #else
          //Display time (same as above)
          word almTime; almTime = readEEPROM(fn==FN_ALARM?0:152,true);
          editDisplay(almTime/60, 0, 1, readEEPROM(19,false), true); //hours with leading zero
          editDisplay(almTime%60, 2, 3, true, true);
          //Display alarm status on seconds: 1 (on), 01 (skip), 0 (off)
          if(readEEPROM(fn==FN_ALARM?2:3,false) && (fn==FN_ALARM?alarmSkip:alarm2Skip)){ //alarm on+skip
            editDisplay(1,4,5,true,true); //01 to indicate off now, on maybe later
          } else { //alarm fully on or off
            editDisplay(readEEPROM(fn==FN_ALARM?2:3,false),4,4,false,true);
            blankDisplay(5,5,true);
          }
        #endif
        break;
      case FN_TIMER: //timer - display time
        unsigned long td; //current timer duration - unless this is lap display, in which case show that
        td = (getTimerRun() && getTimerLapDisplay()? timerLapTime: getTimerDuration());
        byte tdc; tdc = (td%1000)/10; //capture hundredths (centiseconds)
        td = td/1000+(!(getTimerDir())&&tdc!=0?1:0); //remove mils, and if countdown, round up
        //Countdown shows H:M:S, but on DISPLAY_SIZE<6 and H<1, M:S
        //Countup shows H:M:S, but if H<1, M:S:C, but if DISPLAY_SIZE<6 and M<1, S:C
        bool lz; lz = readEEPROM(19,false)&1;
        if(getTimerDir()){ //count up
          if(DISPLAY_SIZE<6 && td<60){ //under 1 min, 4-digit displays: [SS]CC--
            if(td>=1||lz) editDisplay(td,0,1,lz,true); else blankDisplay(0,1,true); //secs, leading per lz, fade
            editDisplay(tdc,2,3,td>=1||lz,false); //cents, leading if >=1sec or lz, no fade
            blankDisplay(4,5,true); //just in case 4-digit code's running on a 6-digit display, don't look ugly
          } else if(td<3600){ //under 1 hr: [MM][SS]CC
            if(td>=60||lz) editDisplay(td/60,0,1,lz,true); else blankDisplay(0,1,true); //mins, leading per lz, fade
            if(td>=1||lz) editDisplay(td%60,2,3,td>=60||lz,true); else blankDisplay(2,3,true); //secs, leading if >=1min or lz, fade
            editDisplay(tdc,4,5,td>=1||lz,false); //cents, leading if >=1sec or lz, no fade - hidden on 4-digit display
          } else { //over 1 hr: HHMMSS
            editDisplay(td/3600,0,1,lz,true); //hrs, leading per lz, fade
            editDisplay((td%3600)/60,2,3,true,true); //mins, leading, fade
            editDisplay(td%60,4,5,true,true); //secs, leading, fade
          }
        } else { //count down
          if(DISPLAY_SIZE<6 && td<3600){ //under 1 hr, 4-digit displays: [MM]SS--
            if(td>=60||lz) editDisplay(td/60,0,1,lz,true); else blankDisplay(0,1,true); //mins, leading per lz, fade
            if(td>=1||lz) editDisplay(td%60,2,3,td>=60||lz,true); else blankDisplay(2,3,true); //secs, leading if >=1min or lz, fade
            blankDisplay(4,5,true); //just in case 4-digit code's running on a 6-digit display, don't look ugly
          } else { //[HH][MM]SS
            if(td>=3600||lz) editDisplay(td/3600,0,1,lz,true); else blankDisplay(0,1,true); //hrs, leading per lz, fade
            if(td>=60||lz) editDisplay((td%3600)/60,2,3,td>=3600||lz,true); else blankDisplay(2,3,true); //mins, leading if >=1h or lz, fade
            editDisplay(td%60,4,5,td>=60||lz,true); //secs, leading if >=1m or lz, fade
          }
        }
        break;
      case FN_THERMOMETER: //thermometer TODO disable if rtc doesn't support it
        int temp; temp = rtcGetTemp();
        if(readEEPROM(45,false)==1) temp = temp*1.8 + 3200;
        //TODO another setting to apply offset?
        #ifdef SEVENSEG
          editDisplay(readEEPROM(45,false)==1?70:67,0,0,false,true); //"F" or "C" at pos 0
          if(temp<0) { //negative
            if(temp<=-10) {
              editDisplay(45,1,1,false,true); //negative sign at pos 1
              editDisplay(abs(temp)/100,2,3,false,true); //temp in pos 2-3, no leading zeros
            } else {
              blankDisplay(1,1,true); //blank at pos 1
              editDisplay(45,2,2,false,true); //negative sign at pos 2
              editDisplay(abs(temp)/100,3,3,false,true); //temp in pos 3, no leading zeros
            }
          } else { //positive
            editDisplay(abs(temp)/100,1,3,false,true); //no leading zeros
          }
        #else
          editDisplay(abs(temp)/100,0,3,(temp<0?true:false),true); //leading zeros if negative
        #endif
        //hundredths on seconds digits
        editDisplay(abs(temp)%100,4,5,true,true);
        break;
      case FN_TUBETEST:
        editDisplay(rtcGetSecond(),0);
        editDisplay(rtcGetSecond(),1);
        editDisplay(rtcGetSecond(),2);
        editDisplay(rtcGetSecond(),3);
        editDisplay(rtcGetSecond(),4);
        editDisplay(rtcGetSecond(),5);
      default: break;
    }//end switch
  } //end if fn running
  
  // if(false) { //DEBUG MODE: when display's not working, just write it to the console, with time. TODO create dummy display handler
  //   if(rtcGetHour()<10) Serial.print(F("0"));
  //   Serial.print(rtcGetHour(),DEC);
  //   Serial.print(F(":"));
  //   if(rtcGetMinute()<10) Serial.print(F("0"));
  //   Serial.print(rtcGetMinute(),DEC);
  //   Serial.print(F(":"));
  //   if(rtcGetSecond()<10) Serial.print(F("0"));
  //   Serial.print(rtcGetSecond(),DEC);
  //   Serial.print(F("   "));
  //   for(byte i=0; i<DISPLAY_SIZE; i++) {
  //     if(i%2==0 && i!=0) Serial.print(F(" ")); //spacer between units
  //     if(displayNext[i]>9) Serial.print(F("-")); //blanked digit
  //     else Serial.print(displayNext[i],DEC);
  //   }
  //   Serial.println();
  // }
  
  //Write display changes to console
  //for(byte w=0; w<6; w++) { if(displayNext[w]>9) Serial.print(F("-")); else Serial.print(displayNext[w],DEC); }
  //Serial.println();
       
} //end updateDisplay()

// void serialPrintDate(int y, byte m, byte d){
//   Serial.print(y,DEC); Serial.print(F("-"));
//   if(m<10) Serial.print(F("0")); Serial.print(m,DEC); Serial.print(F("-"));
//   if(d<10) Serial.print(F("0")); Serial.print(d,DEC);
// }
// void serialPrintTime(int mins){
//   if(mins/60<10) Serial.print(F("0")); Serial.print(mins/60,DEC); Serial.print(F(":"));
//   if(mins%60<10) Serial.print(F("0")); Serial.print(mins%60,DEC);
// }

//A snapshot of sun times, in minutes past midnight, calculated at clean time and when the date or time is changed.
//Need to capture this many, as we could be displaying these values at least through end of tomorrow depending on when cleaning happens.
#if ENABLE_SUN
byte sunDate = 0; //date of month when calculated ("today")
int sunSet0  = -1; //yesterday's set
int sunRise1 = -1; //today rise
int sunSet1  = -1; //today set
int sunRise2 = -1; //tomorrow rise
int sunSet2  = -1; //tomorrow set
int sunRise3 = -1; //day after tomorrow rise
void calcSun(){
  //Calculates sun times and stores them in the values above
  int y = rtcGetYear();
  int m = rtcGetMonth();
  int d = rtcGetDate();
  //Serial.print(millis(),DEC); Serial.println(F("blank display per calcsun"));
  //blankDisplay(0,5); //immediately blank display so we can fade in from it elegantly
  //TODO causes nixie blinking during initial startup and after ntp sync
  Dusk2Dawn here(float(readEEPROM(10,true))/10, float(readEEPROM(12,true))/10, (float(readEEPROM(14,false))-100)/4);
  //Today
  sunDate = d;
  sunRise1 = here.sunrise(y,m,d,isDST(y,m,d)); //TODO: unreliable if event is before time change on DST change day. Optionally if isDSTChangeDay() and event is <2h de-correct for it - maybe modify the library to do this - as when 2h overlaps in fall, we don't know whether the output has been precorrected.
  sunSet1  = here.sunset(y,m,d,isDST(y,m,d));
  // serialPrintDate(y,m,d);
  // Serial.print(F("  Rise ")); serialPrintTime(sunRise1);
  // Serial.print(F("  Set ")); serialPrintTime(sunSet1); Serial.println();
  //Yesterday
  d--; if(d<1){ m--; if(m<1){ y--; m=12; } d=daysInMonth(y,m); }
  sunSet0  = here.sunset(y,m,d,isDST(y,m,d));
  // serialPrintDate(y,m,d);
  // Serial.print(F("            "));
  // Serial.print(F("  Set ")); serialPrintTime(sunSet0); Serial.println();
  //Tomorrow
  d+=2; if(d>daysInMonth(y,m)){ d-=daysInMonth(y,m); m++; if(m>12){ m=1; y++; }}
  sunRise2 = here.sunrise(y,m,d,isDST(y,m,d));
  sunSet2  = here.sunset(y,m,d,isDST(y,m,d));
  // serialPrintDate(y,m,d);
  // Serial.print(F("  Rise ")); serialPrintTime(sunRise2);
  // Serial.print(F("  Set ")); serialPrintTime(sunSet2); Serial.println();
  //Day after tomorrow
  d++; if(d>daysInMonth(y,m)){ d=1; m++; if(m>12){ m=1; y++; }}
  sunRise3 = here.sunrise(y,m,d,isDST(y,m,d));
  // serialPrintDate(y,m,d);
  // Serial.print(F("  Rise ")); serialPrintTime(sunRise3); Serial.println();
} //end calcSun()
void displaySun(byte which, int d, int tod){
  //Displays sun times from previously calculated values
  //Old code to calculate sun at display time, with test serial output, is in commit 163ca33
  //which is 0=prev, 1=next
  int evtTime = 0; bool evtIsRise = 0;
  if(d==sunDate){ //displaying same day as calc
    //before sunrise: prev is calcday-1 sunset, next is calcday sunrise
    //daytime:        prev is calcday sunrise,  next is calcday sunset
    //after sunset:   prev is calcday sunset,   next is calcday+1 sunrise
    if(tod<sunRise1)     { evtIsRise = which;  evtTime = (!which?sunSet0:sunRise1); }
    else if(tod<sunSet1) { evtIsRise = !which; evtTime = (!which?sunRise1:sunSet1); }
    else                 { evtIsRise = which;  evtTime = (!which?sunSet1:sunRise2); }
  }
  else if(d==sunDate+1 || d==1){ //if there's been a midnight since the last calc – displaying day after calc (or month rollover)
    //before sunrise: prev is calcday sunset,    next is calcday+1 sunrise
    //daytime:        prev is calcday+1 sunrise, next is calcday+1 sunset
    //after sunset:   prev is calcday+1 sunset,  next is calcday+2 sunrise
    if(tod<sunRise2)     { evtIsRise = which;  evtTime = (!which?sunSet1:sunRise2); }
    else if(tod<sunSet2) { evtIsRise = !which; evtTime = (!which?sunRise2:sunSet2); }
    else                 { evtIsRise = which;  evtTime = (!which?sunSet2:sunRise3); }
  }
  else { evtIsRise = 0; evtTime = -1; } //date error
  if(evtTime<0){ //event won't happen? super north or south maybe TODO test this. In this case weather will need arbitrary dawn/dusk times
    blankDisplay(0,3,true);
  } else {
    byte hr = evtTime/60;
    if(readEEPROM(16,false)==1) hr = (hr==0?12:(hr>12?hr-12:hr)); //12/24h per settings
    editDisplay(hr, 0, 1, readEEPROM(19,false), true); //leading zero per settings
    editDisplay(evtTime%60, 2, 3, true, true);
  }
  #ifdef SEVENSEG
  if(evtIsRise) {
    editDisplay(85,4,4,0,true); editDisplay(80,5,5,0,true); //UP
  } else {
    editDisplay(100,4,4,0,true); editDisplay(110,5,5,0,true); //dn
  }
  #else
  blankDisplay(4, 4, true);
  editDisplay(evtIsRise, 5, 5, false, true);
  #endif
}
#else
//to give other fns something empty to call, when rise/set isn't enabled
void calcSun(){}
void displaySun(byte which, int d, int tod){}
#endif

void displayWeather(byte which){
  //shows high/low temp (for day/night respectively) in place of hour/min, and precipitation info in place of seconds
  //which==0: display for current sun period (after last sun event)
  //which==1: display for next sun period (after next sun event)
  //IoT: Show the weather for the current period (after last sun event): high (day) or low (night) and precip info//IoT: Show the weather for the period after the next sun event
}




////////// Hardware outputs //////////

void initOutputs() {
  if(PIEZO_PIN>=0) pinMode(PIEZO_PIN, OUTPUT);
  if(SWITCH_PIN>=0) { pinMode(SWITCH_PIN, OUTPUT); digitalWrite(SWITCH_PIN, HIGH); } //LOW = device on
  if(PULSE_PIN>=0) { pinMode(PULSE_PIN, OUTPUT); digitalWrite(PULSE_PIN, HIGH); } //LOW = device on
  if(BACKLIGHT_PIN>=0) pinMode(BACKLIGHT_PIN, OUTPUT);
  updateBacklight(); //set to initial value
}

//Signals are like songs made up of measures (usually 1sec each) tracked by the signalRemain counter.
//Measures are made up of steps such as piezo beeps and pulses, tracked by signalMeasureStep.
//When used with switch signal, it simply turns on at the start, and off at the end, like a clock radio – the measures just wait it out.
//Timed using ms() instead of millis() – see timer/chrono for details.
unsigned long signalMeasureStartTime = 0; //to keep track of individual measures
byte signalMeasureStep = 0; //step number, or 255 if waiting for next measure, or 0 if not signaling
void signalStart(byte sigFn, byte sigDur){
  //sigFn isn't necessarily the current fn, just the one generating the signal
  //sigDur is the number of measures to put on signalRemain,
  //   or 0 for a single "quick measure" as applicable (i.e. skipped in radio mode).
  //Special case: if sigFn==FN_ALARM, and sigDur>0, we'll use SIGNAL_DUR or SWITCH_DUR as appropriate.
  //If sigFn is given as -1 (255), we will use both the existing signalSource and signalPattern for purposes of configs and fibonacci.
  if(!(sigFn==255 && signalSource==FN_TIMER)) signalStop(); // if there is a signal going per the current signalSource, stop it - can only have one signal at a time – except if this is a forced FN_TIMER signal (for signaling runout options) which is cool to overlap timer sleep
  //except if this is a forced
  if(sigFn!=255) signalSource = sigFn;
  if(sigFn!=255) signalPattern = (
    (signalSource==FN_TOD && readEEPROM(21,false)==2)? -1: //special case: the pips
    getSignalPattern() //usual: get pattern from user settings
  );
  // Serial.print(F("signalStart, sigFn="));
  // Serial.print(sigFn,DEC);
  // Serial.print(F(", sigDur="));
  // Serial.print(sigDur,DEC);
  // Serial.print(F(", source="));
  // Serial.print(signalSource,DEC);
  // Serial.print(F(", pattern="));
  // Serial.print(signalPattern,DEC);
  // Serial.println();
  
  //Start a measure for cycleSignal to pick up. For "quick measures" we won't set signalRemain.
  signalMeasureStartTime = ms();
  signalMeasureStep = 1; //waiting to start a new measure
  if(sigDur!=0){ //long-duration signal (alarm, sleep, etc) - set signalRemain
    //If switch signal, except if this is a forced FN_TIMER signal (for signaling runout options)
    if(getSignalOutput()==1 && !(sigFn==255 && signalSource==FN_TIMER)) { //turn it on now
      signalRemain = (sigFn==FN_ALARM||sigFn==FN_ALARM2? SWITCH_DUR: sigDur); //For alarm signal, use switch signal duration from config (eg 2hr)
      digitalWrite(SWITCH_PIN,LOW); updateBacklight(); //LOW = device on
      //Serial.print(millis(),DEC); Serial.println(F(" Switch signal on, signalStart"));
    } else { //start piezo or pulse signal. If neither is present, this will have no effect since cycleSignal will clear it
      signalRemain = (sigFn==FN_ALARM||sigFn==FN_ALARM2? SIGNAL_DUR: sigDur); //For alarm signal, use signal duration from config (eg 2min)
    }
  }
  //cycleSignal will pick up from here
} //end signalStart()
void signalStop(){ //stop current signal and clear out signal timer if applicable
  //Serial.println(F("signalStop"));
  signalRemain = 0; snoozeRemain = 0; signalMeasureStep = 0;
  if(getSignalOutput()==0 && PIEZO_PIN>=0) noTone(PIEZO_PIN);
  if(getSignalOutput()==1 && SWITCH_PIN>=0){
    digitalWrite(SWITCH_PIN,HIGH); //LOW = device on
    //Serial.print(millis(),DEC); Serial.println(F(" Switch signal off, signalStop"));
  }
  if(getSignalOutput()==2 && PULSE_PIN>=0){
    digitalWrite(PULSE_PIN,HIGH); //LOW = device on
    //Serial.print(millis(),DEC); Serial.println(F(" Pulse signal off, signalStop"));
  }
  updateBacklight();
} //end signalStop()
void cycleSignal(){
  //Called on every loop to control the signal.
  word measureDur = 1000; //interval between measure starts, ms - beep pattern can customize this
  if(signalMeasureStep){ //if there's a measure going (or waiting for a new one)
    if((getSignalOutput()==0 || (signalRemain==0 && signalSource==FN_TIMER)) && PIEZO_PIN>=0) { // beeper, or single measure for FN_TIMER runout setting
      //Since tone() handles the duration of each beep,
      //we only need to use signalMeasureStep to track beep starts; they'll stop on their own.
      byte bc = 0; //this many beeps
      word bd = 0; //of this ms duration and separation
      //Could also set a custom value for the measure interval, if we wanted it to go faster or slower than 1/sec
      switch(signalPattern){
        case 255: //the pips: 100ms, except the last is 500ms
          bc = 1; bd = (signalRemain>0? 100: 500); break;
        case 0: //long (one 1/2-second beep)
          bc = 1; bd = 500; break;
        case 1: default: //short (one 1/4-second beep)
          bc = 1; bd = 250; break;
        case 2: case 5: //double and cuckoo (two 1/8-second beeps)
          bc = 2; bd = 125; break;
        case 3: //triple (three 1/12-second beeps)
          bc = 3; bd = 83; break;
        case 4: //quad (four 1/16-second beeps)
          bc = 4; bd = 62; break;
      }
      //See if it's time to start a new measure
      if(signalMeasureStep==255 && (unsigned long)(ms()-signalMeasureStartTime)>=measureDur){
        //Serial.println(F("Starting new measure, sPS -1 -> 1"));
        signalMeasureStartTime += measureDur;
        signalMeasureStep = 1;
      }
      //See if it's time to start a new beep
      if((unsigned long)(ms()-signalMeasureStartTime)>=(signalMeasureStep-1)*bd*2){
        word piezoPitch = (signalPattern==5 && signalMeasureStep==2? getSignalPitch()*0.7937: //cuckoo: go down major third (2^(-4/12)) on 2nd beep
          (signalPattern==255? 1000: //the pips: use 1000Hz just like the Beeb
            (signalRemain==0 && signalSource==FN_TIMER? getHz(69): //FN_TIMER runout setting: use timer start pitch
              getSignalPitch() //usual: get pitch from user settings
            )
          )
        );
        noTone(PIEZO_PIN); tone(PIEZO_PIN, piezoPitch, bd);
        //Serial.print(F("Starting beep, sPS "));
        //Serial.print(signalMeasureStep,DEC);
        //Serial.print(F(" -> "));
        //Set up for the next event
        if(signalMeasureStep<bc) {
          signalMeasureStep++; //set up for another beep in this measure
          //Serial.println(signalMeasureStep,DEC);
        }
        else {
          if(signalRemain) signalRemain--; //this measure is done
          if(signalRemain) signalMeasureStep = 255; //if more measures, set up to start another measure
          else signalMeasureStep = 0; //otherwise, go idle - not using signalStop so as to let the beep expire on its own & fibonacci snooze continue
          //Serial.println(signalMeasureStep,DEC);
        }
      }
    } //end beeper
    else if(getSignalOutput()==2 && PULSE_PIN>=0){ //pulse signal
      //We don't follow the beep pattern here, we simply energize the pulse signal for PULSE_LENGTH time
      //Unlike beeper, we need to use a signalMeasureStep (2) to stop the pulse.
      //See if it's time to start a new measure
      if(signalMeasureStep==255 && (unsigned long)(ms()-signalMeasureStartTime)>=measureDur){
        //Serial.println(F("Starting new measure, sPS -1 -> 1"));
        signalMeasureStartTime += measureDur;
        signalMeasureStep = 1;
      }
      //Upon new measure, start the pulse immediately
      if(signalMeasureStep==1){
        digitalWrite(PULSE_PIN,LOW); updateBacklight(); //LOW = device on
        //Serial.print(millis(),DEC); Serial.println(F(" Pulse signal on, cycleSignal"));
        signalMeasureStep = 2; //set it up to stop
      }
      //See if it's time to stop the pulse
      else if(signalMeasureStep==2 && (unsigned long)(ms()-signalMeasureStartTime)>=PULSE_LENGTH) {
        digitalWrite(PULSE_PIN,HIGH); updateBacklight(); //LOW = device on
        //Serial.print(millis(),DEC); Serial.println(F(" Pulse signal off, cycleSignal"));
        //Set up for the next event
        if(signalRemain) signalRemain--; //this measure is done
        if(signalRemain) signalMeasureStep = 255; //if more measures, set up to start another measure
        else signalMeasureStep = 0; //otherwise, go idle - not using signalStop so as to let the beep expire on its own & fibonacci snooze continue
      }
    } //end pulse signal
    else { //switch signal / default
      //Simply decrement signalRemain until it runs out - signalMeasureStep doesn't matter as long as it stays nonzero as at start
      //See if it's time to start a new measure
      if((unsigned long)(ms()-signalMeasureStartTime)>=measureDur){
        //Serial.println(F("Starting new measure, sPS -1 -> 1"));
        signalMeasureStartTime += measureDur;
      }
      //Set up for the next event
      if(signalRemain) signalRemain--; //this measure is done - but no need to change signalMeasureStep as we haven't changed it
      if(!signalRemain) signalStop(); //go idle - will kill the switch signal
    } //end switch signal / default
  } //end if there's a measure going
} //end cycleSignal()
word getSignalPitch(){ //for current signal: chime, timer, or (default) alarm
  return getHz(readEEPROM((signalSource==FN_TOD?41:(signalSource==FN_TIMER?40:39)),false));
}
word getHz(byte note){
  //Given a piano key note, return frequency
  byte relnote = note-49; //signed, relative to concert A
  float reloct = relnote/12.0; //signed
  word mult = 440*pow(2,reloct);
  return mult;
}
byte getSignalOutput(){ //for current signal: chime, timer, or (default) alarm: 0=piezo, 1=switch, 2=pulse
  return readEEPROM((signalSource==FN_TOD?44:(signalSource==FN_TIMER?43:42)),false);
}
byte getSignalPattern(){ //for current signal: chime, timer, or (default) alarm: (applies only to piezo)
  //0 = long (1/2-second beep)
  //1 = short (1/4-second beep)
  //2 = double (two 1/8-second beeps)
  //5 = cuckoo (two 1/8-second beeps, descending major third)
  //3 = triple (three 1/12-second beeps)
  //4 = quad (four 1/16-second beeps)
  return readEEPROM((signalSource==FN_TOD?49:(signalSource==FN_TIMER?48:47)),false);
}
void quickBeep(int pitch){
  //This is separate from signal system
  //F5 = 57
  //C6 = 64
  //F6 = 69
  //G6 = 71
  //A6 = 73
  //B6 = 75 //second loudest
  //C7 = 76 //loudest
  //F7 = 81
  if(PIEZO_PIN>=0) { noTone(PIEZO_PIN); tone(PIEZO_PIN, getHz(pitch), 100); }
}
void quickBeepPattern(int source, int pattern){
  signalPattern = pattern;
  signalSource = source;
  signalStart(-1,1); //Play a sample using the above source and pattern
}

byte backlightNow = 0;
byte backlightTarget = 0;
#ifndef BACKLIGHT_FADE
#define BACKLIGHT_FADE 0 //no PWM fading
#endif
void updateBacklight(){
  //Run whenever something is changed that might affect the backlight state: initial (initOutputs), signal start/stop, switch signal on/off, setting change
  if(BACKLIGHT_PIN>=0) {
    switch(readEEPROM(26,false)){
      case 0: //always off
        backlightTarget = 0;
        //Serial.println(F("Backlight off always"));
        break;
      case 1: //always on
        backlightTarget = 255;
        //Serial.println(F("Backlight on always"));
        break;
      case 2: //on, but follow display dimming/away
        backlightTarget = (displayBrightness==2? 255: (displayBrightness==1? 127: 0));
        //TODO accommodate ambient light stuff
        //Serial.print(displayBrightness==2? F("Backlight on"): (displayBrightness==1? F("Backlight dim"): F("Backlight off"))); Serial.println(F(" per dim state"));
        break;
      case 3: //off, but on when alarm/timer sounds
        backlightTarget = (signalRemain && (signalSource==FN_ALARM || signalSource==FN_TIMER)? 255: 0);
        //Serial.print(signalRemain && (signalSource==FN_ALARM || signalSource==FN_TIMER)?F("Backlight on"):F("Backlight off")); Serial.println(F(" per alarm/timer"));
        break;
      case 4: //off, but on with switch signal
        if(SWITCH_PIN>=0) {
          backlightTarget = (!digitalRead(SWITCH_PIN)? 255: 0); //LOW = device on
          //Serial.print(!digitalRead(SWITCH_PIN)? F("Backlight on"): F("Backlight off")); Serial.println(F(" per switch signal"));
        }
        break;
      default: break;
    } //end switch
  } //if BACKLIGHT_PIN
} //end updateBacklight

/* The variable display brightness and LED backlighting are (if opted for) faded from one state to another, but unlike the nixie fading – which requires very precise timing to prevent flicker, and is therefore the only allowed use of delay() (TODO delayMicroseconds()) in this sketch – these fade steps can be iterated with interval timing with millis() like everything else, just with a much smaller delay. I'm calling this "tweening." If we're using the nixie display, its delay()s will slow down the loop, so the tweening steps can take place once per loop. If not, the loop will run much faster, so we will need to add a delay – here it is. */
#ifndef TWEENING_DELAY
  #ifdef DISPLAY_NIXIE
  #define TWEENING_DELAY 50
  #else
  #define TWEENING_DELAY 50
  #endif
#endif
/* Tweening is currently linear, with steps of this size (the full range being 0-255). The display may change brightness in more discrete steps if it doesn't have 255 possible brightness levels (e.g. MAX7219 and HT16K33 having just 16 steps), but will change more slowly to compensate (since it is converted from a byte value). */
#ifndef TWEENING_STEP
#define TWEENING_STEP 10
#endif

unsigned long tweenLast = 0;
void cycleTweening() {
  //Allows us to fade the backlight and variable display brightness to their targets.
  if((unsigned long)(millis()-tweenLast)>=TWEENING_DELAY){
    tweenLast = millis();
    
#ifdef LIGHTSENSOR
    //Ambient lighting reading
    if(ambientLightLevel != ambientLightLevelActual) {
      //Serial.print(F("cycleTweening: ambientLightLevel from ")); Serial.print(ambientLightLevel,DEC);
      if(ambientLightLevel < ambientLightLevelActual) { //fade up
        ambientLightLevel = (ambientLightLevelActual-ambientLightLevel <= TWEENING_STEP? ambientLightLevelActual: ambientLightLevel+TWEENING_STEP);
      } else { //fade down
        ambientLightLevel = (ambientLightLevel-ambientLightLevelActual <= TWEENING_STEP? ambientLightLevelActual: ambientLightLevel-TWEENING_STEP);
      }
      //Serial.print(F(" to ")); Serial.print(ambientLightLevel,DEC); Serial.print(F(" on the way to ")); Serial.print(ambientLightLevelActual,DEC);
    }
#endif
  
    //Backlight
    //TODO: it appears setting analogWrite(pin,0) does not completely turn the backlight off. Anything else we could do?
    if(backlightNow != backlightTarget) {
      if(BACKLIGHT_FADE){ //PWM enabled
        if(backlightNow < backlightTarget) { //fade up
          backlightNow = (backlightTarget-backlightNow <= TWEENING_STEP? backlightTarget: backlightNow+TWEENING_STEP);
        } else { //fade down
          backlightNow = (backlightNow-backlightTarget <= TWEENING_STEP? backlightTarget: backlightNow-TWEENING_STEP);
        }
        // Serial.print(backlightNow,DEC);
        // Serial.print(F(" => "));
        // Serial.println(backlightTarget,DEC);
        analogWrite(BACKLIGHT_PIN,backlightNow);
      } else { //just switch
        backlightNow = backlightTarget = (backlightTarget<255? 0: 255);
        digitalWrite(BACKLIGHT_PIN,(backlightNow?LOW:HIGH)); //LOW = device on
      }
    }
  }
} //end cycleTweening

byte getVersionPart(byte part){ //used by network.cpp
  switch(part){
    case 0: return vMajor; break;
    case 1: return vMinor; break;
    case 2: return vPatch; break;
    case 3: default: return vDev; break;
  }
}