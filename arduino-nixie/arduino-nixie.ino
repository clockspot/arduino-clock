// Digital clock code for the Arduino Nano in RLB Designs' Universal Nixie Driver Board
// featuring timekeeping by DS3231 RTC, driving up to six digits multiplexed 3x2 via two SN74141 driver chips
// Sketch by Luke McKenzie (luke@theclockspot.com) - https://github.com/clockspot/arduino-nixie
// Inspired by original sketch by Robin Birtles (rlb-designs.com) and Chris Gerekos
// Display cycling code derived from http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt

////////// Hardware configuration //////////
//Include the config file that matches your hardware setup. If needed, duplicate an existing one.

#include "configs/v8c-6tube-top.h"

#include "song.h"

// Song woody(0); //declare a Song object called woody
//
// void setup(){
//   woody.play();
// }
//
// void loop(){
//   woody.check(false);
// }


////////// Other includes, global consts, and vars //////////
#include <EEPROM.h> //Arduino - GNU LPGL
#include <DS3231.h> //Arduino - GNU LPGL
#include <Wire.h> //NorthernWidget - The Unlicense
#include <Dusk2Dawn.h> //TODO not needed unless whatever

/*EEPROM locations for non-volatile clock settings
Don't change which location is associated with which setting; they should remain permanent to avoid the need for EEPROM initializations after code upgrades; and they are used directly in code.
Most values in the clock are 1-byte; if 2-byte, high byte is loc, low byte is loc+1.
In updateDisplay(), special setting formats are deduced from the option max value (max 1439 is a time of day, max 56 is a UTC offset, etc)

These ones are set outside the options menu (defaults defined in initEEPROM()):
  0-1 Alarm time, mins
  2 Alarm on (mirrors volatile var)
  3-4 Day count year TODO free up
  5 Day count month
  6 Day count date
  7 Function preset (done by Alt when not power-switching)
  8 Functions/pages enabled (bitmask, dynamic) TODO
    const unsigned int FN_TIMER = 1<<0; //1
    const unsigned int FN_DAYCOUNT = 1<<1; //2
    const unsigned int FN_SUN = 1<<2; //4
    const unsigned int FN_WEATHER = 1<<3; //8
  9 Day count orientation (count down, count up) TODO
  10-11 Latitude
  12-13 Longitude
  14 UTC offset in quarter-hours (signed byte int8_t) - range is -48 (-12h, US Minor Outlying Islands) to +56 (+14h, Kiribati)
  15 DST on (mirrors volatile var)

These ones are set inside the options menu (defaults defined in arrays below).
Some are skipped when they wouldn't apply to a given clock's hardware config, see fnOptScroll(); these ones will also be set at startup to the start= values, see setup(). Otherwise, make sure these ones' defaults work for all configs.
  16 Time format
  17 Date format 
  18 Display date during time TODO only the one page
  19 Leading zero in hour, date, and month
  20 Digit fade duration
  21 Strike - skipped when no piezo and relay is switch (start=0)
  22 Auto DST
  23 Alarm days
  24 Alarm snooze
  25 Timer interval mode - skipped when no piezo and relay is switch (start=0)
  26 LED circuit behavior - skipped when no led pin
  27 Night mode
  28-29 Night start, mins
  30-31 Night end, mins
  32 Away mode
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
  45 Temperature format - skipped when fnIsTemp is not in fnsEnabled
  46 Anti-cathode poisoning
*/

//Options menu numbers (displayed in UI and readme), EEPROM locations, and default/min/max values.
//Option numbers/order can be changed (though try to avoid for user convenience);
//but option locs should be maintained so EEPROM doesn't have to be reset after an upgrade.
//                       General                    Alarm        Timer     Strike    Night and away mode              Lat   Long  UTC±
const byte optsNum[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13, 20,21,22, 30,31,32, 40,  41,  42,43,44,45,  46,  47,    50,   51, 52};
const byte optsLoc[] = {16,17,18,19,20,22,26,46,45, 23,42,39,24, 25,43,40, 21,44,41, 27,  28,  30,32,33,34,  35,  37,    10,   12, 14};
const  int optsDef[] = { 2, 1, 0, 0, 5, 0, 1, 0, 0,  0, 0,61, 9,  0, 0,61,  0, 0,61,  0,1320, 360, 0, 1, 5, 480,1020,     0,    0,  0};
const  int optsMin[] = { 1, 1, 0, 0, 0, 0, 0, 0, 0,  0, 0,49, 0,  0, 0,49,  0, 0,49,  0,   0,   0, 0, 0, 0,   0,   0, -1800,-1800,-48};
const  int optsMax[] = { 2, 5, 3, 1,20, 6, 4, 2, 1,  2, 1,88,60,  1, 1,88,  4, 1,88,  2,1439,1439, 2, 6, 6,1439,1439,  1800, 1800, 56};

//RTC objects
DS3231 ds3231; //an object to access the ds3231 specifically (temp, etc)
RTClib rtc; //an object to access a snapshot of the ds3231 rtc via now()
DateTime tod; //stores the now() snapshot for several functions to use
byte toddow; //stores the day of week (read separately from ds3231 dow counter)

// Hardware inputs and value setting
byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()
unsigned long inputLast = 0; //When a button was last pressed
unsigned long inputLast2 = 0; //Second-to-last of above
//TODO the math between these two may fail very rarely due to millis() rolling over while setting. Need to find a fix. I think it only applies to the rotary encoder though.
int inputLastTODMins = 0; //time of day, in minutes past midnight, when button was pressed. Used in paginated functions so they all reflect the same TOD.

const byte fnOpts = 201; //fn values from here to 255 correspond to options in the options menu
byte fn = fnIsTime; //currently displayed fn (per fnsEnabled)
byte fnPg = 0; //allows a function to have multiple pages
byte fnSetPg = 0; //whether this function is currently being set, and which option/page it's on
 int fnSetVal; //the value currently being set, if any
 int fnSetValMin; //min possible
 int fnSetValMax; //max possible
bool fnSetValVel; //whether it supports velocity setting (if max-min > 30)
 int fnSetValDate[3]; //holder for newly set date, so we can set it in 3 stages but set the RTC only once

//the calendar function page numbers, depending on which ones are enabled. See findFnAndPageNumbers
byte fnDatePages = 1;
byte fnDateCounter = 255;
byte fnDateSunlast = 255;
byte fnDateWeathernow = 255;
byte fnDateSunnext = 255;
byte fnDateWeathernext = 255;

// Volatile running values used throughout the code. (Others are defined right above the method that uses them)
bool dstOn = 0; //this is stored in EEPROM too, but only for correction after power loss – this var hedges against EEPROM failure in normal use
bool alarmOn = 0; //this is stored in EEPROM too, but only for power loss recovery – this var hedges against EEPROM failure in normal use
bool alarmSkip = 0;
byte signalSource = 0;
word signalRemain = 0; //alarm/timer signal timeout counter, seconds
word snoozeRemain = 0; //snooze timeout counter, seconds
word timerInitial = 0; //timer original setting, seconds - up to 18 hours (64,800 seconds - fits just inside a word)
word timerRemain = 0; //timer actual counter
unsigned long signalPulseStartTime = 0; //to keep track of individual pulses so we can stop them
word unoffRemain = 0; //un-off (briefly turn on tubes during full night or away modes) timeout counter, seconds
byte displayDim = 2; //dim per display or function: 2=normal, 1=dim, 0=off
byte cleanRemain = 11; //anti-cathode-poisoning clean timeout counter, increments at cleanSpeed ms (see loop()). Start at 11 to run at clock startup
char scrollRemain = 0; //"frames" of scroll – 0=not scrolling, >0=coming in, <0=going out, -128=scroll out at next change


////////// Main code control //////////

void setup(){
  Serial.begin(9600);
  Wire.begin();
  initInputs();
  delay(100); //prevents the below from firing in the event there's a capacitor stabilizing the input, which can read low falsely
  initEEPROM(readInput(mainSel)==LOW); //Do a hard init of EEPROM if button is held; else do a soft init to make sure vals in range
  //Some options need to be set to a fixed value per the hardware configuration.
  //These options will also be skipped in fnOptScroll so the user can't change them.
  if(relayPin<0 || piezoPin<0) { //If no relay or no piezo, set each signal output to [if no relay, then piezo; else relay]  
    writeEEPROM(42,(relayPin<0?0:1),false); //alarm
    writeEEPROM(43,(relayPin<0?0:1),false); //timer
    writeEEPROM(44,(relayPin<0?0:1),false); //strike
  }
  if(piezoPin>=0 && relayMode==0) { //If piezo and switched relay
    writeEEPROM(44,0,false); //strike forced to piezo
  }
  if(piezoPin<0 && relayMode==0) { //If switched relay and no piezo
    writeEEPROM(21,0,false); //turn off strike
    writeEEPROM(25,0,false); //turn off timer interval mode
  }
  if(!enableSoftAlarmSwitch) alarmOn = 1; //force alarm on if software switch is disabled
  else alarmOn = (readEEPROM(2,false)>0); //otherwise set alarm per EEPROM backup
  dstOn = (readEEPROM(15,false)>0); //set last known DST state per EEPROM backup
  //if LED circuit is not switched (v5.0 board), the LED menu setting (eeprom 26) doesn't matter
  findFnAndPageNumbers(); //initial values
  initOutputs(); //depends on some EEPROM settings
}

unsigned long pollCleanLast = 0; //every cleanSpeed ms
unsigned long pollScrollLast = 0; //every scrollSpeed ms
void loop(){
  unsigned long now = millis();
  //If we're running a tube cleaning, advance it every cleanSpeed ms.
  if(cleanRemain && (unsigned long)(now-pollCleanLast)>=cleanSpeed) { //account for rollover
    pollCleanLast=now;
    cleanRemain--;
    updateDisplay();
  }
  //If we're scrolling an animation, advance it every scrollSpeed ms.
  else if(scrollRemain!=0 && scrollRemain!=-128 && (unsigned long)(now-pollScrollLast)>=scrollSpeed) {
    pollScrollLast=now;
    if(scrollRemain<0) {
      scrollRemain++; updateDisplay();
    } else {
      scrollRemain--; updateDisplay();
      if(scrollRemain==0) scrollRemain=-128;
    }
  }
  //Every loop cycle, check the RTC and inputs (previously polled, but works fine without and less flicker)
  checkRTC(false); //if clock has ticked, decrement timer if running, and updateDisplay
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  doSetHold(); //if inputs have been held, this will do more things + updateDisplay as needed
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
  cycleLEDs();
}


////////// Control inputs //////////

void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  pinMode(mainSel, INPUT_PULLUP);
  pinMode(mainAdjUp, INPUT_PULLUP);
  pinMode(mainAdjDn, INPUT_PULLUP);
  pinMode(altSel, INPUT_PULLUP);
  //rotary encoder init
  //TODO encoder support
}

void checkInputs(){
  //TODO can all this if/else business be defined at load instead of evaluated every sample? OR is it compiled that way?
  //TODO potential issue: if user only means to rotate or push encoder but does both?
  //check button states
  checkBtn(mainSel); //main select
  if(mainAdjType==1) { checkBtn(mainAdjUp); checkBtn(mainAdjDn); } //if mainAdj is buttons
  //if(mainAdjType==2) checkRot(); //if mainAdj is rotary encoder TODO encoder support
  if(altSel!=0) checkBtn(altSel); //alt select (if equipped)
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
    btnCur = btn; btnCurHeld = 0; inputLast2 = inputLast; inputLast = now; inputLastTODMins = tod.hour()*60+tod.minute();
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(btnCur==btn && bnow==LOW) {
    if((unsigned long)(now-inputLast)>=btnLongHold && btnCurHeld < 3) { //account for rollover
      btnCurHeld = 3;
      ctrlEvt(btn,3); //hey, the button has been long-held
    }
    else if((unsigned long)(now-inputLast)>=btnShortHold && btnCurHeld < 2) {
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

void checkRot(){
  //Changes in rotary encoder. When rotation(s) occur, will call ctrlEvt to simulate btn presses.
  //TODO rotary encoder support
}//end checkRot


////////// Input handling and value setting //////////

bool stoppingSignal = false; //Special stuff (snooze canceling) happens right after a press that silences the signal
void ctrlEvt(byte ctrl, byte evt){
  //Handle control events (from checkBtn or checkRot), based on current fn and set state.
  //evt: 1=press, 2=short hold, 3=long hold, 0=release.
  //We only handle press evts for adj ctrls, as that's the only evt encoders generate.
  //But we can handle short and long holds and releases for the sel ctrls (always buttons).

  //Before all else, is it a press to stop the signal? Silence it
  if(signalRemain>0 && evt==1){
    stoppingSignal = true;
    signalStop();
    if(signalSource==fnIsAlarm) { //If this was the alarm
      //If the alarm is using the switched relay and this is the Alt button, don't set the snooze
      if(relayMode==0 && readEEPROM(42,false)==1 && altSel!=0 && ctrl==altSel) {
        quickBeep(); //Short signal to indicate the alarm has been silenced until tomorrow
        delay(250); //to flash the display to indicate this as well
      } else { //start snooze
        snoozeRemain = readEEPROM(24,false)*60; //snoozeRemain is seconds, but snooze duration is minutes
      }
    }
    return;
  }
  //After pressing to silence, short hold cancels a snooze; ignore other btn evts
  if(stoppingSignal){
    stoppingSignal = false;
    if(evt==2 && snoozeRemain>0) {
      snoozeRemain = 0;
      quickBeep(); //Short signal to indicate the alarm has been silenced until tomorrow
      delay(250); //to flash the display
    }
    btnStop();
    return;
  }
  
  //Is it a press for an un-off?
  unoffRemain = unoffDur; //always do this so continued button presses during an unoff keep it alive
  if(displayDim==0 && evt==1) {
    updateDisplay();
    btnStop();
    return;
  }
  
  if(fn < fnOpts) { //normal fn running/setting (not in options menu)

    if(evt==3 && ctrl==mainSel) { //mainSel long hold: enter options menu
      btnStop();
      fn = fnOpts;
      clearSet(); //don't need updateDisplay() here because this calls updateRTC with force=true
      return;
    }
    
    if(!fnSetPg) { //fn running
      if(evt==2 && ctrl==mainSel) { //mainSel hold: enter setting mode
        switch(fn){
          case fnIsTime: //set mins
            startSet((tod.hour()*60)+tod.minute(),0,1439,1); break;
          case fnIsDate: //depends what page we're on
            if(fnPg==0){ //regular date display: set year
              fnSetValDate[1]=tod.month(), fnSetValDate[2]=tod.day(); startSet(tod.year(),0,9999,1);
            } else if(fnPg==fnDateCounter){ //set year like date, but from eeprom like startOpt
              startSet(readEEPROM(3,true),2000,9999,1);
            } else if(fnPg==fnDateSunlast || fnPg==fnDateSunnext){ //lat and long
              //TODO
            } else if(fnPg==fnDateWeathernow || fnDateWeathernext){ //temperature units??
              //TODO
            } break;
          case fnIsAlarm: //set mins
            startSet(readEEPROM(0,true),0,1439,1); break;
          case fnIsTimer: //set mins, up to 18 hours (64,800 seconds - fits just inside a word)
            if(timerRemain>0) { timerRemain = 0; btnStop(); updateDisplay(); break; } //If the timer is running, zero it out.
            startSet(timerInitial/60,0,1080,1); break;
          //fnIsDayCount removed in favor of paginated calendar
          case fnIsTemp: //could do calibration here if so inclined
          case fnIsTubeTester:
          default: break;
        }
        return;
      }
      else if((ctrl==mainSel && evt==0) || ((ctrl==mainAdjUp || ctrl==mainAdjDn) && evt==1)) { //sel release or adj press
        //we can't handle sel press here because, if attempting to enter setting mode, it would switch the fn first
        if(ctrl==mainSel){
          fnScroll(1); //Go to next fn in the cycle
          fnPg = 0; //reset page counter in case we were in a paged display
          checkRTC(true); //updates display
        }
        else if(ctrl==mainAdjUp || ctrl==mainAdjDn) {
          if(fn==fnIsAlarm) switchAlarm(ctrl==mainAdjUp?1:-1); //switch alarm
          //if(fn==fnIsTime) TODO volume in I2C radio
        }
      }
      else if(altSel>0 && ctrl==altSel) { //alt sel press
        //TODO switch I2C radio
        //if switched relay, and soft switch enabled, we'll switch power.
        if(enableSoftPowerSwitch && relayPin>=0 && relayMode==0) { switchPower(0); btnStop(); }
        //Otherwise, this becomes our function preset.
        else {
          //On long hold, if this is not currently the preset, we'll set it, double beep, and btnStop.
          //(Decided not to let this button set things, because then it steps on the toes of Sel's functionality.)
          if(evt==2) {
            if(readEEPROM(7,false)!=fn) {
              btnStop();
              writeEEPROM(7,fn,false);
              quickBeep(); //beep 1
              delay(250); //to flash the display and delay beep 2
              quickBeep(); //beep 2
            }
          }
          //On short release, toggle between fnIsTime and the preset fn.
          else if(evt==0) {
            btnStop();
            if(fn==readEEPROM(7,false)) fn=fnIsTime; else fn=readEEPROM(7,false);
            fnPg = 0; //reset page counter in case we were in a paged display
            updateDisplay();
          }
        }
      }
    } //end fn running

    else { //fn setting
      if(evt==1) { //we respond only to press evts during fn setting
        //TODO could we do release/shorthold on mainSel so we can exit without making changes?
        //currently no, because we don't btnStop() when short hold goes into fn setting, in case long hold may go to options menu
        //so we can't handle a release because it would immediately save if releasing from the short hold.
        //Consider recording the btn start time when going into fn setting so we can distinguish its release from a future one
        if(ctrl==mainSel) { //mainSel push: go to next option or save and exit setting mode
          btnStop(); //not waiting for mainSelHold, so can stop listening here
          //We will set ds3231 time parts directly
          //con: potential for very rare clock rollover while setting; pro: can set date separate from time
          switch(fn){
            case fnIsTime: //save in RTC
              ds3231.setHour(fnSetVal/60);
              ds3231.setMinute(fnSetVal%60);
              ds3231.setSecond(0);
              clearSet(); break;
            case fnIsDate: //depends what page we're on
              if(fnPg==0){ //regular date display: save in RTC
                switch(fnSetPg){
                  case 1: //save year, set month
                    delay(300); //blink display to indicate save. Safe b/c we've btnStopped. See below for why
                    fnSetValDate[0]=fnSetVal;
                    startSet(fnSetValDate[1],1,12,2); break; 
                  case 2: //save month, set date
                    delay(300); //blink display to indicate save. Needed if set month == date: without blink, nothing changes.
                    fnSetValDate[1]=fnSetVal;
                    startSet(fnSetValDate[2],1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3); break;
                  case 3: //write year/month/date to RTC
                    ds3231.setYear(fnSetValDate[0]%100); //TODO: do we store century on our end? Per ds3231 docs, "The century bit (bit 7 of the month register) is toggled when the years register overflows from 99 to 00."
                    ds3231.setMonth(fnSetValDate[1]);
                    ds3231.setDate(fnSetVal);
                    ds3231.setDoW(dayOfWeek(fnSetValDate[0],fnSetValDate[1],fnSetVal)+1); //ds3231 weekday is 1-index
                    clearSet(); break;
                  default: break;
                }
              } else if(fnPg==fnDateCounter){ //set like date, save in eeprom like finishOpt
                switch(fnSetPg){
                  case 1: //save year, set month
                    delay(300); //blink display to indicate save. Safe b/c we've btnStopped. See below for why
                    writeEEPROM(3,fnSetVal,true);
                    startSet(readEEPROM(5,false),1,12,2); break;
                  case 2: //save month, set date
                    delay(300); //blink display to indicate save. Needed if set month == date: without blink, nothing changes.
                    writeEEPROM(5,fnSetVal,false);
                    startSet(readEEPROM(6,false),1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3); break;
                  case 3: //save date
                    writeEEPROM(6,fnSetVal,false);
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
            case fnIsTimer: //timer
              timerInitial = fnSetVal*60; //timerRemain is seconds, but timer is set by minute
              timerRemain = timerInitial; //set actual timer going
              if(relayPin>=0 && relayMode==0 && readEEPROM(43,false)==1) { //if switched relay, and timer signal is set to it
                digitalWrite(relayPin,LOW); //LOW = device on
                //Serial.print(millis(),DEC); Serial.println(F(" Relay on, timer set"));
                updateLEDs(); //LEDs following switch relay
                //TODO will this cancel properly? especially if alarm interrupts?
              }
              clearSet(); break;
            //fnIsDayCount removed in favor of paginated calendar
            case fnIsTemp:
              break;
            default: break;
          } //end switch fn
        } //end mainSel push
        if(ctrl==mainAdjUp) doSet(inputLast-inputLast2<velThreshold ? 10 : 1);
        if(ctrl==mainAdjDn) doSet(inputLast-inputLast2<velThreshold ? -10 : -1);
      } //end if evt==1
    } //end fn setting
    
  } //end normal fn running/setting
  
  else { //options menu setting - to/from EEPROM
    
    byte opt = fn-fnOpts; //current option index
    
    if(evt==2 && ctrl==mainSel) { //mainSel short hold: exit options menu
      btnStop();
      //if we were setting a value, writes setting val to EEPROM if needed
      if(fnSetPg) writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]>255?true:false);
      fn = fnIsTime;
      clearSet();
      return;
    }
    
    if(!fnSetPg){ //viewing option number
      if(ctrl==mainSel && evt==0) { //mainSel release: enter option value setting
        startSet(readEEPROM(optsLoc[opt],optsMax[opt]>255?true:false),optsMin[opt],optsMax[opt],1);
      }
      if(ctrl==mainAdjUp && evt==1) fnOptScroll(1); //next one up or cycle to beginning
      if(ctrl==mainAdjDn && evt==1) fnOptScroll(-1); //next one down or cycle to end?
      updateDisplay();
    } //end viewing option number

    else { //setting option value
      if(ctrl==mainSel && evt==0) { //mainSel release: save and exit option value setting
        //Writes setting val to EEPROM if needed
        writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]>255?true:false);
        clearSet();
      }
      if(ctrl==mainAdjUp && evt==1) doSet(inputLast-inputLast2<velThreshold ? 10 : 1);
      if(ctrl==mainAdjDn && evt==1) doSet(inputLast-inputLast2<velThreshold ? -10 : -1);
      updateDisplay();
    }  //end setting option value
  } //end options menu setting
  
} //end ctrlEvt

void fnScroll(char dir){
  //Switch to the next (1) or previous (-1) fn in fnsEnabled
  byte pos;
  byte posLast = sizeof(fnsEnabled)-1;
  if(dir==1) for(pos=0; pos<=posLast; pos++) if(fnsEnabled[pos]==fn) { fn = (pos==posLast?0:fnsEnabled[pos+1]); break; }
  if(dir==-1) for(pos=posLast; pos>=0; pos--) if(fnsEnabled[pos]==fn) { fn = (pos==0?posLast:fnsEnabled[pos-1]); break; }
}
void fnOptScroll(char dir){
  //Switch to the next options fn between min and max (inclusive), looping around at range ends
  byte posLast = fnOpts+sizeof(optsLoc)-1;
  if(dir==1) fn = (fn==posLast? fnOpts: fn+1);
  if(dir==-1) fn = (fn==fnOpts? posLast: fn-1);
  //Certain options don't apply to some hardware configurations; skip those
  byte optLoc = optsLoc[fn-fnOpts];
  if(
      (piezoPin<0 && (optLoc==39||optLoc==40||optLoc==41)) //no piezo: no signal pitches
      || ((piezoPin<0 && relayMode==0) && (optLoc==21||optLoc==25)) //no piezo, and relay is switch: no strike or timer interval mode
      || ((relayPin<0 || piezoPin<0) && (optLoc==42||optLoc==43||optLoc==44)) //no relay or no piezo: no alarm/timer/strike signal
      || ((relayMode==0) && (optLoc==44)) //relay is switch: no strike signal
      || ((ledPin<0) && (optLoc==26)) //no led pin: no led control
    ) {
    fnOptScroll(dir);
  }
  if(optLoc==45) { //temp not in fnsEnabled: skip temp format option (and calib if we get to it TODO)
    bool found = 0;
    for(byte fnct=0; fnct<sizeof(fnsEnabled); fnct++) if(fnsEnabled[fnct]==fnIsTemp) found = 1;
    if(found==0) fnOptScroll(dir);
  }
}

void startSet(int n, int m, int x, byte p){ //Enter set state at page p, and start setting a value
  fnSetVal=n; fnSetValMin=m; fnSetValMax=x; fnSetValVel=(x-m>30?1:0); fnSetPg=p;
  updateDisplay();
}
void doSet(int delta){
  //Does actual setting of fnSetVal, as told to by ctrlEvt or doSetHold. Makes sure it stays within range.
  if(delta>0) if(fnSetValMax-fnSetVal<delta) fnSetVal=fnSetValMax; else fnSetVal=fnSetVal+delta;
  if(delta<0) if(fnSetVal-fnSetValMin<abs(delta)) fnSetVal=fnSetValMin; else fnSetVal=fnSetVal+delta;
  updateDisplay();
}
unsigned long doSetHoldLast;
void doSetHold(){
  //When we're setting via an adj button that's passed a hold point, fire off doSet commands at intervals
  //TODO integrate this with checkInputs?
  unsigned long now = millis();
  if((unsigned long)(now-doSetHoldLast)>=250) {
    doSetHoldLast = now;
    if(fnSetPg!=0 && (mainAdjType==1 && (btnCur==mainAdjUp || btnCur==mainAdjDn)) ){ //if we're setting, and this is an adj btn
      bool dir = (btnCur==mainAdjUp ? 1 : 0);
      //If short hold, or long hold but high velocity isn't supported, use low velocity (delta=1)
      if(btnCurHeld==2 || (btnCurHeld==3 && fnSetValVel==false)) doSet(dir?1:-1);
      //else if long hold, use high velocity (delta=10)
      else if(btnCurHeld==3) doSet(dir?10:-10);
    }
  }
}
void clearSet(){ //Exit set state
  startSet(0,0,0,0);
  updateLEDs(); //in case LED setting was changed
  checkRTC(true); //force an update to tod and updateDisplay()
}

//EEPROM values are exclusively bytes (0-255) or words (unsigned ints, 0-65535)
//If it's a word, high byte is in loc, low byte is in loc+1
void initEEPROM(bool hard){
  //Set EEPROM and clock to defaults
  //First prevent the held button from doing anything else
  btnCur = mainSel; btnStop();
  //If a hard init, set the clock
  if(hard) {
    ds3231.setYear(18);
    ds3231.setMonth(1);
    ds3231.setDate(1);
    ds3231.setDoW(1); //2018-01-01 is Monday. DS3231 will keep count from here
    ds3231.setHour(0);
    ds3231.setMinute(0);
    ds3231.setSecond(0);
  }
  //Set the default values that aren't part of the options menu
  //only on hard init, or if the set value is outside of range
  if(hard || readEEPROM(0,true)>1439) writeEEPROM(0,420,true); //alarm at 7am
  if(hard){ alarmOn = !enableSoftAlarmSwitch; writeEEPROM(2,alarmOn,false); } //alarm is off, or on if no software switch
  if(hard || readEEPROM(3,true)<2018) writeEEPROM(3,2018,true); //day counter target: 2018...
  if(hard || readEEPROM(5,false)<1 || readEEPROM(5,false)>12) writeEEPROM(5,1,false); //...January...
  if(hard || readEEPROM(6,false)<1 || readEEPROM(6,false)>31) writeEEPROM(6,1,false); //...first.
  bool foundAltFn = false;
  for(byte fni=0; fni<sizeof(fnsEnabled); fni++) { if(fnsEnabled[fni]==readEEPROM(7,false)) { foundAltFn = true; break; }}
  if(hard || !foundAltFn) writeEEPROM(7,0,false); //Alt function preset – make sure it is not set to a function that isn't enabled in this clock  
  //then the options menu defaults
  bool isWord = false;
  for(byte opt=0; opt<sizeof(optsLoc); opt++) {
    isWord = (optsMax[opt]>255?true:false); //TODO shouldn't this be optsMax
    if(hard || readEEPROM(optsLoc[opt],isWord)<optsMin[opt] || readEEPROM(optsLoc[opt],isWord)>optsMax[opt])
      writeEEPROM(optsLoc[opt],optsDef[opt],isWord);
  } //end for
} //end initEEPROM()
word readEEPROM(int loc, bool isWord){
  if(isWord) {
    //TODO why was it done this way
    //word w = (EEPROM.read(loc)<<8)+EEPROM.read(loc+1); return w; //word / unsigned int, 0-65535
    return (EEPROM.read(loc)<<8)+EEPROM.read(loc+1);
  } else {
    //byte b = EEPROM.read(loc); return b; //byte, 0-255
    return EEPROM.read(loc);
  }
}
void writeEEPROM(int loc, int val, bool is2Byte){
  if(is2Byte) {
    Serial.print(F("EEPROM write 2 bytes (")); Serial.print(val,DEC); Serial.print(F("):"));
    Serial.print(F(" loc ")); Serial.print(loc,DEC); Serial.print(F(" val "));
    if(EEPROM.read(loc)!=highByte(val)) { Serial.print(highByte(val),DEC); } else { Serial.print(F("no change")); } Serial.print(F(";"));
    EEPROM.update(loc,highByte(val));
    Serial.print(F(" loc ")); Serial.print(loc+1,DEC); Serial.print(F(" val "));
    if(EEPROM.read(loc+1)!=lowByte(val)) { Serial.println(lowByte(val),DEC); } else { Serial.println(F("no change")); }
    EEPROM.update(loc+1,lowByte(val));
  } else {
    Serial.print(F("EEPROM write byte (")); Serial.print(val,DEC); Serial.print(F("):"));
    Serial.print(F(" loc ")); Serial.print(loc,DEC); Serial.print(F(" val "));
    if(EEPROM.read(loc)!=val) { Serial.println(val,DEC); } else { Serial.println(F("no change")); }
    EEPROM.update(loc,val);
  }
  //Serial.println();
}

void findFnAndPageNumbers(){
  //Each function, and each page in a paged function, has a number. //TODO should pull from EEPROM 8
  fnDatePages = 1; //date function always has a page for the date itself
  if(true){ fnDatePages++; fnDateCounter=fnDatePages-1; }
  if(false){ fnDatePages++; fnDateSunlast=fnDatePages-1; }
  if(false){ fnDatePages++; fnDateWeathernow=fnDatePages-1; }
  if(false){ fnDatePages++; fnDateSunnext=fnDatePages-1; }
  if(false){ fnDatePages++; fnDateWeathernext=fnDatePages-1; }
}
byte daysInMonth(word y, byte m){
  if(m==2) return (y%4==0 && (y%100!=0 || y%400==0) ? 29 : 28);
  //https://cmcenroe.me/2014/12/05/days-in-month-formula.html
  else return (28 + ((m + (m/8)) % 2) + (2 % m) + (2 * (1/m)));
}
//The following are for use with the Day Counter feature - find number of days between now and target date
//I don't know how reliable the unixtime() is from RTClib past 2038, plus there might be timezone complications
//so we'll just calculate the number of days since 2000-01-01
long dateToDayCount(word y, byte m, byte d){
  long dc = (y-2000)*365; //365 days for every full year since 2000
  word i; for(i=0; i<y-2000; i++) if(i%4==0 && (i%100!=0 || i%400==0)) dc++; //+1 for every feb 29th in those years
  for(i=1; i<m; i++) dc += daysInMonth(y,i); //add every full month since start of this year
  dc += d-1; //every full day since start of this month
  return dc;
}
byte dayOfWeek(word y, byte m, byte d){
  //DS3231 doesn't really calculate the day of the week, it just keeps a counter.
  //When setting date, we'll calculate per https://en.wikipedia.org/wiki/Zeller%27s_congruence
  byte yb = y%100; //2-digit year
  byte ya = y/100; //century
  //For this formula, Jan and Feb are considered months 11 and 12 of the previous year.
  //So if it's Jan or Feb, add 10 to the month, and set back the year and century if applicable
  if(m<3) { m+=10; if(yb==0) { yb=99; ya-=1; } else yb-=1; }
  else m -= 2; //otherwise subtract 2 from the month
  return (d + ((13*m-1)/5) + yb + (yb/4) + (ya/4) + 5*ya) %7;
}


////////// Clock ticking and timed event triggering //////////

byte rtcSecLast = 61;
void checkRTC(bool force){
  //Checks display timeouts;
  //checks for new time-of-day second -> decrements timers and checks for timed events;
  //updates display for "running" functions.
  unsigned long now = millis();
  
  //Things to do every time this is called: timeouts to reset display. These may force a tick.
  //Option/setting timeout: if we're in the options menu, or we're setting a value
  if(fnSetPg || fn>=fnOpts){
    if((unsigned long)(now-inputLast)>=timeoutSet*1000) { fnSetPg = 0; fn = fnIsTime; force=true; } //Time out after 2 mins
  }
  //Paged-display mode timeout //TODO change fnIsDate to consts? //TODO timeoutPageFn var
  else if(fn==fnIsDate && (unsigned long)(now-inputLast)>=2500) {
    //Here we just have to increment the page and decide when to reset. updateDisplay() will do the rendering
    fnPg++; inputLast+=2500; //but leave inputLastTODMins alone so the subsequent page displays will be based on the same TOD
    if(fnPg >= fnDatePages){ fnPg = 0; fn = fnIsTime; }
    force=true;
  }
  //Temporary-display mode timeout: if we're *not* in a permanent one (time, or running/signaling timer)
  else if(fn!=fnIsTime && !(fn==fnIsTimer && (timerRemain>0 || signalRemain>0))){ // && fn!=readEEPROM(7,false)
    if((unsigned long)(now-inputLast)>=timeoutTempFn*1000) { fnSetPg = 0; fn = fnIsTime; force=true; }
  }
  //Stop a signal pulse if it's time to
  //This is only used for relay pulses, since beeper beep durations are done via tone()
  //So we can safely assume the length of the pulse should be relayPulse
  if(signalPulseStartTime && (unsigned long)(now-signalPulseStartTime)>=relayPulse) { signalPulseStop(); signalPulseStartTime = 0; }
  
  //Update things based on RTC
  tod = rtc.now();
  toddow = ds3231.getDoW()-1; //ds3231 weekday is 1-index
  
  if(rtcSecLast != tod.second() || force) { //If it's a new RTC second, or we are forcing it
    
    //Things to do at specific times
    if(tod.second()==0) { //at top of minute
      //at 2am, or if this is first run, check for DST change
      if((tod.minute()==0 && tod.hour()==2) || rtcSecLast==61) autoDST();
      //at the alarm trigger time
      if(tod.hour()*60+tod.minute()==readEEPROM(0,true)){
        if(alarmOn && !alarmSkip) { //if the alarm is on and not skipped, sound it!
          fnSetPg = 0; fn = fnIsTime; signalStart(fnIsAlarm,1,0);
        }
        //set alarmSkip for the next instance of the alarm
        alarmSkip =
          //if alarm is any day of the week
          (readEEPROM(23,false)==0 ||
          //or if alarm is weekday only, and tomorrow is a weekday
          (readEEPROM(23,false)==1 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),(toddow==6?0:toddow+1))) ||
          //or if alarm is weekend only, and tomorrow is a weekend
          (readEEPROM(23,false)==2 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),(toddow==6?0:toddow+1)))
          ? 0: 1); //then don't skip the next alarm; else skip it
      } //end alarm trigger
    }
    //At bottom of minute, see if we should show the date
    if(tod.second()==30 && fn==fnIsTime && fnSetPg==0 && unoffRemain==0) {
      if(readEEPROM(18,false)>=2) { fn = fnIsDate; inputLast = now; inputLastTODMins = tod.hour()*60+tod.minute(); fnPg = 254; updateDisplay(); }
      if(readEEPROM(18,false)==3) { startScroll(); }
    }
    //Anti-poisoning routine triggering: start when applicable, and not at night, during setting, or after a button press (unoff)
    if(tod.second()<2 && displayDim==2 && fnSetPg==0 && unoffRemain==0) {
      switch(readEEPROM(46,false)) { //how often should the routine run?
        case 0: //every day
          if(readEEPROM(27,false)>0? //is night mode enabled?
            tod.second()==0 && tod.hour()*60+tod.minute()==readEEPROM(28,true): //if so, at start of night mode (at second :00 before dim is in effect)
            tod.second()==1 && tod.hour()*60+tod.minute()==0) //if not, at 00:00:01
              cleanRemain = 51; //run routine for five cycles
          break;
        case 1: //every hour
          if(tod.second()==1 && tod.minute()==0) //at min/sec :00:01
            cleanRemain = 51; //run routine for five cycles
          break;
        case 2: //every minute
          if(tod.second()==1) //at second :01
            cleanRemain = 11; //run routine for one cycle
          break;
        default: break;
      }
    }
    
    //Strikes - only if fn=clock, not setting, not night/away. Setting 21 will be off if signal type is no good
    //Short pips before the top of the hour
    if(tod.minute()==59 && tod.second()>=55 && readEEPROM(21,false)==2 && signalRemain==0 && snoozeRemain==0 && fn==fnIsTime && fnSetPg==0 && displayDim==2) {
      signalStart(fnIsTime,0,100);
    }
    //Strikes on/after the hour
    if(tod.second()==0 && (tod.minute()==0 || tod.minute()==30) && signalRemain==0 && snoozeRemain==0 && fn==fnIsTime && fnSetPg==0 && displayDim==2){
      byte hr; hr = tod.hour(); hr = (hr==0?12:(hr>12?hr-12:hr));
      switch(readEEPROM(21,false)) {
        case 1: //single beep
          if(tod.minute()==0) signalStart(fnIsTime,0,0); break;
        case 2: //long pip
          if(tod.minute()==0) signalStart(fnIsTime,0,500); break;
        case 3: //hour strike via normal signal cycle
          if(tod.minute()==0) signalStart(fnIsTime,hr,0); break;
        case 4: //ship's bell at :00 and :30 mins via normal signal cycle
          signalStart(fnIsTime,((hr%4)*2)+(tod.minute()==30?1:0),0); break;
        default: break;
      } //end strike type
    } //end strike
    
    //Things to do every natural second (decrementing real-time counters)
    if(rtcSecLast != tod.second()) {
      //If timer has time on it, decrement and trigger beeper if we reach zero
      if(timerRemain>0) {
        timerRemain--;
        if(timerRemain<=0) { //timer has elasped
          if(relayPin>=0 && relayMode==0 && readEEPROM(43,false)==1) { //if switched relay, and timer signal is set to it
            signalStop(); //interval timer setting is ignored (unlike below)
          } else { //not switched relay: turn on signal
            if(readEEPROM(25,false)) { //interval timer: a short signal and restart; don't change to timer fn
              signalStart(fnIsTimer,0,0); timerRemain = timerInitial;
            } else {
              fnSetPg = 0; fn = fnIsTimer; inputLast = now; inputLastTODMins = tod.hour()*60+tod.minute(); signalStart(fnIsTimer,signalDur,0);
            }
          } //end not switched relay
        } //end timer elapsed
      }
      //If alarm snooze has time on it, decrement and trigger signal if we reach zero (and alarm is still on)
      //Won't check alarm skip status here, as it reflects tomorrow
      if(snoozeRemain>0) {
        snoozeRemain--;
        if(snoozeRemain<=0 && alarmOn) {
          fnSetPg = 0; fn = fnIsTime; signalStart(fnIsAlarm,1,0);
        }
      }
      //If signal has time on it, decrement and make a beep (if applicable to outputs)
      if(signalRemain>0) {
        signalRemain--;
        if(signalRemain<=0 && relayPin>=0 && relayMode==0) {
          signalStop();
        } else {
          signalPulseStart(0);
        }
      }
      if(unoffRemain>0) {
        unoffRemain--; //updateDisplay will naturally put it back to off state if applicable
      }
    } //end natural second
    
    //Finally, update the display, whether natural tick or not, as long as we're not setting or on a scrolled display (unless forced eg. fn change)
    //This also determines night/away mode, which is why strikes will happen if we go into off at top of hour, and not when we come into on at the top of the hour TODO find a way to fix this
    if(fnSetPg==0 && (scrollRemain==0 || force)) updateDisplay();
    
    rtcSecLast = tod.second();
    
  } //end if force or new second
} //end checkRTC()

void autoDST(){
  //Call daily when clock reaches 2am, and at first run.
  bool dstNow = isDST(tod.year(),tod.month(),tod.day());
  if(dstNow!==dstOn){ ds3231.setHour(dstNow>dstOn? 3: 1); dstOn = dstNow; writeEEPROM(15,dstOn,false); }
}
bool isDST(int y, char m, char d){
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
char nthSunday(int y, char m, char nth){
  if(nth>0) return (((7-dayOfWeek(y,m,1))%7)+1+((nth-1)*7));
  if(nth<0) return (dayOfWeek(y,m,1)==0 && daysInMonth(y,m)>28? 29: nthSunday(y,m,1)+21+((nth+1)*7));
  return 0;
}

void switchAlarm(char dir){
  if(enableSoftAlarmSwitch){
    signalStop(); //snoozeRemain = 0;
    //There are three alarm states - on, on with skip (skips the next alarm trigger), and off.
    //Currently we use up/down buttons or a rotary control, rather than a binary switch, so we can cycle up/down through these states.
    //On/off is stored in EEPROM to survive power loss; skip is volatile, not least because it can change automatically and I don't like making automated writes to EEPROM if I can help it. Skip state doesn't matter when alarm is off.
    //Using tone() instead of quickBeep since we need to set the pitch. TODO replace with the Song class.
    if(dir==0) dir=(alarmOn?-1:1); //If alarm is off, cycle button goes up; otherwise down.
    if(dir==1){
      if(!alarmOn){ //if off, go straight to on, no skip
        alarmOn=1; writeEEPROM(2,alarmOn,false); alarmSkip=0; if(piezoPin>=0) { signalStop(); tone(piezoPin, getHz(76), 100); }} //C7
      else if(alarmSkip){ //else if skip, go to on
        alarmSkip=0; if(piezoPin>=0) { signalStop(); tone(piezoPin, getHz(76), 100); }}} //C7
    if(dir==-1){
      if(alarmOn){ //if on
        if(!alarmSkip){ //if not skip, go to skip
          alarmSkip=1; if(piezoPin>=0) { signalStop(); tone(piezoPin, getHz(71), 100); }} //G6
        else { //if skip, go to off
          alarmOn=0; writeEEPROM(2,alarmOn,false); if(piezoPin>=0) { signalStop(); tone(piezoPin, getHz(64), 100); }}}} //C6
    updateDisplay();
  }
}
void switchPower(char dir){
  signalRemain = 0; snoozeRemain = 0; //in case alarm is going now - alternatively use signalStop()?
  //If the timer is running and is using the switched relay, this instruction conflicts with it, so cancel it
  if(timerRemain>0 && readEEPROM(43,false)==1) {
    timerRemain=0;
    updateDisplay();
  }
  //relayPin state is the reverse of the appliance state: LOW = device on, HIGH = device off
  //Serial.print(millis(),DEC);
  //Serial.print(F(" Relay requested to "));
  if(dir==0) { //toggle
    dir = (digitalRead(relayPin)?1:-1); //LOW = device on, so this effectively does our dir reversion for us
    //Serial.print(dir==1?F("toggle on"):F("toggle off"));
  } else {
    //Serial.print(dir==1?F("switch on"):F("switch off"));
  }
  digitalWrite(relayPin,(dir==1?0:1)); //LOW = device on
  //Serial.println(F(", switchPower"));
  updateLEDs(); //LEDs following switch relay
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


////////// Display data formatting //////////

byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start. Change this to change tubes.
byte displayLast[6] = {11,11,11,11,11,11}; //for noticing changes to displayNext and fading the display to it
byte scrollDisplay[6] = {15,15,15,15,15,15}; //For animating a value into displayNext from right, and out to left

void updateDisplay(){
  //Run as needed to update display when the value being shown on it has changed
  //This formats the new value and puts it in displayNext[] for cycleDisplay() to pick up
  
  if(scrollRemain==-128) {  //If the current display is flagged to be scrolled out, do it. This is kind of the counterpart to startScroll()
    for(byte i=0; i<6; i++) scrollDisplay[i] = displayNext[i]; //cache the current value in scrollDisplay[] just in case it changed
    scrollRemain = (0-displaySize)-1;
  }
  
  if(cleanRemain) { //cleaning tubes
    displayDim = 2;
    byte digit = 10-((cleanRemain-1)%10); //(11-cleanRemain)%10;
    editDisplay(digit,0,0,true,false);
    editDisplay(digit,1,1,true,false);
    editDisplay(digit,2,2,true,false);
    editDisplay(digit,3,3,true,false);
    editDisplay(digit,4,4,true,false);
    editDisplay(digit,5,5,true,false);
  }
  /*
  Scrolling "frames" (ex. on 4-tube clock):
  To use this, call editDisplay() as usual, then startScroll()
  4:              [       ]1 2 3 4      tube[n] (0-index) is source[n-scrollRemain] unless that index < 0, then blank
  3:              [      1]2 3 4
  2:              [    1 2]3 4
  1:              [  1 2 3]4
  0/-128:         [1 2 3 4]
  -4:            1[2 3 4  ]      tube[n] is source[n+displaySize+scrollRemain+1] unless that index >= displaySize, then blank
  -3:          1 2[3 4    ]
  -2:        1 2 3[4      ]
  -1:      1 2 3 4[       ]
  0: (scroll functions are skipped to display whatever's applicable)
  */
  else if(scrollRemain>0) { //scrolling display: value coming in - these don't use editDisplay as we're going array to array
    for(byte i=0; i<displaySize; i++) {
      char isrc = i-scrollRemain;
      displayNext[i] = (isrc<0? 15: scrollDisplay[isrc]); //allow to fade
    }
  }
  else if(scrollRemain<0 && scrollRemain!=-128) { //scrolling display: value going out
    for(byte i=0; i<displaySize; i++) {
      char isrc = i+displaySize+scrollRemain+1;
      displayNext[i] = (isrc>=displaySize? 15: scrollDisplay[isrc]); //allow to fade
    }
  }
  else if(fnSetPg) { //setting value, for either fn or option
    displayDim = 2;
    blankDisplay(4, 5, false);
    if(fnSetValMax==1439) { //Time of day (0-1439 mins, 0:00–23:59): show hrs/mins
      editDisplay(fnSetVal/60, 0, 1, readEEPROM(19,false), false); //hours with leading zero
      editDisplay(fnSetVal%60, 2, 3, true, false);
    } else if(fnSetValMax==1080) { //Timer duration (0-1080 mins, up to 18:00): show hrs/mins w/minimal leading
      if(fnSetVal>=60) editDisplay(fnSetVal/60, 0, 1, false, false); else blankDisplay(0,1,false); //hour only if present, else blank
      editDisplay(fnSetVal%60, 2, 3, (fnSetVal>=60?true:false), false); //leading zero only if hour present
      editDisplay(0,4,5,true,false); //placeholder seconds
    } else if(fnSetValMax==88) { //A piezo pitch. Play a short demo beep.
      editDisplay(fnSetVal, 0, 3, false, false);
      if(piezoPin>=0) { signalStop(); tone(piezoPin, getHz(fnSetVal), 100); } //One exception to using signalStart since we need to specify pitch directly
    } else if(fnSetValMax==56) { //Timezone offset from UTC in quarter hours
      editDisplay((abs(fnSetVal)*25)/100, 0, 1, fnSetVal<0, false); //hours, leading zero for negatives
      editDisplay((abs(fnSetVal)%4)*15, 2, 3, true, false); //minutes, leading zero always
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
    word todmins = tod.hour()*60+tod.minute();
    //In order of precedence: //TODO can we fade between dim states? 
    //clock at work: away on weekends, all day
    if( readEEPROM(32,false)==1 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),toddow) )
      displayDim = (unoffRemain>0? 2: 0); //unoff overrides this
    //clock at home: away on weekdays, during office hours only
    else if( readEEPROM(32,false)==2 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),toddow) && isTimeInRange(readEEPROM(35,true), readEEPROM(37,true), todmins) ) displayDim = (unoffRemain>0? 2: 0);
    //night mode - if night end is 0:00, use alarm time instead
    else if( readEEPROM(27,false) && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins) ) displayDim = (readEEPROM(27,false)==1?1:(unoffRemain>0?2:0)); //dim or (unoff? bright: off)
    //normal
    else displayDim = 2;
    updateLEDs();
    
    switch(fn){
      case fnIsTime:
        byte hr; hr = tod.hour();
        if(readEEPROM(16,false)==1) hr = (hr==0?12:(hr>12?hr-12:hr));
        editDisplay(hr, 0, 1, readEEPROM(19,false), true);
        editDisplay(tod.minute(), 2, 3, true, true);
        if(readEEPROM(18,false)==1) editDisplay(tod.day(), 4, 5, readEEPROM(19,false), true); //date
        else editDisplay(tod.second(), 4, 5, true, true); //seconds
        break;
      case fnIsDate: //a paged display
        if(fnPg==0 || fnPg==254){ //plain ol' date - 0 will continue to other pages, 254 will only display date then return to time (e.g. at half minute)
          byte df; df = readEEPROM(17,false); //1=m/d/w, 2=d/m/w, 3=m/d/y, 4=d/m/y, 5=y/m/d
          if(df<=4) {
            editDisplay((df==1||df==3?tod.month():tod.day()),0,1,readEEPROM(19,false),true); //month or date first
            editDisplay((df==1||df==3?tod.day():tod.month()),2,3,readEEPROM(19,false),true); //date or month second
            editDisplay((df<=2?toddow:tod.year()),4,5,(df<=2?false:true),true); //dow or year third - dow never leading zero, year always
          }
          else { //df==5
            editDisplay(tod.year(),0,1,true,true); //year always has leading zero
            editDisplay(tod.month(),2,3,readEEPROM(19,false),true);
            editDisplay(tod.day(),4,5,readEEPROM(19,false),true);
          }
        }
        else if(fnPg==fnDateCounter){
          long targetDayCount; targetDayCount = dateToDayCount(
            readEEPROM(3,true),
            readEEPROM(5,false),
            readEEPROM(6,false)
          );
          long currentDayCount; currentDayCount = dateToDayCount(tod.year(),tod.month(),tod.day());
          editDisplay(abs(targetDayCount-currentDayCount),0,3,false,true);
          //TODO for now don't indicate negative. Elsewhere we use leading zeros to represent negative but I don't like how that looks here
          blankDisplay(4,5,true);
        }
        else if(fnPg==fnDateSunlast) displaySun(0);
        else if(fnPg==fnDateWeathernow) displayWeather(0);
        else if(fnPg==fnDateSunnext) displaySun(1);
        else if(fnPg==fnDateWeathernext) displayWeather(1);
        break; //end fnIsDate
      //fnIsDayCount removed in favor of paginated calendar
      case fnIsAlarm: //alarm
        word almTime; almTime = readEEPROM(0,true);
        editDisplay(almTime/60, 0, 1, readEEPROM(19,false), true); //hours with leading zero
        editDisplay(almTime%60, 2, 3, true, true);
        if(alarmOn && alarmSkip){ //alarm on+skip
          editDisplay(1,4,5,true,true); //01 to indicate off now, on maybe later
        } else { //alarm fully on or off
          editDisplay(alarmOn,4,4,false,true);
          blankDisplay(5,5,true);
        }
        displayDim = (alarmOn?2:1); //status bright/dim
        break;
      case fnIsTimer: //timer - display time left.
        //Relative unit positioning: when t <1h, display min/sec in place of hr/min on 4-tube displays
        byte mspos; mspos = (displaySize<6 && timerRemain<3600? 0 : 2);
        if(timerRemain >= 3600) { //t >=1h: display hr on first two tubes
          editDisplay(timerRemain/3600,0,1,false,true);
        } else blankDisplay(0,1,true);
        if(timerRemain >= 60) { //t >=1m: display minute (in relative position). Leading zero if t >=1h.
          editDisplay((timerRemain%3600)/60,mspos,mspos+1,(timerRemain>=3600?true:false),true);
        } else blankDisplay(mspos,mspos+1,true);
        //display second (in relative position). Leading zero if t>=1m.
        editDisplay(timerRemain%60,mspos+2,mspos+3,(timerRemain>=60?true:false),true);
        //just in case 4-digit code is run on a 6-digit clock
        if(displaySize<6) blankDisplay(4,5,false);
        break;
      case fnIsTemp: //thermometer
        int temp; temp = ds3231.getTemperature()*100;
        if(readEEPROM(45,false)==1) temp = temp*1.8 + 3200;
        //TODO another option to apply offset
        editDisplay(abs(temp)/100,1,3,(temp<0?true:false),true); //leading zeros if negative
        editDisplay(abs(temp)%100,4,5,true,true);
        break;
      case fnIsTubeTester:
        editDisplay(tod.second(),0,0,true,false);
        editDisplay(tod.second(),1,1,true,false);
        editDisplay(tod.second(),2,2,true,false);
        editDisplay(tod.second(),3,3,true,false);
        editDisplay(tod.second(),4,4,true,false);
        editDisplay(tod.second(),5,5,true,false);
      default: break;
    }//end switch
  } //end if fn running
  
  if(false) { //DEBUG MODE: when display's not working, just write it to the console, with time
    if(tod.hour()<10) Serial.print(F("0"));
    Serial.print(tod.hour(),DEC);
    Serial.print(F(":"));
    if(tod.minute()<10) Serial.print(F("0"));
    Serial.print(tod.minute(),DEC);
    Serial.print(F(":"));
    if(tod.second()<10) Serial.print(F("0"));
    Serial.print(tod.second(),DEC);
    Serial.print(F("   "));
    for(byte i=0; i<displaySize; i++) {
      if(i%2==0 && i!=0) Serial.print(F(" ")); //spacer between units
      if(displayNext[i]>9) Serial.print(F("-")); //blanked tube
      else Serial.print(displayNext[i],DEC);
    }
    Serial.println();
  }
  
  //Write display changes to console
  //for(byte w=0; w<6; w++) { if(displayNext[w]>9) Serial.print(F("-")); else Serial.print(displayNext[w],DEC); }
  //Serial.println();
       
} //end updateDisplay()

void displaySun(char which){
  //which==0: display last sun event: yesterday's sunset, today's sunrise, or today's sunset
  //which==1: display next sun event: today's sunrise, today's sunset, or tomorrow's sunrise
  Dusk2Dawn here(readEEPROM(10,true), readEEPROM(12,true), float(readEEPROM(14,true))/4);
  int rise = here.sunrise(tod.year(),tod.month(),tod.day(),
  //int8_t 
  //inputLastTODMins
}

void displayWeather(char which){
  //shows high/low temp (for day/night respectively) on main tubes, and precipitation info on seconds tubes
  //which==0: display for current sun period (after last sun event)
  //which==1: display for next sun period (after next sun event)
  //IoT: Show the weather for the current period (after last sun event): high (day) or low (night) and precip info//IoT: Show the weather for the period after the next sun event
}

void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade){
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank tube
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4 tubes --> 0015)
  word place;
  for(byte i=0; i<=posEnd-posStart; i++){
    switch(i){ //because int(pow(10,1))==10 but int(pow(10,2))==99...
      case 0: place=1; break;
      case 1: place=10; break;
      case 2: place=100; break;
      case 3: place=1000; break;
      default: break;
    }
    displayNext[posEnd-i] = (i==0&&n==0 ? 0 : (n>=place ? (n/place)%10 : (leadingZeros?0:15)));
    if(!fade) displayLast[posEnd-i] = displayNext[posEnd-i]; //cycleDisplay will be none the wiser
  }
} //end editDisplay()
void blankDisplay(byte posStart, byte posEnd, byte fade){
  for(byte i=posStart; i<=posEnd; i++) { displayNext[i]=15; if(!fade) displayLast[i]=15; }
} //end blankDisplay();
void startScroll() { //To scroll a value in, call this after calling editDisplay as normal
  for(byte i=0; i<6; i++) scrollDisplay[i] = displayNext[i]; //cache the incoming value in scrollDisplay[]
  blankDisplay(0,5,true);
  scrollRemain = displaySize+1; //this will trigger updateDisplay() to start scrolling. displaySize+1 adds blank frame at front
} //end startScroll()


////////// Hardware outputs //////////

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {outA1,outA2,outA3,outA4};
byte binOutB[4] = {outB1,outB2,outB3,outB4};
//3 pins out to anode channel switches
byte anodes[3] = {anode1,anode2,anode3};

const int fadeDur = 5; //ms - each multiplexed pair of digits appears for this amount of time per cycle
const int dimDur = 4; //ms - portion of fadeDur that is left dark during dim times
int fadeNextDur = 0; //ms - during fade, incoming digit's portion of fadeDur
int fadeLastDur = 0; //ms - during fade, outgoing digit's portion of fadeDur
unsigned long fadeStartLast = 0; //millis - when the last digit fade was started
unsigned long setStartLast = 0; //to control flashing during start

void initOutputs() {
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
  if(piezoPin>=0) pinMode(piezoPin, OUTPUT);
  if(relayPin>=0) {
    pinMode(relayPin, OUTPUT); digitalWrite(relayPin, HIGH); //LOW = device on
    quickBeep(); //"primes" the beeper, seems necessary when relay pin is spec'd, otherwise first intentional beep doesn't happen TODO still true?
  }
  if(ledPin>=0) pinMode(ledPin, OUTPUT);
  updateLEDs(); //set to initial value
}

void cycleDisplay(){
  unsigned long now = millis();
  
  //Other display code decides whether we should dim per function or time of day
  bool dim = (displayDim==1?1:0);
  //But if we're setting, decide here to dim for every other 500ms since we started setting
  if(fnSetPg>0) {
    if(setStartLast==0) setStartLast = now;
    dim = 1-(((unsigned long)(now-setStartLast)/500)%2);
  } else {
    if(setStartLast>0) setStartLast=0;
  }
  //TODO if we want to flash certain elements, we might do it similarly here
  
  fadeLastDur = fadeDur-(dim?dimDur:0); //by default, last digit displays for entire fadeDur minus dim time
  
  if(readEEPROM(20,false)==0 || dim) { //fading disabled or dim
    if(fadeStartLast) fadeStartLast = 0; //cancel any fade currently going - dim state doesn't have enough steps to fade well
    for(byte i=0; i<6; i++) if(displayNext[i] != displayLast[i]) displayLast[i] = displayNext[i];
  }
  else { //fading enabled
    if(fadeStartLast==0) { //not fading - time to fade?
      for(byte i=0; i<6; i++) if(displayNext[i] != displayLast[i]) { fadeStartLast = now; break; }
    }
    if(fadeStartLast!=0) { //currently fading
      //let the next digit steal some display time from the last digit
      //ex: if fade time (from EEPROM) is 20ms, and fadeDur (next+last) is 6ms:
      // at  0ms, next = (( 0*(6-1))/20)+1 = 1; last = (6-nextDur) = 5;
      // at 10ms, next = ((10*(6-1))/20)+1 = 3; last = (6-nextDur) = 3; ...
      // at 20ms, next = ((20*(6-1))/20)+1 = 6; next = total, so fade is over!
      //TODO facilitate longer fades by writing a tweening function that smooths the frames, i.e. 111121222 - or use delayMicroseconds as below
      //TODO does this have more problems with the millis rollover issue?
      fadeNextDur = (((unsigned long)(now-fadeStartLast)*(fadeDur-1))/(readEEPROM(20,false)*10))+1;
      if(fadeNextDur >= fadeLastDur) { //fade is over
        fadeStartLast = 0;
        fadeNextDur = 0;
        fadeLastDur = fadeDur;
        for(byte j=0; j<6; j++) displayLast[j] = displayNext[j];
      } //end fade is over
      else { //shorten last digit display duration by subtracting next display duration from it
        fadeLastDur = fadeLastDur - fadeNextDur;
      }
    } //end curently fading
  } //end fading enabled
  
  //TODO consider using delayMicroseconds() which, with its tighter resolution, may give better control over fades and dim levels
  if(displayDim>0) { //if other display code says to shut off entirely, skip this part
    //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
    setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
    digitalWrite(anodes[0], HIGH); //Turn on tubes
    delay(fadeLastDur);//-(dim?dimDur:0)); //Display for fade-out cycles
    setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
    delay(fadeNextDur);//-(dim?dimDur:0)); //Display for fade-in cycles
    digitalWrite(anodes[0], LOW); //Turn off tubes
  
    if(dim) delay(dimDur);
  
    //Anode channel 1: tubes #4 (sec x10) and #1 (hour x1)
    setCathodes(displayLast[4],displayLast[1]);
    digitalWrite(anodes[1], HIGH);
    delay(fadeLastDur);
    setCathodes(displayNext[4],displayNext[1]);
    delay(fadeNextDur);
    digitalWrite(anodes[1], LOW);
  
    if(dim) delay(dimDur);
  
    //Anode channel 2: tubes #0 (hour x10) and #3 (min x1)
    setCathodes(displayLast[0],displayLast[3]);
    digitalWrite(anodes[2], HIGH);
    delay(fadeLastDur);
    setCathodes(displayNext[0],displayNext[3]);
    delay(fadeNextDur);
    digitalWrite(anodes[2], LOW);
  
    if(dim) delay(dimDur);
  } //end if displayDim>0
  
} //end cycleDisplay()

void setCathodes(byte decValA, byte decValB){
  bool binVal[4]; //4-bit binary number with values [1,2,4,8]
  decToBin(binVal,decValA); //have binary value of decVal set into binVal
  for(byte i=0; i<4; i++) digitalWrite(binOutA[i],binVal[i]); //set bin inputs of SN74141
  decToBin(binVal,decValB);
  for(byte i=0; i<4; i++) digitalWrite(binOutB[i],binVal[i]); //set bin inputs of SN74141
} //end setCathodes()

void decToBin(bool binVal[], byte i){
  //binVal is a reference (modify in place) of a binary number bool[4] with values [1,2,4,8]
  if(i<0 || i>15) i=15; //default value, turns tubes off
  binVal[3] = int(i/8)%2;
  binVal[2] = int(i/4)%2;
  binVal[1] = int(i/2)%2;
  binVal[0] = i%2;
} //end decToBin()

void signalStart(byte sigFn, byte sigDur, word pulseDur){ //make some noise! or switch on an appliance!
  //sigFn isn't necessarily the current fn, just the one generating the signal
  //sigDur is the number of seconds to put on signalRemain, or 0 for a single immediate beep (skipped in radio mode).
  //Special case: if sigFn==fnIsAlarm, and sigDur>0, we'll use signalDur or switchDur as appropriate.
  //If doing a single beep, pulseDur is the number of ms it should last, or 0 for signal source's chosen output's pulse length (which will be used anyway if pulsed relay)
  signalSource = sigFn; //Set this first so signalStop won't inadvertently turn off a switched relay started by something else
  signalStop();
  if(sigDur==0) signalPulseStart(pulseDur); //single immediate beep
  else { //long-duration signal (alarm, sleep, etc)
    if(sigFn==fnIsAlarm) signalRemain = (readEEPROM(42,false)==1 && relayPin>=0 && relayMode==0 ? switchDur : signalDur);
    else signalRemain = sigDur;
    //piezo or pulsed relay: checkRTC will handle it
    if(getSignalOutput()==1 && relayPin>=0 && relayMode==0) { //switched relay: turn it on now
      digitalWrite(relayPin,LOW); //LOW = device on
      //Serial.print(millis(),DEC); Serial.println(F(" Relay on, signalStart"));
    }
  }
  updateLEDs(); //LEDs following signal or relay
}
void signalStop(){ //stop current signal and clear out signal timer if applicable
  signalRemain = 0; snoozeRemain = 0;
  signalPulseStop(); //piezo or pulsed relay: stop now
  if(getSignalOutput()==1 && relayPin>=0 && relayMode==0) { //switched relay: turn it off now
    digitalWrite(relayPin,HIGH); //LOW = device on
    //Serial.print(millis(),DEC); Serial.println(F(" Relay off, signalStop"));
  }
  updateLEDs(); //LEDs following relay
}
//beep start and stop should only be called by signalStart/signalStop and checkRTC
void signalPulseStart(word pulseDur){
  //Only act if the signal source output is pulsable
  if(getSignalOutput()==0 && piezoPin>=0) { //beeper
    tone(piezoPin, getSignalPitch(), (pulseDur==0?piezoPulse:pulseDur));
  }
  else if(getSignalOutput()==1 && relayPin>=0 && relayMode==1) { //pulsed relay
    digitalWrite(relayPin,LOW); //LOW = device on
    //Serial.print(millis(),DEC); Serial.println(F(" Relay on, signalPulseStart"));
    signalPulseStartTime = millis();
  }
}
void signalPulseStop(){
  if(getSignalOutput()==0 && piezoPin>=0) { //beeper
    noTone(piezoPin);
  }
  else if(getSignalOutput()==1 && relayPin>=0 && relayMode==1) { //pulsed relay
    digitalWrite(relayPin,HIGH); //LOW = device on
    //Serial.print(millis(),DEC); Serial.println(F(" Relay off, signalPulseStop"));
    signalPulseStartTime = 0;
  }
}
word getSignalPitch(){ //for current signal: time, timer, or (default) alarm
  return getHz(readEEPROM((signalSource==fnIsTime?41:(signalSource==fnIsTimer?40:39)),false));
}
word getHz(byte note){
  //Given a piano key note, return frequency
  char relnote = note-49; //signed, relative to concert A
  float reloct = relnote/12.0; //signed
  word mult = 440*pow(2,reloct);
  return mult;
}
char getSignalOutput(){ //for current signal: time, timer, or (default) alarm: 0=piezo, 1=relay
  return readEEPROM((signalSource==fnIsTime?44:(signalSource==fnIsTimer?43:42)),false);
}
void quickBeep(){
  //Short beeper signal at alarm pitch (if equipped), even if relay is switched. Used when silencing snooze or switching alarm on.
  if(getSignalOutput()==1 && relayMode==0) { if(piezoPin>=0) { signalSource=fnIsAlarm; tone(piezoPin, getSignalPitch(), 100); }}
  else signalStart(fnIsAlarm,0,100);
}

const byte ledFadeStep = 10; //fade speed – with every loop() we'll increment/decrement the LED brightness (between 0-255) by this amount
byte ledStateNow = 0;
byte ledStateTarget = 0;
void updateLEDs(){
  //Run whenever something is changed that might affect the LED state: initial (initOutputs), signal start/stop, relay on/off, setting change
  if(ledPin>=0) {
    switch(readEEPROM(26,false)){
      case 0: //always off
        ledStateTarget = 0;
        //Serial.println(F("LEDs off always"));
        break;
      case 1: //always on
        ledStateTarget = 255;
        //Serial.println(F("LEDs on always"));
        break;
      case 2: //on, but follow night/away modes
        ledStateTarget = (displayDim==2? 255: (displayDim==1? 127: 0));
        //Serial.print(displayDim==2? F("LEDs on"): (displayDim==1? F("LEDs dim"): F("LEDs off"))); Serial.println(F(" per dim state"));
        break;
      case 3: //off, but on when alarm/timer sounds
        ledStateTarget = (signalRemain && (signalSource==fnIsAlarm || signalSource==fnIsTimer)? 255: 0);
        //Serial.print(signalRemain && (signalSource==fnIsAlarm || signalSource==fnIsTimer)?F("LEDs on"):F("LEDs off")); Serial.println(F(" per alarm/timer"));
        break;
      case 4: //off, but on with switched relay
        if(relayPin>=0 && relayMode==0) {
          ledStateTarget = (!digitalRead(relayPin)? 255: 0); //LOW = device on
          //Serial.print(!digitalRead(relayPin)? F("LEDs on"): F("LEDs off")); Serial.println(F(" per switched relay"));
        }
        break;
      default: break;
    } //end switch
  } //if ledPin
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
    analogWrite(ledPin,ledStateNow);
  }
}