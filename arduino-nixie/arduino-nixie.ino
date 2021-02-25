// Digital clock code for the Arduino Nano in RLB Designs' Universal Nixie Driver Board
// featuring timekeeping by DS3231 RTC, driving up to six digits multiplexed 3x2 via two SN74141 driver chips
// Sketch by Luke McKenzie (luke@theclockspot.com) - https://github.com/clockspot/arduino-nixie
// Inspired by original sketch by Robin Birtles (rlb-designs.com) and Chris Gerekos
// Display cycling code derived from http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt

////////// Hardware configuration //////////
//Include the config file that matches your hardware setup. If needed, duplicate an existing one.

#include "configs/sample.h"

////////// Software version //////////
const byte vMajor = 1;
const byte vMinor = 9;
const byte vPatch = 0;

////////// Other includes, global consts, and vars //////////

#if ENABLE_DATE_RISESET
  #include <Dusk2Dawn.h> //DM Kiski - unlicensed - install in your Arduino IDE if needed
#endif
#if CTRL_UPDN_TYPE==2 //rotary encoder
  #include <Encoder.h> //Paul Stoffregen - install in your Arduino IDE if needed
#endif

/*Some variables are backed by persistent storage. These are referred to by their location in that storage. See storage.cpp.
Values for these are bytes (0 to 255) or ints (-32768 to 32767) where high byte is loc and low byte is loc+1.
In updateDisplay(), special setting formats are deduced from the option max value (max 1439 is a time of day, max 156 is a UTC offset, etc)
and whether a value is an int or byte is deduced from whether the max value > 255.
If adding more variables, be sure to increase STORAGE_SPACE in storage.cpp (and use the ones marked [free] below first).

These ones are set outside the options menu (defaults defined in initEEPROM()):
  0-1 Alarm time, mins
  2 Alarm on
  3 [free]
  4 Day count direction
  5 Day count month
  6 Day count date
  7 Function preset (done by Alt when not power-switching)
  8 Functions/pages enabled (bitmask, dynamic) TODO
    const unsigned int FN_TIMER = 1<<0; //1
    const unsigned int FN_DAYCOUNT = 1<<1; //2
    const unsigned int FN_SUN = 1<<2; //4
    const unsigned int FN_WEATHER = 1<<3; //8
  9 [free]
  15 DST on

These ones are set inside the options menu (defaults defined in arrays below).
Some are skipped when they wouldn't apply to a given clock's hardware config, see fnOptScroll(); these ones will also be set at startup to the start= values, see setup(). Otherwise, make sure these ones' defaults work for all configs.
  10-11 Latitude
  12-13 Longitude
  14 UTC offset in quarter-hours plus 100 - range is 52 (-12h or -48qh, US Minor Outlying Islands) to 156 (+14h or +56qh, Kiribati)
  16 Time format
  17 Date format 
  18 Display date during time
  19 Leading zeros in time hour, calendar, and chrono/timer
  20 Digit fade duration
  21 Strike - skipped when no piezo and relay is switch (start=0)
  22 Auto DST
  23 Alarm days
  24 Alarm snooze
  25 [free] - formerly Timer interval mode - skipped when no piezo and relay is switch (start=0)
  26 LED circuit behavior - skipped when no led pin
  27 Night shutoff
  28-29 Night start, mins
  30-31 Night end, mins
  32 Away shutoff
  33 First day of workweek
  34 Last day of workweek
  35-36 Work starts at, mins
  37-38 Work ends at, mins
  39 Alarm beeper pitch - skipped when no piezo
  40 Timer beeper pitch - skipped when no piezo
  41 Strike beeper pitch - skipped when no piezo
  42 Alarm signal, 0=beeper, 1=relay - skipped when no relay (start=0) or no piezo (start=0)
  43 Timer signal - skipped when no relay (start=0) or no piezo (start=1)
  44 Strike signal - skipped when no pulse relay (start=0) or no piezo (start=1)
  45 Temperature format - skipped when !ENABLE_TEMP_FN TODO also useful for weather display
  46 Anti-cathode poisoning
  47 Alarm beeper pattern - skipped when no piezo
  48 Timer beeper pattern - skipped when no piezo
  49 Strike beeper pattern - skipped when no piezo
  50 Alarm Fibonacci mode
*/

//Options menu numbers (displayed in UI and readme), locs, and default/min/max/current values.
//Option numbers/order can be changed (though try to avoid for user convenience);
//but option locs should be maintained so EEPROM doesn't have to be reset after an upgrade.
//The current values array is wasteful, since they have to all be ints here; but there should be room. We will initialize them at startup.
//                       General                    Alarm              Timer        Strike       Night and away shutoff           Geo
const byte optsNum[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15, 21,22,23, 30,31,32,33, 40,  41,  42,43,44,45,  46,  47,    50,   51, 52};
const byte optsLoc[] = {16,17,18,19,20,22,26,46,45, 23,42,39,47,24,50, 43,40,48, 21,44,41,49, 27,  28,  30,32,33,34,  35,  37,    10,   12, 14};
const  int optsDef[] = { 2, 1, 0, 0, 5, 0, 1, 0, 0,  0, 0,76, 4, 9, 0,  0,76, 2,  0, 0,68, 5,  0,1320, 360, 0, 1, 5, 480,1080,     0,    0,100};
const  int optsMin[] = { 1, 1, 0, 0, 0, 0, 0, 0, 0,  0, 0,49, 0, 0, 0,  0,49, 0,  0, 0,49, 0,  0,   0,   0, 0, 0, 0,   0,   0,  -900,-1800, 52};
const  int optsMax[] = { 2, 5, 3, 1,20, 6, 4, 2, 1,  2, 1,88, 5,60, 1,  1,88, 5,  4, 1,88, 5,  2,1439,1439, 2, 6, 6,1439,1439,   900, 1800,156};

//The rest of these variables are not backed by persistent storage, so they are regular named vars.

// Hardware inputs and value setting
byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()
unsigned long inputLast = 0; //When a button was last pressed
int inputLastTODMins = 0; //time of day, in minutes past midnight, when button was pressed. Used in paginated functions so they all reflect the same time of day.
#if CTRL_UPDN_TYPE==2 //rotary encoder
  Encoder rot(CTRL_DN,CTRL_UP);
#endif

// Functions and pages
// Unique IDs - see also fnScroll
const byte fnIsTime = 0; //time of day
const byte fnIsDate = 1; //date, with optional day counter and sunrise/sunset pages
const byte fnIsAlarm = 2; //alarm time
const byte fnIsTimer = 3; //countdown timer and chronograph
const byte fnIsTemp = 4; //temperature per rtc – will likely read high
const byte fnIsTest = 5; //simply cycles all tubes
const byte fnOpts = 201; //fn values from here to 255 correspond to options in the options menu
byte fn = 0; //currently displayed fn per above
byte fnPg = 0; //allows a function to have multiple pages
byte fnSetPg = 0; //whether this function is currently being set, and which option/page it's on
 int fnSetVal; //the value currently being set, if any
 int fnSetValMin; //min possible
 int fnSetValMax; //max possible
bool fnSetValVel; //whether it supports velocity setting (if max-min > 30)
 int fnSetValDate[3]; //holder for newly set date, so we can set it in 3 stages but set the RTC only once
bool fnSetValDid; //false when starting a set; true when value is changed - to detect if value was not changed

//the calendar function page numbers, depending on which ones are enabled. See findFnAndPageNumbers
byte fnDatePages = 1;
byte fnDateCounter = 255;
byte fnDateSunlast = 255;
byte fnDateWeathernow = 255;
byte fnDateSunnext = 255;
byte fnDateWeathernext = 255;

// Volatile running values used throughout the code. (Others are defined right above the method that uses them)
bool alarmSkip = 0;
byte signalSource = 0; //which function triggered the signal - fnIsTime (chime), fnIsAlarm, or fnIsTimer
byte signalPattern = 0; //the pattern for that source
word signalRemain = 0; //alarm/timer signal timeout counter, seconds
word snoozeRemain = 0; //snooze timeout counter, seconds
byte timerState = 0; //bit 0 is stop/run, bit 1 is down/up, bit 2 is runout repeat / short signal, bit 3 is runout chrono, bit 4 is lap display
word timerInitialMins = 0; //timer original duration setting, minutes - up to 99h 59m (5999m)
word timerInitialSecs = 0; //timer original duration setting, seconds - up to 59s (could be a byte, but I had trouble casting to unsigned int when doing the math to set timerTime)
unsigned long timerTime = 0; //timestamp of timer target / chrono origin (while running) or duration (while stopped)
unsigned long timerLapTime = 0; 
const byte millisCorrectionInterval = 30; //used to calibrate millis() to RTC for timer/chrono purposes
unsigned long millisAtLastCheck = 0;
word unoffRemain = 0; //un-off (briefly turn on tubes during full night/away shutoff) timeout counter, seconds
byte displayDim = 2; //dim per display or function: 2=normal, 1=dim, 0=off

byte versionRemain = 3; //display version at start

//Utility functions that are called by both the main code and the below includes
//Placed here so I can avoid making header files for the moment
byte daysInMonth(word y, byte m){
  if(m==2) return (y%4==0 && (y%100!=0 || y%400==0) ? 29 : 28);
  //https://cmcenroe.me/2014/12/05/days-in-month-formula.html
  else return (28 + ((m + (m/8)) % 2) + (2 % m) + (2 * (1/m)));
}

//These cpp files contain code that is conditionally included
//based on the available hardware and settings in the config file.
//TODO revisit - This is probably not the right way to do this –
//these are being compiled (or not) as part of this file,
//rather than independently, with header files included here –
//but it seems to work, as long as they don't reference later functions.
//All variants of each type (disp, rtc, etc) should define the same functions.
#define STORAGE
#include "storage.cpp";
#include "dispNixie.cpp" //for a SN74141-multiplexed nixie array
#include "dispMAX7219.cpp" //for a SPI MAX7219 8x8 LED array
#include "rtcDS3231.cpp" //for an I2C DS3231 RTC module
#include "rtcMillis.cpp" //for no RTC


////////// Main code control //////////

void setup(){
  Serial.begin(9600);
  //while(!Serial); //TODO 33 IoT only
  rtcInit();
  initInputs();
  delay(100); //prevents the below from firing in the event there's a capacitor stabilizing the input, which can read low falsely
  initEEPROM(readInput(CTRL_SEL)==LOW); //Do a hard init of EEPROM if button is held; else do a soft init to make sure vals in range
  //Some options need to be set to a fixed value per the configuration.
  //These options will also be skipped in fnOptScroll so the user can't change them.
  if(RELAY_PIN<0 || PIEZO_PIN<0) { //If no relay or no piezo, set each signal output to [if no relay, then piezo; else relay]  
    writeEEPROM(42,(RELAY_PIN<0?0:1),false); //alarm
    writeEEPROM(43,(RELAY_PIN<0?0:1),false); //timer
    writeEEPROM(44,(RELAY_PIN<0?0:1),false); //strike
  }
  if(PIEZO_PIN>=0 && RELAY_MODE==0) { //If piezo and switched relay
    writeEEPROM(44,0,false); //strike forced to piezo
  }
  if(PIEZO_PIN<0 && RELAY_MODE==0) { //If switched relay and no piezo
    writeEEPROM(21,0,false); //turn off strike
    writeEEPROM(50,0,false); //turn off fibonacci mode
  }
  if(!ENABLE_ALARM_FN) writeEEPROM(2,0,false); //force alarm off if disabled in config
  else if(!ENABLE_SOFT_ALARM_SWITCH) writeEEPROM(2,1,false); //force alarm on if software switch is disabled
  switch(readEEPROM(7,false)){ //if the preset is set to a function that is no longer enabled, use alarm if enabled, else use time
    case fnIsDate: if(!ENABLE_DATE_FN) writeEEPROM(7,(ENABLE_ALARM_FN?fnIsAlarm:fnIsTime),false); break;
    case fnIsAlarm: if(!ENABLE_ALARM_FN) writeEEPROM(7,fnIsTime,false); break;
    case fnIsTimer: if(!ENABLE_TIMER_FN) writeEEPROM(7,(ENABLE_ALARM_FN?fnIsAlarm:fnIsTime),false); break;
    case fnIsTemp: if(!ENABLE_TEMP_FN) writeEEPROM(7,(ENABLE_ALARM_FN?fnIsAlarm:fnIsTime),false); break;
    case fnIsTest: if(!ENABLE_TUBETEST_FN) writeEEPROM(7,(ENABLE_ALARM_FN?fnIsAlarm:fnIsTime),false); break;
    default: writeEEPROM(7,(ENABLE_ALARM_FN?fnIsAlarm:fnIsTime),false); break;
  }
  if(!ENABLE_ALARM_AUTOSKIP) writeEEPROM(23,0,false); //alarm autoskip off
  if(!ENABLE_ALARM_FIBONACCI) writeEEPROM(50,0,false); //fibonacci off
  if(!ENABLE_TIME_CHIME) writeEEPROM(21,0,false); //chime off
  if(!ENABLE_SHUTOFF_NIGHT) writeEEPROM(27,0,false); //night shutoff off
  if(!ENABLE_SHUTOFF_AWAY) writeEEPROM(32,0,false); //away shutoff off
  //if LED circuit is not switched (v5.0 board), the LED menu setting (eeprom 26) doesn't matter
  findFnAndPageNumbers(); //initial values
  initDisplay();
  initOutputs(); //depends on some EEPROM settings
}

void loop(){
  //Every loop cycle, check the RTC and inputs (previously polled, but works fine without and less flicker)
  //checkEffects(false); //cleaning and scrolling display effects - not handled by checkRTC since they have their own timing
  checkRTC(false); //if clock has ticked, decrement timer if running, and updateDisplay
  millisApplyDrift();
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  #if CTRL_UPDN_TYPE==1 //buttons
    doSetHold(false); //if inputs have been held, this will do more things + updateDisplay as needed
  #endif
  cycleTimer();
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
  cycleLEDs();
  cycleSignal();
}


////////// Control inputs //////////

void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  pinMode(CTRL_SEL, INPUT_PULLUP);
  pinMode(CTRL_UP, INPUT_PULLUP);
  pinMode(CTRL_DN, INPUT_PULLUP);
  #if CTRL_ALT!=0
    pinMode(CTRL_ALT, INPUT_PULLUP);
  #endif
}

void checkInputs(){
  //TODO potential issue: if user only means to rotate or push encoder but does both?
  checkBtn(CTRL_SEL); //select
  #if CTRL_UPDN_TYPE==1 //buttons
    checkBtn(CTRL_UP); checkBtn(CTRL_DN);
  #endif
  #if CTRL_UPDN_TYPE==2 //rotary encoder
    checkRot();
  #endif
  #if CTRL_ALT!=0 //alt (if equipped)
    checkBtn(CTRL_ALT);
  #endif
}

bool readInput(byte pin){
  if(pin==A6 || pin==A7) return analogRead(pin)<100?0:1; //analog-only pins
  else return digitalRead(pin);
}
void checkBtn(byte btn){
  //Changes in momentary buttons, LOW = pressed.
  //When a button event has occurred, will call ctrlEvt
  bool bnow = readInput(btn);
  unsigned long now = millis();
  //If the button has just been pressed, and no other buttons are in use...
  if(btnCur==0 && bnow==LOW) {
    btnCur = btn; btnCurHeld = 0; inputLast = now; inputLastTODMins = rtcGetTOD();
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(btnCur==btn && bnow==LOW) {
    if((unsigned long)(now-inputLast)>=CTRL_HOLD_LONG_DUR && btnCurHeld < 3) { //account for rollover
      btnCurHeld = 3;
      ctrlEvt(btn,3); //hey, the button has been long-held
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_SHORT_DUR && btnCurHeld < 2) {
      btnCurHeld = 2;
      ctrlEvt(btn,2); //hey, the button has been short-held
    }
  }
  //If the button has just been released...
  if(btnCur==btn && bnow==HIGH) {
    btnCur = 0;
    if(btnCurHeld < 4) ctrlEvt(btn,0); //hey, the button was released
    btnCurHeld = 0;
  }
}
void btnStop(){
  //In some cases, when handling btn evt 1/2/3, we may call this so following events 2/3/0 won't cause unintended behavior (e.g. after a fn change, or going in or out of set)
  btnCurHeld = 4;
}

bool rotVel = 0; //high velocity setting (x10 rather than x1)
#if CTRL_UPDN_TYPE==2 //rotary encoder
unsigned long rotLastStep = 0; //timestamp of last completed step (detent)
int rotLastVal = 0;
void checkRot(){
  //Changes in rotary encoder. When rotation(s) occur, will call ctrlEvt to simulate btn presses. During setting, ctrlEvt will take rotVel into account.
  int rotCurVal = rot.read();
  if(rotCurVal!=rotLastVal){ //we've sensed a state change
    rotLastVal = rotCurVal;
    if(rotCurVal>=4 || rotCurVal<=-4){ //we've completed a step of 4 states (this library doesn't seem to drop states much, so this is reasonably reliable)
      unsigned long now = millis();
      inputLast = now; inputLastTODMins = rtcGetTOD();
      if((unsigned long)(now-rotLastStep)<=ROT_VEL_START) rotVel = 1; //kick into high velocity setting (x10)
      else if((unsigned long)(now-rotLastStep)>=ROT_VEL_STOP) rotVel = 0; //fall into low velocity setting (x1)
      rotLastStep = now;
      while(rotCurVal>=4) { rotCurVal-=4; ctrlEvt(CTRL_UP,1); }
      while(rotCurVal<=-4) { rotCurVal+=4; ctrlEvt(CTRL_DN,1); }
      rot.write(rotCurVal);
    }
  }
} //end checkRot()
#endif


////////// Input handling and value setting //////////

void ctrlEvt(byte ctrl, byte evt){
  //Handle control events (from checkBtn or checkRot), based on current fn and set state.
  //evt: 1=press, 2=short hold, 3=long hold, 0=release.
  //We only handle press evts for adj ctrls, as that's the only evt encoders generate.
  //But we can handle short and long holds and releases for the sel ctrls (always buttons).

  //If the signal is going, any press should silence it
  if(signalRemain>0 && evt==1){
    signalStop();
    if(signalSource==fnIsAlarm) { //If this was the alarm
      //If the alarm is using the switched relay and this is the Alt button; or if alarm is *not* using the switched relay and this is Fibonacci mode; don't set the snooze
      if((RELAY_MODE==0 && readEEPROM(42,false)==1 && CTRL_ALT!=0 && ctrl==CTRL_ALT) || (readEEPROM(50,false) && !(RELAY_PIN>=0 && RELAY_MODE==0 && readEEPROM(42,false)==1))) {
        quickBeep(64); //Short signal to indicate the alarm has been silenced until tomorrow
        displayBlink(); //to indicate this as well
      } else { //start snooze
        snoozeRemain = readEEPROM(24,false)*60; //snoozeRemain is seconds, but snooze duration is minutes
      }
    }
    btnStop();
    return;
  }
  //If the snooze is going, any press should cancel it, with a signal
  if(snoozeRemain>0 && evt==1){
    snoozeRemain = 0;
    quickBeep(64); //Short signal to indicate the alarm has been silenced until tomorrow
    displayBlink(); //to indicate this as well
    btnStop();
    return;
  }
  // //TODO NIXIE
  // //If the clean is going, any press should cancel it, with a display update
  // if(cleanRemain>0 && evt==1){
  //   cleanRemain = 0;
  //   btnStop();
  //   updateDisplay();
  //   return;
  // }
  // //TODO NIXIE??
  // //If a scroll is waiting to scroll out, cancel it, and let the button event do what it will
  // if(scrollRemain==-128 && evt==1){
  //   scrollRemain = 0;
  // }
  // //If a scroll is going, fast-forward to end of scroll in/out - see also checkRTC
  // else if(scrollRemain!=0 && evt==1){
  //   btnStop();
  //   if(scrollRemain>0) scrollRemain = 1;
  //   else scrollRemain = -1;
  //   checkEffects(true);
  //   return;
  // }
  //If the version display is going, any press should cancel it, with a display update
  if(versionRemain>0 && evt==1){
    versionRemain = 0;
    btnStop();
    updateDisplay();
    return;
  }
  
  //Is it a press for an un-off?
  unoffRemain = UNOFF_DUR; //always do this so continued button presses during an unoff keep it alive
  if(displayDim==0 && evt==1) {
    updateDisplay();
    btnStop();
    return;
  }
  
  if(fn < fnOpts) { //normal fn running/setting (not in options menu)

    if(evt==3 && ctrl==CTRL_SEL) { //CTRL_SEL long hold: enter options menu
      btnStop();
      fn = fnOpts;
      clearSet(); //don't need updateDisplay() here because this calls updateRTC with force=true
      return;
    }
    
    if(!fnSetPg) { //fn running
      if(evt==2 && ctrl==CTRL_SEL) { //CTRL_SEL hold: enter setting mode
        switch(fn){
          case fnIsTime: //set mins
            startSet(rtcGetTOD(),0,1439,1); break; //TODO pull from time
          case fnIsDate: //depends what page we're on
            if(fnPg==0){ //regular date display: set year //TODO pull from time
              fnSetValDate[1]=rtcGetMonth(), fnSetValDate[2]=rtcGetDate(); startSet(rtcGetYear(),0,9999,1);
            } else if(fnPg==fnDateCounter){ //month, date, direction
              startSet(readEEPROM(5,false),1,12,1);
            } else if(fnPg==fnDateSunlast || fnPg==fnDateSunnext){ //lat and long
              //TODO
            } else if(fnPg==fnDateWeathernow || fnDateWeathernext){ //temperature units??
              //TODO
            } break;
          case fnIsAlarm: //set mins
            startSet(readEEPROM(0,true),0,1439,1); break;
          case fnIsTimer: //set mins
            if(timerTime!=0 || timerState&1) { timerClear(); } // updateDisplay(); break; } //If the timer is nonzero or running, zero it. But rather than stop there, just go straight into setting – since adjDn (or cycling fns) can reset to zero
            startSet(timerInitialMins,0,5999,1); break; //minutes
          //fnIsDayCount removed in favor of paginated calendar
          case fnIsTemp: //could do calibration here if so inclined
          case fnIsTest:
          default: break;
        }
        return;
      }
      else if((ctrl==CTRL_SEL && evt==0) || ((ctrl==CTRL_UP || ctrl==CTRL_DN) && evt==1)) { //sel release or adj press
        //we can't handle sel press here because, if attempting to enter setting mode, it would switch the fn first
        if(ctrl==CTRL_SEL){ //sel release
          if(fn==fnIsTimer && !(timerState&1)) timerClear(); //if timer is stopped, clear it
          fnScroll(1); //Go to next fn in the cycle
          fnPg = 0; //reset page counter in case we were in a paged display
          checkRTC(true); //updates display
        }
        else if(ctrl==CTRL_UP || ctrl==CTRL_DN) {
          if(fn==fnIsAlarm) switchAlarm(ctrl==CTRL_UP?1:0); //switch alarm
          if(fn==fnIsTimer){
            if(ctrl==CTRL_UP){
              if(!(timerState&1)){ //stopped
                timerStart();
              } else { //running
                #if CTRL_UPDN_TYPE==2 //rotary encoder
                  if((timerState>>1)&1) timerLap(); //chrono: lap
                  else timerRunoutToggle(); //timer: runout option
                #else //button
                  timerStop();
                #endif
              }
            } else { //CTRL_DN
              if(!(timerState&1)){ //stopped
                #if CTRL_UPDN_TYPE==1 //buttons
                  timerClear();
                  //if we wanted to reset to the previous time, we could use this; but sel hold is easy enough to get there
                  // //same as //save timer secs
                  // timerTime = (timerInitialMins*60000)+(timerInitialSecs*1000); //set timer duration
                  // if(timerTime!=0){
                  //   bitWrite(timerState,1,0); //set timer direction (bit 1) to down (0)
                  //   //timerStart(); //we won't automatically start, we'll let the user do that
                  // }
                  updateDisplay();
                #endif
              } else { //running
                #if CTRL_UPDN_TYPE==2 //rotary encoder
                  timerStop();
                #else
                  if((timerState>>1)&1) timerLap(); //chrono: lap
                  else timerRunoutToggle(); //timer: runout option
                #endif
              }
            }
          } //end if fnIsTimer
          //if(fn==fnIsTime) TODO volume in I2C radio
        }
        //else do nothing
      } //end sel release or adj press
      else if(CTRL_ALT>0 && ctrl==CTRL_ALT) { //alt sel press
        //if switched relay, and soft switch enabled, we'll switch power.
        if(ENABLE_SOFT_POWER_SWITCH && RELAY_PIN>=0 && RELAY_MODE==0) { switchPower(2); btnStop(); }
        //Otherwise, this becomes our function preset.
        else {
          //On long hold, if this is not currently the preset, we'll set it, double beep, and btnStop.
          //(Decided not to let this button set things, because then it steps on the toes of Sel's functionality.)
          if(evt==2) {
            if(readEEPROM(7,false)!=fn) {
              btnStop();
              writeEEPROM(7,fn,false);
              quickBeep(76);
              displayBlink();
            }
          }
          //On short release, jump to the preset fn.
          else if(evt==0) {
            btnStop();
            if(fn!=readEEPROM(7,false)) fn=readEEPROM(7,false);
            else {
              //Special case: if this is the alarm, toggle the alarm switch
              if(fn==fnIsAlarm) switchAlarm(2);
            }
            fnPg = 0; //reset page counter in case we were in a paged display
            updateDisplay();
          }
        }
      }
    } //end fn running

    else { //fn setting
      if(evt==1) { //press
        //TODO could we do release/shorthold on CTRL_SEL so we can exit without making changes?
        //currently no, because we don't btnStop() when short hold goes into fn setting, in case long hold may go to options menu
        //so we can't handle a release because it would immediately save if releasing from the short hold.
        //Consider recording the btn start time when going into fn setting so we can distinguish its release from a future one
        if(ctrl==CTRL_SEL) { //CTRL_SEL push: go to next option or save and exit setting mode
          btnStop(); //not waiting for CTRL_SELHold, so can stop listening here
          //We will set rtc time parts directly
          //con: potential for very rare clock rollover while setting; pro: can set date separate from time
          switch(fn){
            case fnIsTime: //save in RTC
              if(fnSetValDid){ //but only if the value was actually changed
                rtcSetTime(fnSetVal/60,fnSetVal%60,0);
                millisAtLastCheck = 0; //see ms()
                calcSun(rtcGetYear(),rtcGetMonth(),rtcGetDate());
                isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),fnSetVal/60,true);
              }
              clearSet(); break;
            case fnIsDate: //depends what page we're on
              if(fnPg==0){ //regular date display: save in RTC
                switch(fnSetPg){
                  case 1: //save year, set month
                    displayBlink(); //to indicate save. Safe b/c we've btnStopped. See below for why
                    fnSetValDate[0]=fnSetVal;
                    startSet(fnSetValDate[1],1,12,2); break; 
                  case 2: //save month, set date
                    displayBlink(); //to indicate save. Needed if set month == date: without blink, nothing changes.
                    fnSetValDate[1]=fnSetVal;
                    startSet(fnSetValDate[2],1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3); break;
                  case 3: //write year/month/date to RTC
                    rtcSetDate(fnSetValDate[0],fnSetValDate[1],fnSetVal,
                      dayOfWeek(fnSetValDate[0],fnSetValDate[1],fnSetVal));
                    calcSun(fnSetValDate[0],fnSetValDate[1],fnSetVal);
                    isDSTByHour(fnSetValDate[0],fnSetValDate[1],fnSetVal,rtcGetHour(),true);
                    clearSet(); break;
                  default: break;
                }
              } else if(fnPg==fnDateCounter){ //set like date, save in eeprom like finishOpt
                switch(fnSetPg){
                  case 1: //save month, set date
                    displayBlink(); //to indicate save.
                    //Needed if set month == date: without blink, nothing changes. Also just good feedback.
                    writeEEPROM(5,fnSetVal,false);
                    startSet(readEEPROM(6,false),1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),2); break;
                  case 2: //save date, set direction
                    displayBlink(); //to indicate save.
                    writeEEPROM(6,fnSetVal,false);
                    startSet(readEEPROM(4,false),0,1,3); break;
                  case 3: //save date
                    writeEEPROM(4,fnSetVal,false);
                    clearSet(); break;
                  default: break;
                }
              } else if(fnPg==fnDateSunlast || fnPg==fnDateSunnext){ //lat and long
                //TODO
              } else if(fnPg==fnDateWeathernow || fnPg==fnDateWeathernext){ //temperature units??
                //TODO
              }
              break;
            case fnIsAlarm:
              writeEEPROM(0,fnSetVal,true);
              clearSet(); break;
            case fnIsTimer: //timer - depends what page we're on
              switch(fnSetPg){
                case 1: //save timer mins, set timer secs
                  displayBlink(); //to indicate save.
                  timerInitialMins = fnSetVal; //minutes, up to 5999 (99m 59s)
                  startSet(timerInitialSecs,0,59,2); break;
                case 2: //save timer secs
                  displayBlink(); //to indicate save.
                  timerInitialSecs = fnSetVal;
                  timerTime = (timerInitialMins*60000)+(timerInitialSecs*1000); //set timer duration
                  if(timerTime!=0){
                    bitWrite(timerState,1,0); //set timer direction (bit 1) to down (0)
                    //timerStart(); //we won't automatically start, we'll let the user do that
                    //TODO: in timer radio mode, skip setting setting the seconds (display placeholder) and start when done. May even want to skip runout options even if the beeper is there. Or could make it an option in the settings file.
                  }
                  clearSet(); break;
                default: break;
              }
              break;
            //fnIsDayCount removed in favor of paginated calendar
            case fnIsTemp:
              break;
            default: break;
          } //end switch fn
        } //end CTRL_SEL push
        if(ctrl==CTRL_UP) doSet(rotVel ? 10 : 1);
        if(ctrl==CTRL_DN) doSet(rotVel ? -10 : -1);
      } //end if evt==1
      else if(evt==2){ //short hold - trigger doSetHold directly for better timing
        #if CTRL_UPDN_TYPE==1 //buttons
          if(ctrl==CTRL_UP || ctrl==CTRL_DN) doSetHold(true);
        #endif
      }
    } //end fn setting
    
  } //end normal fn running/setting
  
  else { //options menu setting - to/from EEPROM
    
    byte opt = fn-fnOpts; //current option index
    
    if(evt==2 && ctrl==CTRL_SEL) { //CTRL_SEL short hold: exit options menu
      btnStop();
      //if we were setting a value, writes setting val to EEPROM if needed
      if(fnSetPg) writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]>255?true:false);
      fn = fnIsTime;
      //we may have changed lat/long/GMT/DST settings so recalc those
      calcSun(rtcGetYear(),rtcGetMonth(),rtcGetDate()); //TODO pull from clock
      isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),rtcGetHour(),true);
      clearSet();
      return;
    }
    
    if(!fnSetPg){ //viewing option number
      if(ctrl==CTRL_SEL && evt==0) { //CTRL_SEL release: enter option value setting
        startSet(readEEPROM(optsLoc[opt],optsMax[opt]>255?true:false),optsMin[opt],optsMax[opt],1);
      }
      if(ctrl==CTRL_UP && evt==1) fnOptScroll(1); //next one up or cycle to beginning
      if(ctrl==CTRL_DN && evt==1) fnOptScroll(0); //next one down or cycle to end?
      updateDisplay();
    } //end viewing option number

    else { //setting option value
      if(ctrl==CTRL_SEL && evt==0) { //CTRL_SEL release: save and exit option value setting
        //Writes setting val to EEPROM if needed
        writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]>255?true:false);
        clearSet();
      }
      if(evt==1 && (ctrl==CTRL_UP || ctrl==CTRL_DN)){
        if(ctrl==CTRL_UP) doSet(rotVel ? 10 : 1);
        if(ctrl==CTRL_DN) doSet(rotVel ? -10 : -1);
        updateDisplay(); //may also make sounds for sampling
      }
    }  //end setting option value
  } //end options menu setting
  
} //end ctrlEvt

void fnScroll(byte dir){
  //0=down, 1=up
  //Switch to the next (up) or previous (down) enabled function. This determines the order.
  //We'll use switch blocks *without* breaks to cascade to the next enabled function
  if(dir) { // up
    switch(fn) {
      case fnIsTime: if(ENABLE_DATE_FN) { fn = fnIsDate; break; }
      case fnIsDate: if(ENABLE_ALARM_FN) { fn = fnIsAlarm; break; }
      case fnIsAlarm: if(ENABLE_TIMER_FN) { fn = fnIsTimer; break; }
      case fnIsTimer: if(ENABLE_TEMP_FN) { fn = fnIsTemp; break; }
      case fnIsTemp: if(ENABLE_TUBETEST_FN) { fn = fnIsTest; break; }
      case fnIsTest: default: fn = fnIsTime; break;
    }
  } else { // down
    switch(fn) {
      case fnIsTime: if(ENABLE_TUBETEST_FN) { fn = fnIsTest; break; } 
      case fnIsTest: if(ENABLE_TEMP_FN) { fn = fnIsTemp; break; }
      case fnIsTemp: if(ENABLE_TIMER_FN) { fn = fnIsTimer; break; }
      case fnIsTimer: if(ENABLE_ALARM_FN) { fn = fnIsAlarm; break; }
      case fnIsAlarm: if(ENABLE_DATE_FN) { fn = fnIsDate; break; }
      case fnIsDate: default: fn = fnIsTime; break;
    }
  }
}
void fnOptScroll(byte dir){
  //0=down, 1=up
  //Switch to the next options fn between min and max (inclusive), looping around at range ends
  byte posLast = fnOpts+sizeof(optsLoc)-1;
  if(dir==1) fn = (fn==posLast? fnOpts: fn+1);
  if(dir==0) fn = (fn==fnOpts? posLast: fn-1);
  //Certain options don't apply to some configurations; skip those.
  byte optLoc = optsLoc[fn-fnOpts];
  if(
      //Hardware config
      (PIEZO_PIN<0 && (optLoc==39||optLoc==40||optLoc==41||optLoc==47||optLoc==48||optLoc==49)) //no piezo: no signal pitches or alarm/timer/strike beeper pattern
      || ((PIEZO_PIN<0 && RELAY_MODE==0) && (optLoc==21||optLoc==50)) //no piezo, and relay is switch: no strike, or alarm fibonacci mode
      || ((RELAY_PIN<0 || PIEZO_PIN<0) && (optLoc==42||optLoc==43||optLoc==44)) //no relay or no piezo: no alarm/timer/strike signal
      || ((RELAY_MODE==0) && (optLoc==44)) //relay is switch: no strike signal
      || ((LED_PIN<0) && (optLoc==26)) //no led pin: no led control
      || ((LED_PIN<0) && (optLoc==26)) //no led pin: no led control
      //Functions disabled
      || (!ENABLE_DATE_FN && (optLoc==17||optLoc==18||optLoc==10||optLoc==12||optLoc==14)) //date fn disabled in config: skip date and geography options
      || (!ENABLE_ALARM_FN && (optLoc==23||optLoc==42||optLoc==39||optLoc==47||optLoc==24||optLoc==50)) //alarm fn disabled in config: skip alarm options
      || (!ENABLE_TIMER_FN && (optLoc==43||optLoc==40||optLoc==48)) //timer fn disabled in config: skip timer options
      || (!ENABLE_TEMP_FN && (optLoc==45)) //temp fn disabled in config: skip temp format TODO good for weather also
      //Other functionality disabled
      || (!ENABLE_DATE_RISESET && (optLoc==10||optLoc==12||optLoc==14)) //date rise/set disabled in config: skip geography
      || (!ENABLE_ALARM_AUTOSKIP && (optLoc==23)) //alarm autoskip disabled in config: skip autoskip switch
      || (!ENABLE_ALARM_FIBONACCI && (optLoc==50)) //fibonacci mode disabled in config: skip fibonacci switch
      || (!ENABLE_TIME_CHIME && (optLoc==21||optLoc==44||optLoc==41||optLoc==49)) //chime disabled in config: skip chime
      || (!ENABLE_SHUTOFF_NIGHT && (optLoc==27||optLoc==28||optLoc==30)) //night shutoff disabled in config: skip night
      || ((!ENABLE_SHUTOFF_NIGHT || !ENABLE_SHUTOFF_AWAY) && (optLoc==32||optLoc==35||optLoc==37)) //night or away shutoff disabled in config: skip away (except workweek)
      || ((!ENABLE_SHUTOFF_NIGHT || !ENABLE_SHUTOFF_AWAY) && (!ENABLE_ALARM_AUTOSKIP || !ENABLE_ALARM_FN) && (optLoc==33||optLoc==34)) //(night or away) and alarm autoskip disabled: skip workweek
    ) {
    fnOptScroll(dir);
  }
}

void switchAlarm(byte dir){
  //0=down, 1=up, 2=toggle
  if(ENABLE_SOFT_ALARM_SWITCH){
    //signalStop(); //snoozeRemain = 0; //TODO I don't think we need this anymore – test
    //There are three alarm states - on, on with skip (skips the next alarm trigger), and off.
    //Currently we use up/down buttons or a rotary control, rather than a binary switch, so we can cycle up/down through these states.
    //On/off is stored in EEPROM to survive power loss; skip is volatile, not least because it can change automatically and I don't like making automated writes to EEPROM if I can help it. Skip state doesn't matter when alarm is off.
    if(dir==2) dir=(readEEPROM(2,false)?0:1); //If alarm is off, cycle button goes up; otherwise down.
    if(dir==1){
      if(!readEEPROM(2,false)){ //if off, go straight to on, no skip
        writeEEPROM(2,1,false); alarmSkip=0; quickBeep(76); //C7
      }
      else if(alarmSkip){ //else if skip, go to on
        alarmSkip=0; quickBeep(76); //C7
      }
    }
    if(dir==0){
      if(readEEPROM(2,false)){ //if on
        if(!alarmSkip){ //if not skip, go to skip
          alarmSkip=1; quickBeep(71); //G6
        }
        else { //if skip, go to off
          writeEEPROM(2,0,false); quickBeep(64); //C6
        }
      }
    }
    updateDisplay();
  }
  //TODO don't make alarm permanent until leaving setting to minimize writes to eeprom as user cycles through options?
}
void switchPower(byte dir){
  //0=down, 1=up, 2=toggle
  signalRemain = 0; snoozeRemain = 0; //in case alarm is going now - alternatively use signalStop()?
  //If the timer is running down and is using the switched relay, this instruction conflicts with it, so cancel it
  if(timerState&1 && !((timerState>>1)&1) && readEEPROM(43,false)==1) {
    timerClear();
    updateDisplay();
    return;
  }
  //RELAY_PIN state is the reverse of the appliance state: LOW = device on, HIGH = device off
  // Serial.print(millis(),DEC);
  // Serial.print(F(" Relay requested to "));
  if(dir==2) { //toggle
    dir = (digitalRead(RELAY_PIN)?1:0); //LOW = device on, so this effectively does our dir reversion for us
    //Serial.print(dir==1?F("toggle on"):F("toggle off"));
  } else {
    //Serial.print(dir==1?F("switch on"):F("switch off"));
  }
  digitalWrite(RELAY_PIN,(dir==1?0:1)); updateLEDs(); //LOW = device on
  //Serial.println(F(", switchPower"));
}

void startSet(int n, int m, int x, byte p){ //Enter set state at page p, and start setting a value
  fnSetVal=n; fnSetValMin=m; fnSetValMax=x; fnSetValVel=(x-m>30?1:0); fnSetPg=p; fnSetValDid=false;
  updateDisplay();
}
void doSet(int delta){
  //Does actual setting of fnSetVal, as told to by ctrlEvt or doSetHold. Makes sure it stays within range.
  if(delta>0) if(fnSetValMax-fnSetVal<delta) fnSetVal-=((fnSetValMax-fnSetValMin)+1-delta); else fnSetVal=fnSetVal+delta;
  if(delta<0) if(fnSetVal-fnSetValMin<abs(delta)) fnSetVal+=((fnSetValMax-fnSetValMin)+1+delta); else fnSetVal=fnSetVal+delta;
  fnSetValDid=true;
  updateDisplay();
}
#if CTRL_UPDN_TYPE==1 //buttons
unsigned long doSetHoldLast;
void doSetHold(bool start){
  //When we're setting via an adj button that's passed a hold point, fire off doSet commands at intervals
  //TODO integrate this with checkInputs?
  unsigned long now = millis();
  //The interval used to be 250, but in order to make the value change by a full 9 values between CTRL_HOLD_SHORT_DUR and CTRL_HOLD_LONG_DUR,
  //the interval is now that difference divided by 9. TODO divisor may need to be a bit higher in case CTRL_HOLD_LONG_DUR ever fires before 9th - it seems indeed it did, so 9.5.
  //It may be weird not being exactly quarter-seconds, but it doesn't line up with the blinking anyway.
  if(start || (unsigned long)(now-doSetHoldLast)>=(((CTRL_HOLD_LONG_DUR-CTRL_HOLD_SHORT_DUR)*2)/19)) { //(x*2)/19 = x/9.5
    doSetHoldLast = now;
    if(fnSetPg!=0 && (btnCur==CTRL_UP || btnCur==CTRL_DN) ){ //if we're setting, and this is an adj btn
      bool dir = (btnCur==CTRL_UP ? 1 : 0);
      //If short hold, or long hold but high velocity isn't supported, use low velocity (delta=1)
      if(btnCurHeld==2 || (btnCurHeld==3 && fnSetValVel==false)) doSet(dir?1:-1);
      //else if long hold, use high velocity (delta=10)
      else if(btnCurHeld==3) doSet(dir?10:-10);
    }
  }
}
#endif
void clearSet(){ //Exit set state
  startSet(0,0,0,0);
  fnSetValDid=false;
  updateLEDs(); //in case LED setting was changed
  checkRTC(true); //force an update to tod and updateDisplay()
}

//EEPROM values are exclusively bytes (0-255) or words (unsigned ints, 0-65535)
//If it's a word, high byte is in loc, low byte is in loc+1
void initEEPROM(bool hard){
  //If hard, set EEPROM and clock to defaults
  //Otherwise, just make sure stuff is in range
  //First prevent the held button from doing anything else
  btnCur = CTRL_SEL; btnStop();
  //If a hard init, set the clock
  if(hard) {
    rtcSetDate(2021,1,1,dayOfWeek(2021,1,1));
    rtcSetTime(0,0,0);
  }
  //The vars outside the options menu
  if(hard || readEEPROM(0,true)>1439) writeEEPROM(0,420,true); //0-1: alarm at 7am
  //2: alarm on, handled by init
  //3: free
  if(hard || readEEPROM(4,false)<0 || readEEPROM(4,false)>1) writeEEPROM(4,1,false); //4: day counter direction: count up...
  if(hard || readEEPROM(5,false)<1 || readEEPROM(5,false)>12) writeEEPROM(5,12,false); //5: ...December...
  if(hard || readEEPROM(6,false)<1 || readEEPROM(6,false)>31) writeEEPROM(6,31,false); //6: ...31st. (This gives the day of the year)
  if(hard) writeEEPROM(7,0,false); //7: Alt function preset
  //8: TODO functions/pages enabled (bitmask)
  //9: free
  if(hard) writeEEPROM(15,0,false); //15: last known DST on flag - clear on hard reset (to match the reset RTC/auto DST/anti-poisoning settings to trigger midnight tubes as a tube test)
  //The vars inside the options menu
  bool isInt = false;
  for(byte opt=0; opt<sizeof(optsLoc); opt++) {
    isInt = (optsMax[opt]>255?true:false);
    if(hard || readEEPROM(optsLoc[opt],isInt)<optsMin[opt] || readEEPROM(optsLoc[opt],isInt)>optsMax[opt])
      writeEEPROM(optsLoc[opt],optsDef[opt],isInt);
  } //end for
} //end initEEPROM()

void findFnAndPageNumbers(){
  //Each function, and each page in a paged function, has a number. //TODO should pull from EEPROM 8
  fnDatePages = 1; //date function always has a page for the date itself
  if(ENABLE_DATE_COUNTER){ fnDatePages++; fnDateCounter=fnDatePages-1; }
  if(ENABLE_DATE_RISESET){ fnDatePages++; fnDateSunlast=fnDatePages-1; }
  if(false){ fnDatePages++; fnDateWeathernow=fnDatePages-1; }
  if(ENABLE_DATE_RISESET){ fnDatePages++; fnDateSunnext=fnDatePages-1; }
  if(false){ fnDatePages++; fnDateWeathernext=fnDatePages-1; }
}


////////// Timing and timed events //////////

byte rtcSecLast = 61;
byte rtcDid = 0; //bitmask to indicate whether we just did things that would be bad to overdo, in case checkRTC is called multiple times during the trigger period (such as if fn is changed during second :00). bit 0 = timer drift correction. At this time other things are OK to overdo since they just restart the same action (alarm/chime signal triggering, date/anti-poisoning display routines) or have other means of canceling (DST change, per DST flag). TODO could get away from this if we move the force stuff into another method?
void checkRTC(bool force){
  //Checks display timeouts;
  //checks for new time-of-day second -> decrements timeouts and checks for timed events;
  //updates display for "running" functions.
  unsigned long now = millis();
  
  //Things to do every time this is called: timeouts to reset display. These may force a tick.
  //Option/setting timeout: if we're in the options menu, or we're setting a value
  if(fnSetPg || fn>=fnOpts){
    if((unsigned long)(now-inputLast)>=SETTING_TIMEOUT*1000) { fnSetPg = 0; fn = fnIsTime; force=true; } //Time out after 2 mins
  }
  //Paged-display function timeout //TODO change fnIsDate to consts? //TODO timeoutPageFn var
  else if(fn==fnIsDate && (unsigned long)(now-inputLast)>=FN_PAGE_TIMEOUT*1000) { //3sec per date page
    // //If a scroll in is going, fast-forward to end - see also ctrlEvt
    // if(scrollRemain>0) {
    //   scrollRemain = 1;
    //   checkEffects(true);
    // }
    //Here we just have to increment the page and decide when to reset. updateDisplay() will do the rendering
    fnPg++; inputLast+=(FN_PAGE_TIMEOUT*1000); //but leave inputLastTODMins alone so the subsequent page displays will be based on the same TOD
    while(fnPg<fnDatePages && fnPg<200 && ( //skip inapplicable date pages. The 200 is an extra failsafe
        (!readEEPROM(10,true) && !readEEPROM(12,true) && //if no lat+long specified, skip weather/rise/set
          (fnPg==fnDateWeathernow || fnPg==fnDateWeathernext || fnPg==fnDateSunlast || fnPg==fnDateSunnext))
      )) fnPg++;
    if(fnPg >= fnDatePages){ fnPg = 0; fn = fnIsTime; } // when we run out of pages, go back to time. When the half-minute date is triggered, fnPg is set to 254, so it will be 255 here and be cancelled after just the one page.
    force=true;
  }
  //Temporary-display function timeout: if we're *not* in a permanent one (time, or running/signaling timer)
  // Stopped/non-signaling timer shouldn't be permanent, but have a much longer timeout, mostly in case someone is waiting to start the chrono in sync with some event, so we'll give that an hour.
  else if(fn!=fnIsTime && !(fn==fnIsTimer && (timerState&1 || signalRemain>0))){
    if((unsigned long)(now-inputLast)>=(fn==fnIsTimer?3600:FN_TEMP_TIMEOUT)*1000) { fnSetPg = 0; fn = fnIsTime; force=true; }
  }
  
  //Update things based on RTC
  rtcTakeSnap();
  
  if(rtcSecLast != rtcGetSecond() || force) { //If it's a new RTC second, or we are forcing it
    
    //First run things
    if(rtcSecLast==61) { autoDST(); calcSun(rtcGetYear(),rtcGetMonth(),rtcGetDate()); }
    
    //Things to do every natural second (decrementing real-time counters)
    if(rtcSecLast != rtcGetSecond()) {
      //If alarm snooze has time on it, decrement, and if we reach zero and alarm is still on, resume
      //Won't check alarm skip status here, as it reflects tomorrow
      if(snoozeRemain>0) {
        snoozeRemain--;
        //Serial.print("sr "); Serial.println(snoozeRemain,DEC);
        if(snoozeRemain<=0 && readEEPROM(2,false)) { //alarm on
          fnSetPg = 0; fn = fnIsTime;
          if((readEEPROM(50,false) && !(RELAY_PIN>=0 && RELAY_MODE==0 && readEEPROM(42,false)==1))) fibonacci(rtcGetHour(),rtcGetMinute(),rtcGetSecond()); //fibonacci sequence
          else signalStart(fnIsAlarm,1); //regular alarm
        }
      }
      if(unoffRemain>0) {
        unoffRemain--; //updateDisplay will naturally put it back to off state if applicable
      }
      if(versionRemain>0) {
        versionRemain--;
      }
    } //end natural second
  
    //Things to do at specific times
    //Timer drift correction: per the millisCorrectionInterval
    if(rtcGetSecond()%millisCorrectionInterval==0){ //if time:
      if(!(rtcDid&1)) millisCheckDrift(); bitWrite(rtcDid,0,1); //do if not done, set as done
    } else bitWrite(rtcDid,0,0); //if not time: set as not done
    //DST change check: every 2am
    if(rtcGetSecond()==0 && rtcGetMinute()==0 && rtcGetHour()==2) autoDST();
    //Alarm check: at top of minute for normal alarm, or 23 seconds past for fibonacci (which starts 26m37s early)
    //Only do fibonacci if enabled and if the alarm is not using the switched relay - otherwise do regular
    bool fibOK = readEEPROM(50,false) && !(RELAY_PIN>=0 && RELAY_MODE==0 && readEEPROM(42,false)==1);
    if((rtcGetSecond()==0 && !fibOK) || (rtcGetSecond()==23 && fibOK)){
      int alarmTime = readEEPROM(0,true);
      if(rtcGetSecond()==23){ alarmTime-=27; if(alarmTime<0) alarmTime+=1440; } //set min to n-27 with midnight rollover
      if(rtcGetHour()*60+rtcGetMinute()==alarmTime){
        //Serial.println(rtcGetSecond()==23?F("It's fibonacci time"):F("It's regular alarm time"));
        if(readEEPROM(2,false) && !alarmSkip) { //if the alarm is on and not skipped, sound it!
          fnSetPg = 0; fn = fnIsTime;
          if(rtcGetSecond()==23) fibonacci(rtcGetHour(),rtcGetMinute(),rtcGetSecond()); //fibonacci sequence
          else signalStart(fnIsAlarm,1); //regular alarm
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
      } //end alarm trigger
    }
    //At bottom of minute, see if we should show the date
    if(rtcGetSecond()==30 && fn==fnIsTime && fnSetPg==0 && unoffRemain==0 && versionRemain==0) { /*cleanRemain==0 && scrollRemain==0 && */ 
      if(readEEPROM(18,false)>=2) { fn = fnIsDate; inputLast = now; inputLastTODMins = rtcGetHour()*60+rtcGetMinute(); fnPg = 254; updateDisplay(); }
      //if(readEEPROM(18,false)==3) { startScroll(); }
    }
    //Anti-poisoning routine triggering: start when applicable, and not at night, during setting, or after a button press (unoff)
    if(rtcGetSecond()<2 && displayDim==2 && fnSetPg==0 && unoffRemain==0) {
      //temporarily we'll recalculate the sun stuff every day
      if(readEEPROM(27,false)>0? //is night shutoff enabled?
        rtcGetSecond()==0 && rtcGetHour()*60+rtcGetMinute()==readEEPROM(28,true): //if so, at start of night shutoff (at second :00 before dim is in effect)
        rtcGetSecond()==1 && rtcGetHour()*60+rtcGetMinute()==0) //if not, at 00:00:01
          calcSun(rtcGetYear(),rtcGetMonth(),rtcGetDate()); //take this opportunity to perform a calculation that blanks the display for a bit
      // TODO the below will need to change cleanRemain=x to displayClean(x)
      
      // switch(readEEPROM(46,false)) { //how often should the routine run?
      //   case 0: //every day
      //     if(readEEPROM(27,false)>0? //is night shutoff enabled?
      //       rtcGetSecond()==0 && rtcGetHour()*60+rtcGetMinute()==readEEPROM(28,true): //if so, at start of night shutoff (at second :00 before dim is in effect)
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
    
    //Strikes - only if fn=clock, not setting, not signaling/snoozing, not night/away. Setting 21 will be off if signal type is no good
    //The six pips
    if(rtcGetMinute()==59 && rtcGetSecond()==55 && readEEPROM(21,false)==2 && signalRemain==0 && snoozeRemain==0 && fn==fnIsTime && fnSetPg==0 && displayDim==2) {
      signalStart(fnIsTime,6); //the signal code knows to use pip durations as applicable
    }
    //Strikes on/after the hour
    if(rtcGetSecond()==0 && (rtcGetMinute()==0 || rtcGetMinute()==30) && signalRemain==0 && snoozeRemain==0 && fn==fnIsTime && fnSetPg==0 && displayDim==2){
      byte hr; hr = rtcGetHour(); hr = (hr==0?12:(hr>12?hr-12:hr));
      switch(readEEPROM(21,false)) {
        case 1: //single beep
          if(rtcGetMinute()==0) signalStart(fnIsTime,0); break;
        case 3: //hour strike via normal signal cycle
          if(rtcGetMinute()==0) signalStart(fnIsTime,hr); break;
        case 4: //ship's bell at :00 and :30 mins via normal signal cycle
          signalStart(fnIsTime,((hr%4)*2)+(rtcGetMinute()==30?1:0)); break;
        default: break;
      } //end strike type
    } //end strike
    
    //Finally, update the display, whether natural tick or not, as long as we're not setting or on a scrolled display (unless forced eg. fn change)
    //This also determines night/away shutoff, which is why strikes will happen if we go into off at top of hour, and not when we come into on at the top of the hour TODO find a way to fix this
    //Also skip updating the display if this is date and not being forced, since its pages take some calculating that cause it to flicker
    if(fnSetPg==0 && (true || force) && !(fn==fnIsDate && !force)) updateDisplay(); /*scrollRemain==0 ||*/
    
    rtcSecLast = rtcGetSecond();
    
  } //end if force or new second
} //end checkRTC()

void fibonacci(byte h, byte m, byte s){
  //This powers the alarm fibonacci feature, using snooze and quick beeps.
  //Find difference between alarm time and current time, in minutes, with midnight rollover
  int diff = readEEPROM(0,true)-(h*60+m); if(diff<0) diff+=1440;
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
          signalSource = fnIsAlarm;
          signalStart(-1,0); //Play a signal measure using above pattern and source
          snoozeRemain = nnn;
          //Serial.print(F(" SR")); Serial.print(snoozeRemain,DEC);
        } else { //Time for regular alarm
          //Serial.print(F(" Alarm!"));
          signalStart(fnIsAlarm,1);
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
//daysInMonth() moved to top
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
unsigned long ms(){
  // Returns millis() with the drift offset applied, for timer/chrono purposes.
  // WARNING: Since the offset is being periodically adjusted, there is the possibility of a discontinuity in ms() output – if we give out a timestamp and then effectively set the clock back, the next timestamp might possibly be earlier than the last one, which could mess up duration math. I tried to think of a way to monitor for that discontinuity – e.g. if now-then is greater than then-now, due to overflow – but it gets tricky since millis() is effectively circular, and that condition occurs naturally at rollover as well – so I think we would need a flag that millisCheckDrift sets when it sets the offset backward, and ms clears when the real time has caught up.... or something like that.
  // So for now we'll restrict use of ms() to the timer/chrono duration – the only place we really need it, and fortunately it's not a problem here, because that duration never exceeds 100 hours so we can easily detect that overflow. And the only time it might really be an issue is if it occurs immediately after the chrono starts, since it would briefly display the overflowed time – in that case we'll just reset the timer to 0 and keep going.
  return (unsigned long)(millis()+millisDriftOffset);
}
void timerStart(){
  bitWrite(timerState,0,1); //set timer running (bit 0) to on (1)
  if(timerTime==0) bitWrite(timerState,1,1); //set timer direction (bit 1) to up (1) if we were idle
  //When the timer is stopped, timerTime holds a duration, independent of any start/stop time.
  //Convert it to a timestamp:
  //If chrono (count up), timestamp is an origin in the past: now minus duration.
  //If timer (count down), timestamp is a destination in the future: now plus duration.
  timerTime = ((timerState>>1)&1? ms() - timerTime: ms() + timerTime);
  if(!((timerState>>1)&1)) timerSwitchSleepRelay(1); //possibly switch the relay, but only if counting down
  quickBeep(69);
} //end timerStart()
void timerStop(){
  bitWrite(timerState,0,0); //set timer running (bit 0) to off (0)
  //When the timer is running, timerTime holds a timestamp, which the current duration is continuously calculated from.
  //Convert it to a duration:
  //If chrono (count up), timestamp is an origin in the past: duration is now minus timestamp.
  //If timer (count down), timestamp is a destination in the future: duration is timestamp minus now.
  timerTime = ((timerState>>1)&1? ms() - timerTime: timerTime - ms());
  if(!((timerState>>1)&1)) timerSwitchSleepRelay(0); //possibly switch the relay, but only if counting down
  quickBeep(64);
  bitWrite(timerState,4,0); //set timer lap display (bit 4) to off (0)
  updateDisplay(); //since cycleTimer won't do it
}
void timerClear(){
  bitWrite(timerState,0,0); //set timer running (bit 0) to off (0)
  bitWrite(timerState,1,1); //set timer direction (bit 1) to up (1) TODO is this necessary
  timerTime = 0; //set timer duration
  timerSwitchSleepRelay(0);
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
    if(!((timerState>>2)&1)) timerState ^= (1<<3); //if it's 0, toggle runout chrono bit
    //do a quick signal to indicate the selection
    signalPattern = ((timerState>>2)&3)+1; //convert 00/01/10/11 to 1/2/3/4
    signalSource = fnIsTimer;
    signalStart(-1,0); //Play a signal measure using above pattern and source
  }
}
void cycleTimer(){
  if(timerState&1){ //If the timer is running
    //Check if we've hit a wall
    if(!((timerState>>1)&1)){ //If we are counting down,
      if((unsigned long)(ms()-timerTime)<1000){ //see if now is past timerTime (diff has rolled over)
        //timer has run out
        //runout action and display
        if((timerState>>3)&1){ //runout chrono - keep target, change direction, kill sleep, change display
          bitWrite(timerState,1,1); //set timer direction (bit 1) to up (1)
          timerSwitchSleepRelay(0);
          fnSetPg = 0; fn = fnIsTimer;
        } else {
          if((timerState>>2)&1){ //runout repeat - keep direction, change target, keep sleep, don't change display
            timerTime += (timerInitialMins*60000)+(timerInitialSecs*1000); //set timer duration ahead by initial setting
          } else { //runout clear - clear timer, change display
            timerClear();
            //If switched relay (radio sleep), go to time of day; otherwise go to empty timer to appear with signal
            fnSetPg = 0; fn = (RELAY_PIN>=0 && RELAY_MODE==0 && readEEPROM(43,false)==1 ? fnIsTime: fnIsTimer);
            updateDisplay();
          }
        }
        //signal (if not switched relay)
        if((timerState>>2)&1){ //short signal (piggybacks on runout repeat flag)
          if(RELAY_PIN<0 || RELAY_MODE!=0 || readEEPROM(43,false)!=1) signalStart(fnIsTimer,1);
          //using 1 instead of 0, because in signalStart, fnIsTimer "quick measure" has a custom pitch for runout option setting
        } else { //long signal
          if(RELAY_PIN<0 || RELAY_MODE!=0 || readEEPROM(43,false)!=1) signalStart(fnIsTimer,SIGNAL_DUR);
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
    if(fn==fnIsTimer) updateDisplay();
  }
} //end cycleTimer()
void timerSwitchSleepRelay(bool on){
  //When timer is set to use switched relay, it's on while timer is running, "radio sleep" style.
  //This doesn't count as a signal (using signal methods) so that other signals might not interrupt it. TODO confirm
  if(RELAY_PIN>=0 && RELAY_MODE==0 && readEEPROM(43,false)==1) { //start "radio sleep"
    digitalWrite(RELAY_PIN,(on?LOW:HIGH)); updateLEDs(); //LOW = device on
    // Serial.print(millis(),DEC);
    // if(on) Serial.println(F(" Relay on, timerSwitchSleepRelay (radio sleep)"));
    // else   Serial.println(F(" Relay off, timerSwitchSleepRelay (radio sleep)"));
  }
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
  //   displayDim = 2;
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
  if(versionRemain>0) {
    editDisplay(vMajor, 0, 1, false, false);
    editDisplay(vMinor, 2, 3, false, false);
    editDisplay(vPatch, 4, 5, false, false);
  }
  else if(fnSetPg) { //setting value, for either fn or option
    displayDim = 2;
    blankDisplay(4, 5, false);
    byte fnOptCurLoc = (fn>=fnOpts? optsLoc[fn-fnOpts]: 0); //current option index loc, to tell what's being set
    if(fnSetValMax==1439) { //Time of day (0-1439 mins, 0:00–23:59): show hrs/mins
      editDisplay(fnSetVal/60, 0, 1, readEEPROM(19,false), false); //hours with leading zero per options
      editDisplay(fnSetVal%60, 2, 3, true, false);
    } else if(fnSetValMax==5999) { //Timer duration mins (0-5999 mins, up to 99:59): show hrs/mins w/regular leading
      editDisplay(fnSetVal/60, 0, 1, readEEPROM(19,false), false); //hours with leading zero per options
      editDisplay(fnSetVal%60, 2, 3, true, false); //minutes with leading zero always
    } else if(fnSetValMax==59) { //Timer duration secs: show with leading
      //If 6 tubes (0-5), display on 4-5
      //If 4 tubes (0-3), dislpay on 2-3
      blankDisplay(0, 3, false);
      editDisplay(fnSetVal, (DISPLAY_SIZE>4? 4: 2), (DISPLAY_SIZE>4? 5: 3), true, false);
    } else if(fnSetValMax==88) { //A piezo pitch. Play a short demo beep.
      editDisplay(fnSetVal, 0, 3, false, false);
      if(PIEZO_PIN>=0) { noTone(PIEZO_PIN); tone(PIEZO_PIN, getHz(fnSetVal), 100); } //Can't use signalStart since we need to specify pitch directly
    } else if(fnOptCurLoc==47 || fnOptCurLoc==48 || fnOptCurLoc==49) { //Signal pattern. Play a demo measure.
      editDisplay(fnSetVal, 0, 3, false, false);
      signalPattern = fnSetVal;
      signalSource = (fnOptCurLoc==49?fnIsTime:(fnOptCurLoc==48?fnIsTimer:fnIsAlarm));
      signalStart(-1,1); //Play a sample using the above source and pattern
    } else if(fnSetValMax==156) { //Timezone offset from UTC in quarter hours plus 100 (since we're not set up to support signed bytes)
      editDisplay((abs(fnSetVal-100)*25)/100, 0, 1, fnSetVal<100, false); //hours, leading zero for negatives
      editDisplay((abs(fnSetVal-100)%4)*15, 2, 3, true, false); //minutes, leading zero always
    } else if(fnSetValMax==900 || fnSetValMax==1800) { //Lat/long in tenths of a degree
      //If 6 tubes (0-5), display degrees on 0-3 and tenths on 4, with 5 blank
      //If 4 tubes (0-3), display degrees on 0-2 and tenths on 3
      editDisplay(abs(fnSetVal), 0, (DISPLAY_SIZE>4? 4: 3), fnSetVal<0, false);
    } else editDisplay(abs(fnSetVal), 0, 3, fnSetVal<0, false); //some other type of value - leading zeros for negatives
  }
  else if(fn >= fnOpts){ //options menu, but not setting a value
    displayDim = 2;
    editDisplay(optsNum[fn-fnOpts],0,1,false,false); //display option number on hour tubes
    blankDisplay(2,5,false);
  }
  else { //fn running
    
    //Set displayDim per night/away settings - fnIsAlarm may override this
    //issue: moving from off alarm to next fn briefly shows alarm in full brightness. I think because of the display delays. TODO
    word todmins = rtcGetHour()*60+rtcGetMinute();
    //In order of precedence: //TODO can we fade between dim states? 
    //clock at work: away on weekends, all day
    if( readEEPROM(32,false)==1 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),rtcGetWeekday()) )
      displayDim = (unoffRemain>0? 2: 0); //unoff overrides this
    //clock at home: away on weekdays, during office hours only
    else if( readEEPROM(32,false)==2 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),rtcGetWeekday()) && isTimeInRange(readEEPROM(35,true), readEEPROM(37,true), todmins) ) displayDim = (unoffRemain>0? 2: 0);
    //night shutoff - if night end is 0:00, use alarm time instead
    else if( readEEPROM(27,false) && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins) ) displayDim = (readEEPROM(27,false)==1?1:(unoffRemain>0?2:0)); //dim or (unoff? bright: off)
    //normal
    else displayDim = 2;
    updateLEDs();
    
    switch(fn){
      case fnIsTime:
        byte hr; hr = rtcGetHour();
        if(readEEPROM(16,false)==1) hr = (hr==0?12:(hr>12?hr-12:hr));
        editDisplay(hr, 0, 1, readEEPROM(19,false), true);
        editDisplay(rtcGetMinute(), 2, 3, true, true);
        if(readEEPROM(18,false)==1) editDisplay(rtcGetDate(), 4, 5, readEEPROM(19,false), true); //date
        else editDisplay(rtcGetSecond(), 4, 5, true, true); //seconds
        break;
      case fnIsDate: //a paged display
        if(fnPg==0 || fnPg==254){ //plain ol' date - 0 will continue to other pages, 254 will only display date then return to time (e.g. at half minute)
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
        }
        else if(fnPg==fnDateCounter){
          editDisplay(dateComp(rtcGetYear(),rtcGetMonth(),rtcGetDate(),readEEPROM(5,false),readEEPROM(6,false),readEEPROM(4,false)),0,3,false,true);
          blankDisplay(4,5,true);
        }
        //The sun and weather displays are based on a snapshot of the time of day when the function display was triggered, just in case it's triggered a few seconds before a sun event (sunrise/sunset) and the "prev/now" and "next" displays fall on either side of that event, they'll both display data from before it. If triggered just before midnight, the date could change as well – not such an issue for sun, but might be for weather - TODO create date snapshot also
        else if(fnPg==fnDateSunlast) displaySun(0,rtcGetDate(),inputLastTODMins);
        else if(fnPg==fnDateWeathernow) displayWeather(0);
        else if(fnPg==fnDateSunnext) displaySun(1,rtcGetDate(),inputLastTODMins);
        else if(fnPg==fnDateWeathernext) displayWeather(1);
        break; //end fnIsDate
      //fnIsDayCount removed in favor of paginated calendar
      case fnIsAlarm: //alarm
        word almTime; almTime = readEEPROM(0,true);
        editDisplay(almTime/60, 0, 1, readEEPROM(19,false), true); //hours with leading zero
        editDisplay(almTime%60, 2, 3, true, true);
        if(readEEPROM(2,false) && alarmSkip){ //alarm on+skip
          editDisplay(1,4,5,true,true); //01 to indicate off now, on maybe later
        } else { //alarm fully on or off
          editDisplay(readEEPROM(2,false),4,4,false,true);
          blankDisplay(5,5,true);
        }
        displayDim = (readEEPROM(2,false)?2:1); //status bright/dim
        break;
      case fnIsTimer: //timer - display time
        unsigned long td; td = (!(timerState&1)? timerTime: //If stopped, use stored duration
          //If running, use same math timerStop() does to calculate duration
          ((timerState>>1)&1? ((timerState>>4)&1? timerLapTime: ms()) - timerTime: //count up - use timerLapTime during lap display
            timerTime - ms() //count down
          )
        );
        byte tdc; tdc = (td%1000)/10; //capture hundredths (centiseconds)
        td = td/1000+(!((timerState>>1)&1)&&tdc!=0?1:0); //remove mils, and if countdown, round up
        //Countdown shows H:M:S, but on DISPLAY_SIZE<6 and H<1, M:S
        //Countup shows H:M:S, but if H<1, M:S:C, but if DISPLAY_SIZE<6 and M<1, S:C
        bool lz; lz = readEEPROM(19,false)&1;
        if((timerState>>1)&1){ //count up
          if(DISPLAY_SIZE<6 && td<60){ //under 1 min, 4-tubers: [SS]CC--
            if(td>=1||lz) editDisplay(td,0,1,lz,true); else blankDisplay(0,1,true); //secs, leading per lz, fade
            editDisplay(tdc,2,3,td>=1||lz,false); //cents, leading if >=1sec or lz, no fade
            blankDisplay(4,5,true); //just in case 4-tube code's running on a 6-tube clock, don't look ugly
          } else if(td<3600){ //under 1 hr: [MM][SS]CC
            if(td>=60||lz) editDisplay(td/60,0,1,lz,true); else blankDisplay(0,1,true); //mins, leading per lz, fade
            if(td>=1||lz) editDisplay(td%60,2,3,td>=60||lz,true); else blankDisplay(2,3,true); //secs, leading if >=1min or lz, fade
            editDisplay(tdc,4,5,td>=1||lz,false); //cents, leading if >=1sec or lz, no fade - hidden on 4-tubers
          } else { //over 1 hr: HHMMSS
            editDisplay(td/3600,0,1,lz,true); //hrs, leading per lz, fade
            editDisplay((td%3600)/60,2,3,true,true); //mins, leading, fade
            editDisplay(td%60,4,5,true,true); //secs, leading, fade
          }
        } else { //count down
          if(DISPLAY_SIZE<6 && td<3600){ //under 1 hr, 4-tubers: [MM]SS--
            if(td>=60||lz) editDisplay(td/60,0,1,lz,true); else blankDisplay(0,1,true); //mins, leading per lz, fade
            if(td>=1||lz) editDisplay(td%60,2,3,td>=60||lz,true); else blankDisplay(2,3,true); //secs, leading if >=1min or lz, fade
            blankDisplay(4,5,true); //just in case 4-tube code's running on a 6-tube clock, don't look ugly
          } else { //[HH][MM]SS
            if(td>=3600||lz) editDisplay(td/3600,0,1,lz,true); else blankDisplay(0,1,true); //hrs, leading per lz, fade
            if(td>=60||lz) editDisplay((td%3600)/60,2,3,td>=3600||lz,true); else blankDisplay(2,3,true); //mins, leading if >=1h or lz, fade
            editDisplay(td%60,4,5,td>=60||lz,true); //secs, leading if >=1m or lz, fade
          }
        }
        break;
      case fnIsTemp: //thermometer
        int temp; temp = rtcGetTemp();
        if(readEEPROM(45,false)==1) temp = temp*1.8 + 3200;
        //TODO another option to apply offset
        editDisplay(abs(temp)/100,1,3,(temp<0?true:false),true); //leading zeros if negative
        editDisplay(abs(temp)%100,4,5,true,true);
        break;
      case fnIsTest:
        editDisplay(rtcGetSecond(),0,0,true,false);
        editDisplay(rtcGetSecond(),1,1,true,false);
        editDisplay(rtcGetSecond(),2,2,true,false);
        editDisplay(rtcGetSecond(),3,3,true,false);
        editDisplay(rtcGetSecond(),4,4,true,false);
        editDisplay(rtcGetSecond(),5,5,true,false);
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
  //     if(displayNext[i]>9) Serial.print(F("-")); //blanked tube
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
// void serialPrintTime(int todMins){
//   if(todMins/60<10) Serial.print(F("0")); Serial.print(todMins/60,DEC); Serial.print(F(":"));
//   if(todMins%60<10) Serial.print(F("0")); Serial.print(todMins%60,DEC);
// }

//A snapshot of sun times, in minutes past midnight, calculated at clean time and when the date or time is changed.
//Need to capture this many, as we could be displaying these values at least through end of tomorrow depending on when cleaning happens.
#if ENABLE_DATE_RISESET
byte sunDate = 0; //date of month when calculated ("today")
int sunSet0  = -1; //yesterday's set
int sunRise1 = -1; //today rise
int sunSet1  = -1; //today set
int sunRise2 = -1; //tomorrow rise
int sunSet2  = -1; //tomorrow set
int sunRise3 = -1; //day after tomorrow rise
void calcSun(int y, byte m, byte d){
  //Calculates sun times and stores them in the values above
  blankDisplay(0,5,false); //immediately blank display so we can fade in from it elegantly
  Dusk2Dawn here(readEEPROM(10,true)/10, readEEPROM(12,true)/10, (float(readEEPROM(14,false))-100)/4);
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
  blankDisplay(4, 4, true);
  editDisplay(evtIsRise, 5, 5, false, true);
}
#else
//to give other fns something empty to call, when rise/set isn't enabled
void calcSun(int y, byte m, byte d){}
void displaySun(byte which, int d, int tod){}
#endif

void displayWeather(byte which){
  //shows high/low temp (for day/night respectively) on main tubes, and precipitation info on seconds tubes
  //which==0: display for current sun period (after last sun event)
  //which==1: display for next sun period (after next sun event)
  //IoT: Show the weather for the current period (after last sun event): high (day) or low (night) and precip info//IoT: Show the weather for the period after the next sun event
}




////////// Hardware outputs //////////

void initOutputs() {
  if(PIEZO_PIN>=0) pinMode(PIEZO_PIN, OUTPUT);
  if(RELAY_PIN>=0) {
    pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, HIGH); //LOW = device on
    quickBeep(71); //"primes" the beeper, seems necessary when relay pin is spec'd, otherwise first intentional beep doesn't happen TODO still true?
  }
  if(LED_PIN>=0) pinMode(LED_PIN, OUTPUT);
  updateLEDs(); //set to initial value
}

//Signals are like songs played on the beeper or relay, when the alarm/timer runs out or as an indicator.
//Like songs, a signal is made up of measures (usually 1sec each) tracked by the signalRemain counter.
//Measures are made up of steps such as beeps and relay pulses, tracked by signalMeasureStep.
//When used with switched relay, it simply turns on at the start, and off at the end, like a clock radio – the measures just wait it out.
//Timed using ms() instead of millis() – see timer/chrono for details.
unsigned long signalMeasureStartTime = 0; //to keep track of individual measures
byte signalMeasureStep = 0; //step number, or 255 if waiting for next measure, or 0 if not signaling
void signalStart(byte sigFn, byte sigDur){
  //sigFn isn't necessarily the current fn, just the one generating the signal
  //sigDur is the number of measures to put on signalRemain,
  //   or 0 for a single "quick measure" as applicable (i.e. skipped in radio mode).
  //Special case: if sigFn==fnIsAlarm, and sigDur>0, we'll use SIGNAL_DUR or SWITCH_DUR as appropriate.
  //If sigFn is given as -1 (255), we will use both the existing signalSource and signalPattern for purposes of configs and fibonacci.
  if(!(sigFn==255 && signalSource==fnIsTimer)) signalStop(); // if there is a signal going per the current signalSource, stop it - can only have one signal at a time – except if this is a forced fnIsTimer signal (for signaling runout options) which is cool to overlap timer sleep
  //except if this is a forced
  if(sigFn!=255) signalSource = sigFn;
  if(sigFn!=255) signalPattern = (
    (signalSource==fnIsTime && readEEPROM(21,false)==2)? -1: //special case: the pips
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
    //If switched relay, except if this is a forced fnIsTimer signal (for signaling runout options)
    if(getSignalOutput()==1 && RELAY_PIN>=0 && RELAY_MODE==0 && !(sigFn==255 && signalSource==fnIsTimer)) { //turn it on now
      signalRemain = (sigFn==fnIsAlarm? SWITCH_DUR: sigDur); //For alarm signal, use switched relay duration from config (eg 2hr)
      digitalWrite(RELAY_PIN,LOW); updateLEDs(); //LOW = device on
      //Serial.print(millis(),DEC); Serial.println(F(" Relay on, signalStart"));
    } else { //start pulsing. If there is no beeper or pulsed relay, this will have no effect since cycleSignal will clear it
      signalRemain = (sigFn==fnIsAlarm? SIGNAL_DUR: sigDur); //For alarm signal, use signal duration from config (eg 2min)
    }
  }
  //cycleSignal will pick up from here
} //end signalStart()
void signalStop(){ //stop current signal and clear out signal timer if applicable
  //Serial.println(F("signalStop"));
  signalRemain = 0; snoozeRemain = 0; signalMeasureStep = 0;
  if(getSignalOutput()==0 && PIEZO_PIN>=0) noTone(PIEZO_PIN);
  if(getSignalOutput()==1 && RELAY_PIN>=0){
    digitalWrite(RELAY_PIN,HIGH); updateLEDs(); //LOW = device on
    //Serial.print(millis(),DEC); Serial.println(F(" Relay off, signalStop"));
  }
} //end signalStop()
void cycleSignal(){
  //Called on every loop to control the signal.
  word measureDur = 1000; //interval between measure starts, ms - beep pattern can customize this
  if(signalMeasureStep){ //if there's a measure going (or waiting for a new one)
    if((getSignalOutput()==0 || (signalRemain==0 && signalSource==fnIsTimer)) && PIEZO_PIN>=0) { // beeper, or single measure for fnIsTimer runout setting
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
            (signalRemain==0 && signalSource==fnIsTimer? getHz(69): //fnIsTimer runout setting: use timer start pitch
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
    else if(getSignalOutput()==1 && RELAY_PIN>=0 && RELAY_MODE==1){ //pulsed relay
      //We don't follow the beep pattern here, we simply energize the relay for RELAY_PULSE time
      //Unlike beeper, we need to use a signalMeasureStep (2) to stop the relay.
      //See if it's time to start a new measure
      if(signalMeasureStep==255 && (unsigned long)(ms()-signalMeasureStartTime)>=measureDur){
        //Serial.println(F("Starting new measure, sPS -1 -> 1"));
        signalMeasureStartTime += measureDur;
        signalMeasureStep = 1;
      }
      //Upon new measure, start the pulse immediately
      if(signalMeasureStep==1){
        digitalWrite(RELAY_PIN,LOW); updateLEDs(); //LOW = device on
        //Serial.print(millis(),DEC); Serial.println(F(" Relay on, cycleSignal"));
        signalMeasureStep = 2; //set it up to stop
      }
      //See if it's time to stop the pulse
      else if(signalMeasureStep==2 && (unsigned long)(ms()-signalMeasureStartTime)>=RELAY_PULSE) {
        digitalWrite(RELAY_PIN,HIGH); updateLEDs(); //LOW = device on
        //Serial.print(millis(),DEC); Serial.println(F(" Relay off, cycleSignal"));
        //Set up for the next event
        if(signalRemain) signalRemain--; //this measure is done
        if(signalRemain) signalMeasureStep = 255; //if more measures, set up to start another measure
        else signalMeasureStep = 0; //otherwise, go idle - not using signalStop so as to let the beep expire on its own & fibonacci snooze continue
      }
    } //end pulsed relay
    else { //switched relay / default
      //Simply decrement signalRemain until it runs out - signalMeasureStep doesn't matter as long as it stays nonzero as at start
      //See if it's time to start a new measure
      if((unsigned long)(ms()-signalMeasureStartTime)>=measureDur){
        //Serial.println(F("Starting new measure, sPS -1 -> 1"));
        signalMeasureStartTime += measureDur;
      }
      //Set up for the next event
      if(signalRemain) signalRemain--; //this measure is done - but no need to change signalMeasureStep as we haven't changed it
      if(!signalRemain) signalStop(); //go idle - will kill the relay
    } //end switched relay
  } //end if there's a measure going
} //end cycleSignal()
word getSignalPitch(){ //for current signal: chime, timer, or (default) alarm
  return getHz(readEEPROM((signalSource==fnIsTime?41:(signalSource==fnIsTimer?40:39)),false));
}
word getHz(byte note){
  //Given a piano key note, return frequency
  byte relnote = note-49; //signed, relative to concert A
  float reloct = relnote/12.0; //signed
  word mult = 440*pow(2,reloct);
  return mult;
}
byte getSignalOutput(){ //for current signal: chime, timer, or (default) alarm: 0=piezo, 1=relay
  return readEEPROM((signalSource==fnIsTime?44:(signalSource==fnIsTimer?43:42)),false);
}
byte getSignalPattern(){ //for current signal: chime, timer, or (default) alarm:
  //0 = long (1/2-second beep)
  //1 = short (1/4-second beep)
  //2 = double (two 1/8-second beeps)
  //5 = cuckoo (two 1/8-second beeps, descending major third)
  //3 = triple (three 1/12-second beeps)
  //4 = quad (four 1/16-second beeps)
  return readEEPROM((signalSource==fnIsTime?49:(signalSource==fnIsTimer?48:47)),false);
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

const byte ledFadeStep = 10; //fade speed – with every loop() we'll increment/decrement the LED brightness (between 0-255) by this amount
byte ledStateNow = 0;
byte ledStateTarget = 0;
void updateLEDs(){
  //Run whenever something is changed that might affect the LED state: initial (initOutputs), signal start/stop, relay on/off, setting change
  if(LED_PIN>=0) {
    switch(readEEPROM(26,false)){
      case 0: //always off
        ledStateTarget = 0;
        //Serial.println(F("LEDs off always"));
        break;
      case 1: //always on
        ledStateTarget = 255;
        //Serial.println(F("LEDs on always"));
        break;
      case 2: //on, but follow night/away shutoff
        ledStateTarget = (displayDim==2? 255: (displayDim==1? 127: 0));
        //Serial.print(displayDim==2? F("LEDs on"): (displayDim==1? F("LEDs dim"): F("LEDs off"))); Serial.println(F(" per dim state"));
        break;
      case 3: //off, but on when alarm/timer sounds
        ledStateTarget = (signalRemain && (signalSource==fnIsAlarm || signalSource==fnIsTimer)? 255: 0);
        //Serial.print(signalRemain && (signalSource==fnIsAlarm || signalSource==fnIsTimer)?F("LEDs on"):F("LEDs off")); Serial.println(F(" per alarm/timer"));
        break;
      case 4: //off, but on with switched relay
        if(RELAY_PIN>=0 && RELAY_MODE==0) {
          ledStateTarget = (!digitalRead(RELAY_PIN)? 255: 0); //LOW = device on
          //Serial.print(!digitalRead(RELAY_PIN)? F("LEDs on"): F("LEDs off")); Serial.println(F(" per switched relay"));
        }
        break;
      default: break;
    } //end switch
  } //if LED_PIN
} //end updateLEDs
void cycleLEDs() {
  //Allows us to fade the LEDs to ledStateTarget by stepping via ledFadeStep
  //TODO: it appears setting analogWrite(pin,0) does not completely turn the LEDs off. Anything else we could do?
  if(ledStateNow != ledStateTarget) {
    if(ledStateNow < ledStateTarget) {
      ledStateNow = (ledStateTarget-ledStateNow <= ledFadeStep? ledStateTarget: ledStateNow+ledFadeStep);
    } else {
      ledStateNow = (ledStateNow-ledStateTarget <= ledFadeStep? ledStateTarget: ledStateNow-ledFadeStep);
    }
    // Serial.print(ledStateNow,DEC);
    // Serial.print(F(" => "));
    // Serial.println(ledStateTarget,DEC);
    analogWrite(LED_PIN,ledStateNow);
  }
}