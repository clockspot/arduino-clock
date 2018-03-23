// Code for Arduino Nano in RLB Designs IN-12/17 clock v5.0
// featuring timekeeping by DS1307 RTC and six digits multiplexed 3x2 via two SN74141 driver chips
// Originally written by Robin Birtles and Chris Gerekos based on http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt
// Refactored and expanded by Luke McKenzie (luke@theclockspot.com)

// TODO: Timer - display, set, run, sound, silence - make non-volatile?
// TODO: Cathode anti-poisoning
// TODO: implement other setup options

// notes on variable sizes on atmega
// char = int8_t, -128 to 127 (negatives stored via 2's complement).
// byte = uint8_t, 0 to 255
// int  = int16_t, -32768 to 32767. Watch out for bitshift right operator
// word = uint16_t, 0 to 65535 (or unsigned int)
// long = int32_t, -2147483648 to 2147483647.
// float, but use integer math when you can
// integer constant modifiers: use leading B for binary, 0 for octal, 0x for hex; trailing U for unsigned and/or L for long

#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
#include <ooPinChangeInt.h>
#include <AdaEncoder.h>


////////// Configuration consts //////////

// available clock functions, and unique IDs (between 0 and 200)
const byte fnIsTime = 0;
const byte fnIsDate = 1;
const byte fnIsAlarm = 2;
const byte fnIsTimer = 3;
const byte fnIsDayCount = 4;
const byte fnIsTemp = 5;
const byte fnIsTubeTester = 6; //cycles all digits on all tubes 1/second, similar to anti-cathode-poisoning cleaner
// functions enabled in this clock, in their display order. Only fnIsTime is required
const byte fnsEnabled[] = {fnIsTime, fnIsDate, fnIsAlarm, fnIsTimer, fnIsDayCount}; //, fnIsTemp, fnIsTubeTester

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
const byte mainSel = A2; //main select button - must be equipped
const byte mainAdjUp = A1; //main up/down buttons or rotary encoder - must be equipped
const byte mainAdjDn = A0;
const byte altSel = 0; //alt select button - if unequipped, set to 0
const byte altAdjUp = 0; //A6; //alt up/down buttons or rotary encoder - if unequipped, set to 0
const byte altAdjDn = 0; //A3;

// What type of adj controls are equipped?
// 1 = momentary buttons. 2 = quadrature rotary encoder.
const byte mainAdjType = 1;
const byte altAdjType = 0; //if unquipped, set to 0

// In normal running mode, what do the controls do?
// assign -1 = nothing, -2 = cycle through functions, or a specific function per above
// TODO, one of them needs to do nothing for the alarm switch, right?
const char mainSelFn = -2;
const char mainAdjFn = -1;
// const byte altSelFn = -1;
// const byte altAdjFn = -1;

const byte enableSoftAlarmSwitch = 1;
// 1 = yes. Use if using the integrated piezo or another non-switched device (bell solenoid, etc).
// 0 = no. Use if the connected alarm device has its own switch (e.g. clock radio function switch).
//     Alarm will be permanently on in software.
const byte signalPin = 10;
const byte signalType = 0;
//What is the signal pin connected to?
// 0 = Piezo. When alarm and timer go off, it will output a beep pattern with tone() for signalDur minutes.
// 1 = Signal relay TODO. Same as above, but it will simply switch the pin, for e.g. a solenoid.
// 2 = Radio relay TODO.
//     When alarm goes off, output will stay on for signalDur minutes (120 is common).
//     When timer is running, output will stay on until timer runs down.
const word signalBeepDur = 500; //With signalType 0/1, beeps happen once per second; how long is each beep in ms?
const byte signalDur = 1; //When alarm goes off (and timer, with signalType 0/1), how many mins does signal run for?

const byte unoffDur = 10; //when display is dim/off, a press will light the tubes for this many seconds

const byte displaySize = 6; //number of tubes

// How long (in ms) are the button hold durations?
const word btnShortHold = 1000; //for setting the displayed feataure
const word btnLongHold = 3000; //for for entering options menu
const byte velThreshold = 150; //ms
// When an adj up/down input (btn or rot) follows another in less than this time, value will change more (10 vs 1).
// Recommend ~150 for rotaries. If you want to use this feature with buttons, extend to ~400.


////////// Other global consts and vars //////////

/*EEPROM locations
These should be permanently associated with a value (unless deprecated and reused).
We could name them all with consts for coding convenience, but for now just using directly in code.
Most values in the clock are 1-byte; if 2-byte, high byte is loc, low byte is loc+1.
These ones are set outside the options menu - defaults defined in initEEPROM()
  0-1 Alarm time, mins
  2 Alarm on
  3-4 Day count year
  5 Day count month
  6 Day count date
( 7-15 are available )
These ones are set inside the options menu - defaults defined in arrays below
  16 Time format
  17 Date format 
  18 Display date during time
  19 Leading zero in hour, date, and month
  20 Digit fade duration TODO
  21 Hourly strike TODO
  22 Auto DST
  23 Alarm days
  24 Alarm snooze
  25 Timer interval mode TODO
( 26 is available )
  27 Night-off TODO
  28-29 Night start, mins TODO
  30-31 Night end, mins TODO
  32 Day-off TODO
  33 First day of workweek
  34 Last day of workweek
  35-36 Work starts at, mins TODO
  37-38 Work ends at, mins TODO
  39 Alarm tone pitch TODO
  40 Timer tone pitch TODO
  41 Hourly strike pitch TODO
*/

//Options menu options' EEPROM locations and default/min/max values. Numbers (and pos in these arrays) 
//Although these arrays are 0-index, the option number displayed (and listed in readme) is 1-index. (see "fn-fnOpts+1")
//Options' numbers may be changed by reordering these arrays (and changing readme accordingly).
//        Option number: 1  2  3  4  5  6  7  8  9 10 11 12 13 14   15   16 17 18 19   20   21
const byte optsLoc[] = {16,17,18,19,20,22,23,24,39,25,40,21,41,27,  28,  30,32,33,34,  35,  37};
const word optsDef[] = { 2, 1, 0, 0, 0, 0, 0, 0,61, 0,61, 0,61, 0,1320, 360, 0, 1, 5, 480,1020};
const word optsMin[] = { 1, 1, 0, 0, 0, 0, 0, 0,49, 0,49, 0,49, 0,   0,   0, 0, 0, 0,   0,   0};
const word optsMax[] = { 2, 5, 2, 1,50, 6, 2,60,88, 1,88, 4,88, 2,1439,1439, 2, 6, 6,1439,1439};

//RTC objects
DS3231 ds3231; //an object to access the ds3231 specifically (temp, etc)
RTClib rtc; //an object to access a snapshot of the ds3231 rtc via now()
DateTime tod; //stores the now() snapshot for several functions to use
byte toddow; //stores the day of week as calculated from tod

// Hardware inputs and value setting
byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()
unsigned long inputLast = 0; //When a button was last pressed
unsigned long inputLast2 = 0; //Second-to-last of above

const byte fnOpts = 201; //fn values from here to 255 correspond to options in the options menu
byte fn = fnIsTime; //currently displayed fn, as above
byte fnSetPg = 0; //whether this function is currently being set, and which option/page it's on
word fnSetVal; //the value currently being set, if any - unsigned int 0-65535
word fnSetValMin; //min possible - unsigned int
word fnSetValMax; //max possible - unsigned int
bool fnSetValVel; //whether it supports velocity setting (if max-min > 30)
word fnSetValDate[3]; //holder for newly set date, so we can set it in 3 stages but set the RTC only once

// Volatile running values
word signalRemain = 0; //alarm/timer signal timeout counter, seconds
word signalPitch = 440; //which pitch to use - set by what started the signal going
word snoozeRemain = 0; //snooze timeout counter, seconds
word timerInitial = 0; //timer original setting, seconds - up to 18 hours (64,800 seconds - fits just inside a word)
word timerRemain = 0; //timer actual counter
word unoffRemain = 0; //un-off (briefly turn on tubes during full night-off or day-off) timeout counter, seconds
byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start. Change this to change tubes.
byte displayDim = 2; //dim per display or function: 2=normal, 1=dim, 0=off
byte cleanRemain = 11; //anti-cathode-poisoning clean timeout counter, increments at cleanSpeed ms (see loop()). Start at 11 to run at clock startup
word cleanSpeed = 200; //ms


////////// Main code control //////////

void setup(){
  Serial.begin(9600);
  Wire.begin();
  initOutputs();
  initInputs();
  if(readInput(mainSel)==LOW) initEEPROM();
}

unsigned long pollLast = 0; //every 50ms
unsigned long pollCleanLast = 0; //every cleanSpeed ms
void loop(){
  unsigned long now = millis();
  //Handle tube cleaning. A special case since it runs at a speed outside the normal cycles as below
  if(cleanRemain) {
    if(pollCleanLast+cleanSpeed<now) {
      pollCleanLast=now;
      cleanRemain--;
      updateDisplay();
    }
  }
  //Things done every 50ms - avoids overpolling(?) and switch bounce(?)
  if(pollLast+20<now) {
    pollLast=now;
    checkRTC(false); //if clock has ticked, decrement timer if running, and updateDisplay
    checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
    doSetHold(); //if inputs have been held, this will do more things + updateDisplay as needed
  }
  //Things done every loop cycle
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
}


////////// Control inputs //////////
void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  //4 and 5 used for I2C
  pinMode(A6, INPUT); digitalWrite(A6, HIGH);
  pinMode(A7, INPUT); digitalWrite(A7, HIGH);
  //rotary encoder init
  if(mainAdjType==2) AdaEncoder mainRot = AdaEncoder('a',mainAdjUp,mainAdjDn);
  //if(altAdjType==2) AdaEncoder altRot = AdaEncoder('b',altAdjUp,altAdjDn);
}

void checkInputs(){
  // TODO can all this if/else business be defined at load instead of evaluated every sample? OR is it compiled that way?
  //potential issue: if user only means to rotate or push encoder but does both?

  //sample button states
  checkBtn(mainSel); //main select
  if(mainAdjType==1) { checkBtn(mainAdjUp); checkBtn(mainAdjDn); } //main adjust buttons (if equipped)
  //if(altSel!=0) checkBtn(altSel); //alt select (if equipped)
  //if(altAdjType==1) { checkBtn(altAdjUp); checkBtn(altAdjDn); } //alt adjust buttons (if equipped)
  
  //check adaencoder library to see if rot(s) have moved
  if(mainAdjType==2 || altAdjType==2) checkRot();
}

bool readInput(byte pin){
  if(pin==A6 || pin==A7) return analogRead(pin)<100?0:1; //analog-only pins
  else return digitalRead(pin);
}
void checkBtn(byte btn){
  //Changes in momentary buttons, LOW = pressed.
  //When a button event has occurred, will call ctrlEvt
  bool bnow = readInput(btn);
  //If the button has just been pressed, and no other buttons are in use...
  if(btnCur==0 && bnow==LOW) {
    btnCur = btn; btnCurHeld = 0; inputLast2 = inputLast; inputLast = millis();
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(btnCur==btn && bnow==LOW) {
    if(millis() >= inputLast+btnLongHold && btnCurHeld < 3) {
      btnCurHeld = 3;
      ctrlEvt(btn,3); //hey, the button has been long-held
    }
    else if(millis() >= inputLast+btnShortHold && btnCurHeld < 2) {
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
  //In some cases, when handling btn evt 1/2/3, we may call this
  //so following events 2/3/0 won't cause unintended behavior
  //(e.g. after a fn change, or going in or out of set)
  btnCurHeld = 4;
}

void checkRot(){
  //Changes in rotary encoders. When rotation(s) occur, will call ctrlEvt to simulate btn presses.
  if(btnCur==0) {
    //TODO does this work with more than one encoder? maybe on separate loops?
    //https://github.com/GreyGnome/AdaEncoder/blob/master/Examples/MyEncoder/MyEncoder.ino
    AdaEncoder *thisEncoder=NULL;
    thisEncoder = AdaEncoder::genie();
    if(thisEncoder!=NULL) {
      unsigned long inputThis = millis();
      if(inputThis-inputLast < 70) return; //ignore inputs that come faster than a human could rotate
      int8_t clicks = thisEncoder->query(); //signed number of clicks it has moved
      byte dir = (clicks<0?0:1);
      clicks = abs(clicks);
      for(byte i=0; i<clicks; i++){ //in case of more than one click
        ctrlEvt((thisEncoder->getID()=='a'?(dir?mainAdjUp:mainAdjDn):(dir?altAdjUp:altAdjDn)),1);
      }
      inputLast2 = inputLast; inputLast = inputThis;
    }
  }
}//end checkRot


////////// Input handling and value setting //////////

bool stoppingSignal = false; //Special stuff (snooze canceling) happens right after a press that silences the signal
void ctrlEvt(byte ctrl, byte evt){
  //Handle control events (from checkBtn or checkRot), based on current fn and set state.
  //evt: 1=press, 2=short hold, 3=long hold, 0=release.
  //We only handle press evts for adj ctrls, as that's the only evt encoders generate.
  //But we can handle short and long holds and releases for the sel ctrls (always buttons).
  //TODO needs alt handling
  
  //Before all else, is it a press to stop the signal? Silence it
  if(signalRemain>0 && evt==1){
    stoppingSignal = true;
    signalRemain = 0;
    if(signalType==0) noTone(signalPin);
    //If we're displaying the clock (as alarm trigger does), start snooze. 0 will have no effect. TODO chime may trigger this also?
    if(fn==fnIsTime) snoozeRemain = readEEPROM(24,false)*60;
    return;
  }
  //After pressing to silence, short hold cancels a snooze; ignore other btn evts
  if(stoppingSignal){
    stoppingSignal = false;
    if(evt==2 && snoozeRemain>0) {
      snoozeRemain = 0;
      //A short beep at alarm pitch. Use signalBeepDur or 100ms, whichever is smaller
      if(signalType==0) tone(signalPin, getHz(readEEPROM(39,false)), (signalBeepDur<100?signalBeepDur:100));
    }
    btnStop();
    return;
  }
  
  //Is it a press for an un-off?
  if(displayDim==0 && evt==1) {
    unoffRemain = unoffDur;
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
          case fnIsDate: //set year
            fnSetValDate[1]=tod.month(), fnSetValDate[2]=tod.day(); startSet(tod.year(),0,9999,1); break;
          case fnIsAlarm: //set mins
            startSet(readEEPROM(0,true),0,1439,1); break;
          case fnIsTimer: //set mins, up to 18 hours (64,800 seconds - fits just inside a word)
            if(timerRemain>0) { timerRemain = 0; btnStop(); updateDisplay(); break; } //If the timer is running, zero it out.
            startSet(timerInitial/60,0,1080,1); break;
          case fnIsDayCount: //set year like date, but from eeprom like startOpt
            startSet(readEEPROM(3,true),2000,9999,1); break;
          case fnIsTemp:
            break; //nothing - or is this where we do the calibration? TODO
          default: break;
        }
        //showSitch();
        return;
      }
      else if((ctrl==mainSel && evt==0) || ((ctrl==mainAdjUp || ctrl==mainAdjDn) && evt==1)) { //sel release or adj press - switch fn, depending on config
        //-1 = nothing, -2 = cycle through functions, other = go to specific function (see fn)
        //we can't handle sel press here because, if attempting to enter setting mode, it would switch the fn first
        bool fnChgd = false;
        if(ctrl==mainSel){
          if(mainSelFn!=-1) { //do a function switch
            fnChgd = true;
            if(mainSelFn==-2) fnScroll(1); //Go to next fn in the cycle
            else fn = mainSelFn;
          } else if(fn==fnIsAlarm) switchAlarm(0); //switch alarm
        }
        else if(ctrl==mainAdjUp || ctrl==mainAdjDn) {
          if(mainAdjFn!=-1) { //do a function switch
            fnChgd = true;
            if(mainAdjFn==-2) fnScroll(ctrl==mainAdjUp?1:-1); //Go to next or previous fn in the cycle
            else fn = mainAdjFn;
          } else if(fn==fnIsAlarm) switchAlarm(ctrl==mainAdjUp?1:-1); //switch alarm
        }
        if(fnChgd){
          switch(fn){
            //static ones
            case fnIsAlarm: case fnIsTemp:
              updateDisplay(); break;
            //"ticking" ones
            default: checkRTC(true); break;
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
            case fnIsDate: //save in RTC
              switch(fnSetPg){
                case 1: //save year, set month
                  fnSetValDate[0]=fnSetVal;
                  startSet(fnSetValDate[1],1,12,2); break; 
                case 2: //save month, set date
                  fnSetValDate[1]=fnSetVal;
                  startSet(fnSetValDate[2],1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3); break;
                case 3: //write year/month/date to RTC
                  ds3231.setYear(fnSetValDate[0]%100); //TODO: do we store century on our end? Per ds3231 docs, "The century bit (bit 7 of the month register) is toggled when the years register overflows from 99 to 00."
                  ds3231.setMonth(fnSetValDate[1]);
                  ds3231.setDate(fnSetVal);
                  ds3231.setDoW(dayOfWeek(fnSetValDate[0],fnSetValDate[1],fnSetVal)+1); //ds3231 weekday is 1-index
                  clearSet(); break;
                default: break;
              } break;
            case fnIsAlarm:
              writeEEPROM(0,fnSetVal,true);
              clearSet(); break;
            case fnIsTimer: //timer
              timerInitial = fnSetVal*60; //timerRemain is seconds, but timer is set by minute
              timerRemain = timerInitial; //set actual timer going
              clearSet(); break;
            case fnIsDayCount: //set like date, save in eeprom like finishOpt
              switch(fnSetPg){
                case 1: //save year, set month
                  writeEEPROM(3,fnSetVal,true);
                  startSet(readEEPROM(5,false),1,12,2); break;
                case 2: //save month, set date
                  writeEEPROM(5,fnSetVal,false);
                  startSet(readEEPROM(6,false),1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3); break;
                case 3: //save date
                  writeEEPROM(6,fnSetVal,false);
                  clearSet(); break;
                default: break;
              } break;
              break;
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
    
    byte opt = fn-fnOpts; //current option number, 0-index (for opts[] arrays)
    
    if(evt==2 && ctrl==mainSel) { //mainSel short hold: exit options menu
      btnStop();
      //if we were setting a value, writes setting val to EEPROM if needed
      if(fnSetPg) writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]-optsMin[opt]>255?true:false);
      fn = fnIsTime;
      clearSet();
      return;
    }
    
    if(!fnSetPg){ //viewing option number
      if(ctrl==mainSel && evt==0) { //mainSel release: enter option value setting
        startSet(readEEPROM(optsLoc[opt],optsMax[opt]-optsMin[opt]>255?true:false),optsMin[opt],optsMax[opt],1);
      }
      if(ctrl==mainAdjUp && evt==1) fnOptScroll(1); //next one up or cycle to beginning
      if(ctrl==mainAdjDn && evt==1) fnOptScroll(-1); //next one down or cycle to end?
      updateDisplay();
    } //end viewing option number

    else { //setting option value
      if(ctrl==mainSel && evt==0) { //mainSel release: save and exit option value setting
        //Writes setting val to EEPROM if needed
        writeEEPROM(optsLoc[opt],fnSetVal,optsMax[opt]-optsMin[opt]>255?true:false);
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
}

void startSet(word n, word m, word x, byte p){ //Enter set state at page p, and start setting a value
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
  if(doSetHoldLast+250<millis()) {
    doSetHoldLast = millis();
    if(fnSetPg!=0 && ((mainAdjType==1 && (btnCur==mainAdjUp || btnCur==mainAdjDn)) || (altAdjType==1 && (btnCur==altAdjUp || btnCur==altAdjDn))) ){ //if we're setting, and this is an adj input for which the type is button
      bool dir = (btnCur==mainAdjUp || btnCur==altAdjUp ? 1 : 0);
      //If short hold, or long hold but high velocity isn't supported, use low velocity (delta=1)
      if(btnCurHeld==2 || (btnCurHeld==3 && fnSetValVel==false)) doSet(dir?1:-1);
      //else if long hold, use high velocity (delta=10)
      else if(btnCurHeld==3) doSet(dir?10:-10);
    }
  }
}
void clearSet(){ //Exit set state
  startSet(0,0,0,0);
  checkRTC(true); //force an update to tod and updateDisplay()
}

//EEPROM values are exclusively bytes (0-255) or words (unsigned ints, 0-65535)
//If it's a word, high byte is in loc, low byte is in loc+1
void initEEPROM(){
  //Set EEPROM and clock to defaults
  //First prevent the held button from doing anything else
  btnCur = mainSel; btnStop();
  //Set the clock
  ds3231.setYear(18);
  ds3231.setMonth(1);
  ds3231.setDate(1);
  ds3231.setDoW(1); //2018-01-01 is Monday. DS3231 will keep count from here
  ds3231.setHour(0);
  ds3231.setMinute(0);
  ds3231.setSecond(0);
  //Set the default values that aren't part of the options menu
  writeEEPROM(0,420,true); //7am - word
  writeEEPROM(2,enableSoftAlarmSwitch==0?1:0,false); //off, or on if no software switch spec'd
  writeEEPROM(3,2018,true);
  writeEEPROM(5,1,false);
  writeEEPROM(6,1,false);
  //then the options menu defaults
  for(byte opt=0; opt<sizeof(optsLoc); opt++) writeEEPROM(optsLoc[opt],optsDef[opt],optsMax[opt]-optsMin[opt]>255?true:false);
}
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
void writeEEPROM(int loc, int val, bool isWord){
  if(isWord) {
    Serial.print(F("EEPROM write word:"));
    Serial.print(F(" loc ")); Serial.print(loc,DEC);
    if(EEPROM.read(loc)!=highByte(val)) {
      EEPROM.write(loc,highByte(val));
      Serial.print(F(" val ")); Serial.print(highByte(val),DEC);
    } else Serial.print(F(" unchanged (no write)."));
    Serial.print(F(" loc ")); Serial.print(loc+1,DEC);
    if(EEPROM.read(loc+1)!=lowByte(val)) {
      EEPROM.write(loc+1,lowByte(val));
      Serial.print(F(" val ")); Serial.print(lowByte(val),DEC);
    } else Serial.print(F(" unchanged (no write)."));
  } else {
    Serial.print(F("EEPROM write byte:"));
    Serial.print(F(" loc ")); Serial.print(loc,DEC);
    if(EEPROM.read(loc)!=val) { EEPROM.write(loc,val);
      Serial.print(F(" val ")); Serial.print(val,DEC);
    } else Serial.print(F(" unchanged (no write)."));
  }
  Serial.println();
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
  
  //Things to do every time this is called: timeouts to reset display. These may force a tick.
  if(pollLast > inputLast){ //don't bother if the last input (which may have called checkRTC) was more recent than poll
    //Option/setting timeout: if we're in the options menu, or we're setting a value
    if(fnSetPg || fn>=fnOpts){
      if(pollLast-inputLast>120000) { fnSetPg = 0; fn = fnIsTime; force=true; } //Time out after 2 mins
    }
    //Temporary-display mode timeout: if we're *not* in a permanent one (time, day counter, tester, or running/signaling timer)
    else if(fn!=fnIsTime && fn!=fnIsTubeTester && fn!=fnIsDayCount && !(fn==fnIsTimer && (timerRemain>0 || signalRemain>0))){
      if(pollLast>inputLast+5000) { fnSetPg = 0; fn = fnIsTime; force=true; }
    }
  }
  
  //Update things based on RTC
  tod = rtc.now();
  toddow = ds3231.getDoW()-1; //ds3231 weekday is 1-index
  
  if(rtcSecLast != tod.second() || force) { //If it's a new RTC second, or we are forcing it
    
    if(force && rtcSecLast != tod.second()) force=false; //in the odd case it's BOTH, recognize the natural second
    rtcSecLast = tod.second();
    
    //Things to do at specific times
    if(tod.second()==0) { //at top of minute
      //at 2am, check for DST change
      if(tod.minute()==0 && tod.hour()==2) autoDST();
      //check if we should trigger the alarm - if the time is right and the alarm is on...
      if(tod.hour()*60+tod.minute()==readEEPROM(0,true) && readEEPROM(2,false)) {
        if(readEEPROM(23,false)==0 || //any day of the week
          (readEEPROM(23,false)==1 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),toddow)) || //weekday only
          (readEEPROM(23,false)==2 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),toddow)) ) { //weekend only
            fnSetPg = 0; fn = fnIsTime; signalPitch = getHz(readEEPROM(39,false)); signalRemain = signalDur*60;
        } //end toddow check
      } //end alarm trigger
      //check if we should trigger the cleaner (at night end time, or alarm time if night end is 0:00)
      if(tod.hour()*60+tod.minute()==(readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true))) {
        cleanRemain = 11; //loop() will pick this up
      } //end cleaner check
      //
    }
    if(tod.second()==30 && fn==fnIsTime && fnSetPg==0 && unoffRemain==0) { //At bottom of minute, maybe show date - not when unoffing
      if(readEEPROM(18,false)==2) { fn = fnIsDate; inputLast = pollLast; }
    }
    
    //Strikes - only if fn=clock, not setting, not night-off/day-off, and appropriate signal type.
    //Short pips before the top of the hour
    if(tod.minute()==59 && tod.second()>=55 && readEEPROM(21,false)==2 && signalType<2 && signalRemain==0 && snoozeRemain==0 && fn==fnIsTime && fnSetPg==0 && displayDim==2) {
      signalPitch = getHz(readEEPROM(41,false));
      tone(signalPin, signalPitch, (signalType==0?100:signalBeepDur));
    }
    //Strikes on/after the hour
    if(tod.second()==0 && (tod.minute()==0 || tod.minute()==30) && signalType<2 && signalRemain==0 && snoozeRemain==0 && fn==fnIsTime && fnSetPg==0 && displayDim==2){
      signalPitch = getHz(readEEPROM(41,false));
      byte hr; hr = tod.hour(); hr = (hr==0?12:(hr>12?hr-12:hr));
      switch(readEEPROM(21,false)) {
        case 1: //single beep via normal signal cycle
          if(tod.minute()==0) signalRemain = 1; break;
        case 2: //long pip
          if(tod.minute()==0) tone(signalPin, signalPitch, (signalType==0?500:signalBeepDur)); break;
        case 3: //hour strike via normal signal cycle
          if(tod.minute()==0) signalRemain = hr; break;
        case 4: //ship's bell at :00 and :30 mins
          signalRemain = ((hr%4)*2)+(tod.minute()==30?1:0); break;
        default: break;
      } //end strike type
    } //end strike
    
    //Things to do every natural second (decrementing real-time counters)
    if(!force) {
      //If timer has time on it, decrement and trigger beeper if we reach zero
      if(timerRemain>0) {
        timerRemain--;
        if(timerRemain<=0) { //timer has elasped
          signalPitch = getHz(readEEPROM(40,false));
          if(readEEPROM(25,false)) { //interval timer: a short signal and restart; don't change to timer fn
            signalRemain = 1; timerRemain = timerInitial;
          } else {
            fnSetPg = 0; fn = fnIsTimer; inputLast = pollLast; signalRemain = signalDur*60;
          }
          //TODO radio mode
        } //end timer elapsed
      }
      //If alarm snooze has time on it, decrement and trigger beeper if we reach zero (and alarm is still on)
      if(snoozeRemain>0) {
        snoozeRemain--;
        if(snoozeRemain<=0 && readEEPROM(2,false)) {
          fnSetPg = 0; fn = fnIsTime; signalPitch = getHz(readEEPROM(39,false)); signalRemain = signalDur*60;
        }
      }
      //If signal has time on it, decrement and make a beep
      if(signalRemain>0) {
        signalRemain--;
        if(signalType==0) tone(signalPin, signalPitch, signalBeepDur);
      }
      if(unoffRemain>0) {
        unoffRemain--; //updateDisplay will naturally put it back to off state if applicable
      }
    } //end natural second
    
    //Finally, whether natural tick or not, if we're not setting anything, update the display.
    //This also determines night-off/day-off, which is why chimes will happen if we go into off at top of hour TODO find a way to fix this
    if(fnSetPg==0) updateDisplay();
    
  } //end if force or new second
} //end checkRTC()

bool fellBack = false;
void autoDST(){
  //Call daily when clock reaches 2am.
  //If rule matches, will set to 3am in spring, 1am in fall (and set fellBack so it only happens once)
  if(fellBack) { fellBack=false; return; } //If we fell back at last 2am, do nothing.
  if(toddow==0) { //is it sunday? currently all these rules fire on Sundays only
    switch(readEEPROM(22,false)){
      case 1: //second Sunday in March to first Sunday in November (US/CA)
        if(tod.month()==3 && tod.day()>=8 && tod.day()<=14) setDST(1);
        if(tod.month()==11 && tod.day()<=7) setDST(-1);
        break;
      case 2: //last Sunday in March to last Sunday in October (UK/EU)
        if(tod.month()==3 && tod.day()>=25) setDST(1);
        if(tod.month()==10 && tod.day()>=25) setDST(-1);
        break;
      case 3: //first Sunday in April to last Sunday in October (MX)
        if(tod.month()==4 && tod.day()<=7) setDST(1);
        if(tod.month()==10 && tod.day()>=25) setDST(-1);
        break;
      case 4: //last Sunday in September to first Sunday in April (NZ)
        if(tod.month()==9 && tod.day()>=24) setDST(1); //30 days hath September: last Sun will be 24-30
        if(tod.month()==4 && tod.day()<=7) setDST(-1);
        break;
      case 5: //first Sunday in October to first Sunday in April (AU)
        if(tod.month()==10 && tod.day()<=7) setDST(1);
        if(tod.month()==4 && tod.day()<=7) setDST(-1);
        break;
      case 6: //third Sunday in October to third Sunday in February (BZ)
        if(tod.month()==10 && tod.day()>=15 && tod.day()<=21) setDST(1);
        if(tod.month()==2 && tod.day()>=15 && tod.day()<=21) setDST(-1);
        break;
      default: break;
    } //end setting switch
  } //end is it sunday
}
void setDST(char dir){
  if(dir==1) ds3231.setHour(3); //could set relatively if we move away from 2am-only sets
  if(dir==-1 && !fellBack) { ds3231.setHour(1); fellBack=true; }
}

void switchAlarm(char dir){
  if(enableSoftAlarmSwitch){
    if(dir==1) writeEEPROM(2,1,false);
    if(dir==-1) writeEEPROM(2,0,false);
    if(dir==0) writeEEPROM(2,!readEEPROM(2,false),false);
    snoozeRemain = 0;
    updateDisplay();
  }
}

word getHz(byte note){
  //Given a piano key note, return frequency
  char relnote = note-49; //signed, relative to concert A
  float reloct = relnote/12.0; //signed
  word mult = 440*pow(2,reloct);
  return mult;
}
//Sometimes we activate the beeper directly (e.g. pips) but most of the time we make a bell tone TODO
// char curBellToneLevel; //support negative just in case
// word curBellTonePitch;
// void cycleBellTone(){
//   //Called by loop() 50ms polling – make bell tone decay
//   //An Arduino pin directly to a piezo element/buzzer is not very good. The piezo is like a capacitor and you need a resistor of 100 ohm to reduce the peak currents.
//   if(curBellToneLevel > 0) {
//     curBellToneLevel *= 0.9; //decay by 1/10 each time
//     if(curBellToneLevel<=0) {
//       currBelToneLevel = 0; //TODO stop tone fully
//     } else {
//       //TODO make the bell this level - whatever it was doing before
//     }
//   }
// }

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
void updateDisplay(){
  //Run as needed to update display when the value being shown on it has changed
  //This formats the new value and puts it in displayNext[] for cycleDisplay() to pick up
  if(cleanRemain) { //cleaning tubes
    displayDim = 2;
    byte digit = (11-cleanRemain)%10;
    editDisplay(digit,0,0,true);
    editDisplay(digit,1,1,true);
    editDisplay(digit,2,2,true);
    editDisplay(digit,3,3,true);
    editDisplay(digit,4,4,true);
    editDisplay(digit,5,5,true);
  }
  else if(fnSetPg) { //setting value, for either fn or option
    displayDim = 2;
    blankDisplay(4, 5);
    if(fnSetValMax==1439) { //Time of day (0-1439 mins, 0:00–23:59): show hrs/mins
      editDisplay(fnSetVal/60, 0, 1, readEEPROM(19,false)); //hours with leading zero
      editDisplay(fnSetVal%60, 2, 3, true);
    } else if(fnSetValMax==1080) { //Timer duration (0-1080 mins, up to 18:00): show hrs/mins w/minimal leading
      if(fnSetVal>=60) editDisplay(fnSetVal/60, 0, 1, false); else blankDisplay(0,1); //hour only if present, else blank
      editDisplay(fnSetVal%60, 2, 3, (fnSetVal>=60?true:false)); //leading zero only if hour present
      editDisplay(0,4,5,true); //placeholder seconds
    } else if(fnSetValMax==88) { //A piezo pitch. Play a short demo beep.
      editDisplay(fnSetVal, 0, 3, false);
      if(signalType==0) tone(signalPin, getHz(fnSetVal), (signalBeepDur<100?signalBeepDur:100));
    } else editDisplay(fnSetVal, 0, 3, false); //some other type of value
  }
  else if(fn >= fnOpts){ //options menu, but not setting a value
    displayDim = 2;
    editDisplay(fn-fnOpts+1,0,1,0); //display option number (1-index) on hour tubes
    blankDisplay(2,5);
  }
  else { //fn running
    
    //Set displayDim per night-off and day-off settings - fnIsAlarm may override this
    //issue: moving from off alarm to next fn briefly shows alarm in full brightness. I think because of the display delays. TODO
    word todmins = tod.hour()*60+tod.minute();
    //In order of precedence:
    //temporary unoff
    if(unoffRemain > 0) displayDim = 2;
    //day-off on weekends, all day
    else if( readEEPROM(32,false)==1 && !isDayInRange(readEEPROM(33,false),readEEPROM(34,false),toddow) ) displayDim = 0;
    //day-off on weekdays, during office hours only
    else if( readEEPROM(32,false)==2 && isDayInRange(readEEPROM(33,false),readEEPROM(34,false),toddow) && isTimeInRange(readEEPROM(35,true), readEEPROM(37,true), todmins) ) displayDim = 0;
    //night-off - if night end is 0:00, use alarm time instead
    else if( readEEPROM(27,false) && isTimeInRange(readEEPROM(28,true), (readEEPROM(30,true)==0?readEEPROM(0,true):readEEPROM(30,true)), todmins) ) displayDim = (readEEPROM(27,false)==1?1:0); //dim or off
    //normal
    else displayDim = 2;
    
    switch(fn){
      case fnIsTime:
        byte hr; hr = tod.hour();
        if(readEEPROM(16,false)==1) hr = (hr==0?12:(hr>12?hr-12:hr));
        editDisplay(hr, 0, 1, readEEPROM(19,false));
        editDisplay(tod.minute(), 2, 3, true);
        if(readEEPROM(18,false)==1) editDisplay(tod.day(), 4, 5, readEEPROM(19,false)); //date
        else editDisplay(tod.second(), 4, 5, true); //seconds
        break;
      case fnIsDate:
        byte df; df = readEEPROM(17,false); //1=m/d/w, 2=d/m/w, 3=m/d/y, 4=d/m/y, 5=y/m/d
        if(df<=4) {
          editDisplay((df==1||df==3?tod.month():tod.day()),0,1,readEEPROM(19,false)); //month or date first
          editDisplay((df==1||df==3?tod.day():tod.month()),2,3,readEEPROM(19,false)); //date or month second
          editDisplay((df<=2?toddow:tod.year()),4,5,(df<=2?false:true)); //dow or year third - dow never leading zero, year always
        }
        else { //df==5
          editDisplay(tod.year(),0,1,true); //year always has leading zero
          editDisplay(tod.month(),2,3,readEEPROM(19,false));
          editDisplay(tod.day(),4,5,readEEPROM(19,false));
        }
        break;
      case fnIsDayCount:
        long targetDayCount; targetDayCount = dateToDayCount(
          readEEPROM(3,true),
          readEEPROM(5,false),
          readEEPROM(6,false)
        );
        long currentDayCount; currentDayCount = dateToDayCount(tod.year(),tod.month(),tod.day());
        editDisplay(abs(targetDayCount-currentDayCount),0,3,false);
        //TODO for now don't indicate negative. Elsewhere we use leading zeros to represent negative but I don't like how that looks here
        blankDisplay(4,5);
        break;
      case fnIsAlarm: //alarm
        word almTime; almTime = readEEPROM(0,true);
        editDisplay(almTime/60, 0, 1, readEEPROM(19,false)); //hours with leading zero
        editDisplay(almTime%60, 2, 3, true);
        editDisplay(readEEPROM(2,false),4,4,false); //status 1/0
        displayDim = (readEEPROM(2,false)?2:1); //status bright/dim
        blankDisplay(5,5);
        break;
      case fnIsTimer: //timer - display time left.
        //Relative unit positioning: when t <1h, display min/sec in place of hr/min on 4-tube displays
        byte mspos; mspos = (displaySize<6 && timerRemain<3600? 0 : 2);
        if(timerRemain >= 3600) { //t >=1h: display hr on first two tubes
          editDisplay(timerRemain/3600,0,1,false);
        } else blankDisplay(0,1);
        if(timerRemain >= 60) { //t >=1m: display minute (in relative position). Leading zero if t >=1h.
          editDisplay((timerRemain%3600)/60,mspos,mspos+1,(timerRemain>=3600?true:false));
        } else blankDisplay(mspos,mspos+1);
        //display second (in relative position). Leading zero if t>=1m.
        editDisplay(timerRemain%60,mspos+2,mspos+3,(timerRemain>=60?true:false));
        break;
      case fnIsTemp: //thermometer
        int temp; temp = ds3231.getTemperature()*100;
        editDisplay(abs(temp)/100,1,3,(temp<0?true:false)); //leading zeros if negative
        editDisplay(abs(temp)%100,4,5,true);
        break;
      case fnIsTubeTester:
        editDisplay(tod.second(),0,0,true);
        editDisplay(tod.second(),1,1,true);
        editDisplay(tod.second(),2,2,true);
        editDisplay(tod.second(),3,3,true);
        editDisplay(tod.second(),4,4,true);
        editDisplay(tod.second(),5,5,true);
      default: break;
    }//end switch
  }
} //end updateDisplay()

void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros){
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank tube
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4 tubes --> 0015)
  word place;
  for(byte i=0; i<=posEnd-posStart; i++){
    //place = int(pow(10,i)); TODO PROBLEM: int(pow(10,2))==99 and int(pow(10,3))==999. Why??????????
    switch(i){
      case 0: place=1; break;
      case 1: place=10; break;
      case 2: place=100; break;
      case 3: place=1000; break;
      default: break;
    }
    displayNext[posEnd-i] = (i==0&&n==0 ? 0 : (n>=place ? (n/place)%10 : (leadingZeros?0:15)));
  }
} //end editDisplay()
void blankDisplay(byte posStart, byte posEnd){
  for(byte i=posStart; i<=posEnd; i++) displayNext[i]=15;
} //end blankDisplay();


////////// Hardware outputs //////////
//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {2,3,4,5};
byte binOutB[4] = {6,7,8,9};
//3 pins out to anode channel switches
byte anodes[3] = {11,12,13};

float fadeMax = 5.0f;
float fadeStep = 1.0f;
int displayLast[6]={11,11,11,11,11,11}; //What is currently being displayed. We slowly fade away from this.
float displayNextFade[6]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f}; //Fading in displayNext values
float displayLastFade[6]={8.0f,8.0f,8.0f,8.0f,8.0f,8.0f}; //Fading out displayLast values
unsigned long setStartLast = 0; //to control flashing

void initOutputs() {
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
  pinMode(10, OUTPUT); //Alarm signal pin
}

void cycleDisplay(){
  //Other display code decides whether we should dim per function or time of day
  bool dim = (displayDim==1?1:0);
  //But if we're setting, decide here to dim for every other 500ms since we started setting
  if(fnSetPg>0) {
    if(setStartLast==0) setStartLast = millis();
    dim = 1-(((millis()-setStartLast)/500)%2);
  } else {
    if(setStartLast>0) setStartLast=0;
  }
  
  if(displayDim>0) { //if other display code says to shut off entirely, skip this part
    //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
    setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
    digitalWrite(anodes[0], HIGH); //Turn on tubes
    delay(displayLastFade[0]/(dim?4:1)); //Display for fade-out cycles
    setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
    delay(displayNextFade[0]/(dim?4:1)); //Display for fade-in cycles
    digitalWrite(anodes[0], LOW); //Turn off tubes
  
    if(dim) delay(fadeMax/1.5);
  
    //Anode channel 1: tubes #4 (sec x10) and #1 (hour x1)
    setCathodes(displayLast[4],displayLast[1]);
    digitalWrite(anodes[1], HIGH);
    delay(displayLastFade[1]/(dim?4:1));
    setCathodes(displayNext[4],displayNext[1]);
    delay(displayNextFade[1]/(dim?4:1));
    digitalWrite(anodes[1], LOW);
  
    if(dim) delay(fadeMax/1.5);
  
    //Anode channel 2: tubes #0 (hour x10) and #3 (min x1)
    setCathodes(displayLast[0],displayLast[3]);
    digitalWrite(anodes[2], HIGH);
    delay(displayLastFade[2]/(dim?4:1));
    setCathodes(displayNext[0],displayNext[3]);
    delay(displayNextFade[2]/(dim?4:1));
    digitalWrite(anodes[2], LOW);
  
    if(dim) delay(fadeMax*0.75);
  } //end if displayDim !== 0
  
  // Loop thru and update all the arrays, and fades.
  for( byte i = 0 ; i < 6 ; i ++ ) {
    if( displayNext[i] != displayLast[i] ) {
      displayNextFade[i] += fadeStep;
      displayLastFade[i] -= fadeStep;
  
      if( displayNextFade[i] >= fadeMax ){
        displayNextFade[i] = 0.0f;
        displayLastFade[i] = fadeMax;
        displayLast[i] = displayNext[i];
      }
    }
  }
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