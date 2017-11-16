// Code for Arduino Nano in RLB Designs IN-12/17 clock v5.0
// featuring timekeeping by DS1307 RTC and six digits multiplexed 3x2 via two SN74141 driver chips
// Originally written by Robin Birtles and Chris Gerekos based on http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt
// Refactored and overwrought by Luke McKenzie (luke@theclockspot.com)

// TODO: Rotary encoders with velocity
// TODO: Flesh out menu - make it loop
// TODO: Date - display, set
// TODO: Alarm - display, set, sound, snooze, 24h silence
// TODO: Timer - display, set, run, sound, silence
// TODO: fnLast timeout when on a mode other than clock or running timer, 5sec - and when setting, 120sec. What checks this?
// TODO: Cathode anti-poisoning


////////// Configuration consts //////////

// These are the RLB board connections to Arduino analog input pins.
// S1/PL13 = Reset
// S2/PL5 = A1
// S3/PL6 = A0
// S4/PL7 = A6
// S5/PL8 = A3
// S6/PL9 = A2
// S7/PL14 = A7
// A6-A7 are analog-only pins that aren't as responsive and require a physical pullup resistor (1K to +5V).

// What input is associated with each control?
const byte mainSel = A2; //main select button - must be equipped
const byte mainAdjA = A1; //main up/down buttons or rotary encoder - must be equipped
const byte mainAdjB = A0;
const byte altSel = 0; //alt select button - if unequipped, set to 0
const byte altAdjA = A6; //alt up/down buttons or rotary encoder - if unequipped, set to 0
const byte altAdjB = A3;

// What type of adj controls are equipped?
// 1 = momentary buttons. 2 = quadrature rotary encoder. 
const byte mainAdjType = 1;
const byte altAdjType = 1; //if unquipped, set to 0

// In normal running mode, what do the controls do?
// 255 = nothing, 254 = cycle through functions TODO, 0-3 = go to specific function (see fn)
const byte mainSelFn = 255; //1; //date
const byte mainAdjFn = 254;
const byte altSelFn = 255; //3; //timer //if equipped
const byte altAdjFn = 255; //if equipped

const byte enableSoftAlarmSwitch = 1;
// 1 = yes. Use if using the integrated beeper or another non-switched device (bell solenoid, etc).
// 0 = no. Use if the connected alarm device has its own switch (e.g. clock radio function switch).
//     Alarm will be permanently on in software.
const byte alarmRadio = 0;
// 0 = no. Alarm output is connected to the onboard piezoelectric beeper or similar signal device.
//     When alarm and timer go off, it will output a beep pattern for alarmDur minutes.
// 1 = yes. Alarm output is connected to a relay to switch other equipment (like a radio).
//     When alarm goes off, output will stay on for alarmDur minutes (120 is common).
//     When timer is running, output will stay on until timer runs down.
const byte alarmDur = 3;

// How long (in ms) are the button hold durations?
const int btnShortHold = 1000; //for setting the displayed feataure
const int btnLongHold = 3000; //for for entering SETUP menu


////////// Function prototypes, global consts and vars //////////

// Hardware inputs
unsigned long inputSampleLast = 0; //millis() of last time inputs (and RTC) were sampled
const byte inputSampleDur = 50; //input sampling frequency (in ms) to avoid bounce
byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
unsigned long btnCurStart = 0; //When the current button was pressed / knob was last turned  TODO convert to inputStart
byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()

void initInputs(); //Set pinModes for inputs; capture initial state of knobs (rotary encoders, if equipped).
void checkInputs(); //Run at sample rate. Calls checkBtn() for each eligible button and ctrlEvt() for each knob event.
void checkBtn(byte btn); //Calls ctrlEvt() for each button event
//void checkRot(); TODO do we need to watch the rotation direction to accommodate for inputs changing faster than we sample?
bool readInput(byte pin); //Does analog or digital read depending on which type of pin it is.
void btnStop(); //Stops further events from a button until it's released, to prevent unintended behavior after an event is handled.

// Input handling and value setting
const byte fnCt = 2; //number of functions in the clock
byte fn = 0; //currently displayed function: 0=time, 1=date, 2=alarm, 3=timer, 255=SETUP menu
byte fnSec = 0; //seconds since the current fn or set state was entered, for timeout purposes
byte fnSet = 0; //whether this function is currently being set, and which option/page it's on
int fnSetVal; //the value currently being set, if any
int fnSetValMin; //min possible
int fnSetValMax; //max possible
bool fnSetValVel; //whether it supports velocity setting (if max-min > 30)
int fnSetValDate[3]; //holder for newly set date, so we can set it in 3 stages but set the RTC only once
const byte alarmTimeLoc = 0; //EEPROM loc. In minutes past midnight.
const byte alarmOnLoc = 1; //EEPROM loc
bool alarmSounding = 0; //also used for timer expiry TODO what happens if they are concurrent?
unsigned int snoozeTime = 0; //seconds
unsigned int timerTime = 0; //seconds - up to just under 18 hours
//Setup menu options - see readme for details
const byte optsLoc = 2; //EEPROM loc of first option. The rest are at optsLoc + (option number - 1)
const byte optsCt = 18; //number of options
//         Option number: 1  2  3  4  5  6  7  8  9  10 11   12   13 14 15 16   17   18
const int  optsDef[18] = {2, 1, 0, 0, 0, 0, 0, 0, 0,500, 0,1320, 360, 0, 1, 5, 480,1020};
const int  optsMin[18] = {1, 1, 0, 0, 0, 0, 0, 0, 0,  1, 0,   0,   0, 0, 0, 0,   0,   0};
const int  optsMax[18] = {2, 2, 2, 1,50, 1, 6, 4,60,999, 2,1439,1439, 2, 6, 6,1439,1439};

void initEEPROM(bool force=false); //Sets EEPROM to defaults, either on first run or when forced - see setup()
void ctrlEvt(byte ctrl, byte evt); //Handles an input control event, based on current fn and set state.
void startSet(int n, int m, int x, byte p=1); //Enter set state (at optional page p), and start setting value n.
void startOpt(int n); //Calls startSet for a given setup menu option.
void clearSet(); //Exit set state.
void doSet(int delta); //Does the actual value setting. Ensures range isn't exceeded. Updates display.
unsigned long doSetHoldLast;
void doSetHold(); //Run at sample rate per doSetHoldLast. Fires doSet commands in response to held buttons.
byte daysInMonth(int y, int m);

// Clock ticking and timed event triggering
unsigned int rtcPollLast = 0; //maybe don't poll the RTC every loop? would that be good?
byte rtcSecLast = 61;
void checkRTC(); //Run at sample rate. When clock ticks, updates display, inc/decrements timers.

// Display formatting
byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start. Change this to change tubes.

// Hardware outputs
//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {2,3,4,5};
byte binOutB[4] = {6,7,8,9};
//3 pins out to anode channel switches
byte anodes[3] = {11,12,13};
void initOutputs();
float fadeMax = 5.0f;
float fadeStep = 1.0f;
int displayLast[6]={11,11,11,11,11,11}; //What is currently being displayed. We slowly fade away from this.
float displayNextFade[6]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f}; //Fading in displayNext values
float displayLastFade[6]={8.0f,8.0f,8.0f,8.0f,8.0f,8.0f}; //Fading out displayLast values
unsigned long setStartLast = 0; //to control flashing
void cycleDisplay(); //Run on every "clock cycle" to keep multiplexing going.
void setCathodes(int decValA, int decValB);
void decToBin(bool binVal[], int i);

//Misc
byte debug = 0; //debug value TODO remove this when done with it


////////// Includes and main code control //////////

#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 rtc;

void setup(){
  //Serial.begin(57600);
  Wire.begin();
  rtc.begin();
  if(!rtc.isrunning()) rtc.adjust(DateTime(2017,1,1,0,0,0)); //TODO test
  initOutputs();
  initInputs();
  initEEPROM(readInput(mainSel)==LOW?true:false); //Defaults restored if mainSel is held on startup //TODO test
  checkRTC();
  updateDisplay(); //initial fill of data
}

void loop(){
  //Things done every "clock cycle"
  checkRTC(); //if clock has ticked, decrement timer if running, and updateDisplay
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  doSetHold(); //if inputs have been held, this will do more things + updateDisplay as needed
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
}


////////// Control inputs //////////
//bool mainRotLast[2] = {0,0}; //last state of main rotary encoder inputs TODO implement
//bool altRotLast[2] = {0,0}; //and alt (if equipped)
void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  //4 and 5 used for I2C
  pinMode(A6, INPUT); digitalWrite(A6, HIGH);
  pinMode(A7, INPUT); digitalWrite(A7, HIGH);
  //If using rotary encoders, capture their initial state
  // if(mainAdjType==2) checkRot(mainAdj,mainAdjRotLast,true);
  // if(altAdjType==2) checkRot(altAdj,altAdjRotLast,true);
}

void checkInputs(){
  // TODO can all this if/else business be defined at load instead of evaluated every sample?
  if(millis() >= inputSampleLast+inputSampleDur) { //time for a sample
    inputSampleLast = millis();
    //potential issue: if user only means to rotate or push encoder but does both?
    checkBtn(mainSel); //main select
    //if(mainAdjType==2) checkRot(mainAdj,mainAdjRotLast,true); //main rotary encoder
    //else {
    checkBtn(mainAdjA); checkBtn(mainAdjB);//main adj buttons
    //}
    //if(altSel!=0) checkBtn(altSel); //alt select (if equipped)
    //if(altAdjType==2) checkRot(altAdj,altAdjRotLast,true); //alt rotary encoder (if equipped)
    //else
    //if(altAdjType==1) { checkBtn(altAdjA); checkBtn(altAdjB); } //alt adj buttons
  } //end if time for a sample
}

// void checkRot(rot[],last[],triggerEvent){
//   bool n = readInput(rot[0]);
//
//   if(mainAdjType==1) { //main
//     bool new0 = readInput(mainAdjA);
//     if(new0 != mainAdjRotLast[0]) { rotAction(0,(
//     mainAdj,mainAdjRotLast);
//   }
//
//   rotLast[0]=readInput(inputs[1]); rotLast[1]=readInput(inputs[2]);
// }

void checkBtn(byte btn){
  //Changes in momentary buttons, LOW = pressed.
  //When a button event has occurred, will call ctrlEvt(btn,evt)
  //evt: 0=pressed, 1=released, 2=held past med point, 3=held past long point
  bool bnow = readInput(btn);
  //If the button has just been pressed, and no other buttons are in use...
  if(btnCur==0 && bnow==LOW) {
    btnCur = btn; btnCurHeld = 0; btnCurStart = millis();
    //log("Btn "); log(String(btn)); log(" has been pressed\n");
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(btnCur==btn && bnow==LOW) {
    if(millis() >= btnCurStart+btnLongHold && btnCurHeld < 3) {
      btnCurHeld = 3;
      //log("Btn "); log(String(btn)); log(" has been long-held\n");
      ctrlEvt(btn,3); //hey, the button has been held past the long point
    }
    else if(millis() >= btnCurStart+btnShortHold && btnCurHeld < 2) {
      btnCurHeld = 2;
      //log("Btn "); log(String(btn)); log(" has been short-held\n");
      ctrlEvt(btn,2); //hey, the button has been held past the med point
    }
  }
  //If the button has just been released...
  if(btnCur==btn && bnow==HIGH) {
    btnCur = 0;
    //log("Btn "); log(String(btn)); log(" has been released\n\n");
    if(btnCurHeld < 4) ctrlEvt(btn,0); //hey, the button was released
    btnCurHeld = 0;
  }
}
bool readInput(byte pin){
  if(pin==A6 || pin==A7) return analogRead(pin)<100?0:1; //analog-only pins
  else return digitalRead(pin);
}
void btnStop(){
  //In some cases, when handling btn evt 1/2/3, we may call this
  //so following events 2/3/0 won't cause unintended behavior
  //(e.g. after a fn change, or going in or out of set)
  btnCurHeld = 4;
}


////////// Input handling and value setting //////////

void initEEPROM(bool force){
  //Set EEPROM to defaults. Done if force==true
  //or if the first setup menu option has a value of 0 (e.g. first run)
  if(force || EEPROM.read(optsLoc)==0) {
    for(byte i=0; i<optsCt; i++) { EEPROM.write(optsLoc+i,optsDef[i]); } //setup menu //debug++;
    EEPROM.write(alarmTimeLoc,420); //7am
    EEPROM.write(alarmOnLoc,0); //off
  }
}

void ctrlEvt(byte ctrl, byte evt){  
  //Handle button-like events, based on current fn and set state.
  //Currently only main, not alt... TODO
  bool mainSelPush = (ctrl==mainSel && evt==1?1:0);
  bool mainSelHold = (ctrl==mainSel && evt==2?1:0);
  bool mainAdjUpPush = (ctrl==mainAdjA && evt==1?1:0);
  bool mainAdjDnPush = (ctrl==mainAdjB && evt==1?1:0);
  
  if(fn!=255 && ctrl==mainSel && evt==3) { //joker mainSel long hold: enter SETUP menu
    //Serial.print(F("debug is ")); Serial.println(debug,DEC);
    btnStop(); fn=255; startOpt(1); return;
  }
  
  DateTime d = rtc.now();
  if(fn!=255) { //normal fn running/setting
    if(!fnSet) { //fn running
      if(mainAdjUpPush) fn=(fn==fnCt-1?0:fn+1);
      if(mainAdjDnPush) fn=(fn==0?fnCt-1:fn-1);
      if(mainSelHold) { //enter fnSet
        //btnStop(); prevents joker
        switch(fn){
          case 0: startSet((d.hour()*60)+d.minute(),0,1439,1); break; //time: set mins
          case 1: fnSetValDate[1]=d.month(), fnSetValDate[2]=d.day(); startSet(d.year(),0,32767,1); break; //date: set year
          case 2: //alarm: set mins
          case 3: //timer: set mins
          default: break;
        }
        return;
      }
      if(mainSelPush) { //mainSel press: do nothing TODO
        //btnPresses++; log("Incrementing counter to "); log(String(btnPresses)); log("\n"); updateDisplay(); return;
        return;
      }
    } //end fn running
    else { //fn setting
      if(mainAdjUpPush) doSet(1);
      if(mainAdjDnPush) doSet(-1);
      //change (adj hold will be handled by doSetHold(), but here we'll need press velocity to treat encoder changes like very rapid button presses - use btnCurStart and disregard rot inputs if button is pressed)
      //no mainSelHold will happen here
      if(mainSelPush) { //go to next option or save and exit fnSet
        btnStop();
        switch(fn){
          case 0: //date: save in RTC
            rtc.adjust(DateTime(d.year(),d.month(),d.day(),fnSetVal/60,fnSetVal%60,0));
            clearSet(); break;
          case 1: switch(fnSet){
            case 1: //date: save year, set month
              fnSetValDate[0]=fnSetVal;
              startSet(fnSetValDate[1],1,12,2); break; 
            case 2: //date: save month, set date
              fnSetValDate[1]=fnSetVal;
              startSet(fnSetValDate[2],1,daysInMonth(fnSetValDate[0],fnSetValDate[1]),3); break;
            case 3: //date: save in RTC
              rtc.adjust(DateTime(fnSetValDate[0],fnSetValDate[1],fnSetVal,d.hour(),d.minute(),d.second()));
              clearSet(); break;
            default: break;
          } break;
          case 2: //alarm
          case 3: //timer
          default: break;
        } //end switch fn
      } //end mainSelPush
    } //end fn setting
  } //end normal fn running/setting
  else { //setup menu setting
    if(mainSelPush) {
      if(fnSet==optsCt) { //that was the last one – rotate back around
        //log("SETUP: Exiting after last option\n");
        startOpt(1); return;
      } else {
        //log("SETUP: Proceeding to next option\n");
        startOpt(fnSet++); return; //advance to next option
      }
    }
    if(mainSelHold) {
      //log("SETUP: Exiting per mainSel short hold\n");
      btnStop(); fn=0; clearSet(); return; //exit setup
    }
    if(mainAdjUpPush) doSet(1);
    if(mainAdjDnPush) doSet(-1);
  }//end setup
}

void startSet(int n, int m, int x, byte p=1){ //Enter set state at page p, and start setting a value
  fnSetVal=n; fnSetValMin=m; fnSetValMax=x; fnSetValVel=(x-m>30?1:0); fnSet=p;
  updateDisplay();
}

void startOpt(int n){ //Call startSet for a given setup menu option (1-index)
  startSet(EEPROM.read(optsLoc),optsMin[0],optsMax[0],1);
}

void clearSet(){ startSet(0,0,0,0); } //Exit set state

void doSet(int delta){
  //Does actual setting of fnSetVal, as told to by ctrlEvt or doSetHold, within params of fnSetVal vars
  if(delta>0) if(fnSetValMax-fnSetVal<delta) fnSetVal=fnSetValMax; else fnSetVal=fnSetVal+delta;
  if(delta<0) if(fnSetVal-fnSetValMin<abs(delta)) fnSetVal=fnSetValMin; else fnSetVal=fnSetVal+delta;
  updateDisplay();
}

void doSetHold(){
  //When we're setting via an adj button that's passed a hold point, fire off doSet commands at intervals
  if(doSetHoldLast+250<millis()) { //TODO is it a problem this won't sync up with the blinking?
    doSetHoldLast = millis();
    if(fnSet!=0 && ((mainAdjType==1 && (btnCur==mainAdjA || btnCur==mainAdjB)) || (altAdjType==1 && (btnCur==altAdjA || btnCur==altAdjB))) ){ //if we're setting, and this is an adj input for which the type is button
      bool dir = (btnCur==mainAdjA || btnCur==altAdjA ? 1 : 0);
      //If short hold, or long hold but high velocity isn't supported, use low velocity (delta=1)
      if(btnCurHeld==2 || (btnCurHeld==3 && fnSetValVel==false)) doSet(dir?1:-1);
      //else if long hold, use high velocity (delta=10)
      else if(btnCurHeld==3) doSet(dir?10:-10);
    }
  }
}

//TODO which is better - speed of reading from this array or storage of 
//const byte daysInMonth[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
byte daysInMonth(int y, int m){
  if(m==2) return (y%4==0 && (y%100!=0 || y%400==0) ? 29 : 28);
  //https://cmcenroe.me/2014/12/05/days-in-month-formula.html
  else return (28 + ((m + (m/8)) % 2) + (2 % m) + (2 * (1/m)));
}


////////// Clock ticking and timed event triggering //////////
void checkRTC(){
  //TODO why are the digits flashing before changing? I've got some blocking code happening? is it here?
  //If in fn time or timer, updates display when RTC time changes. Also decrements running timer.
  if(rtcPollLast<millis()+50) { //check every 1/20th of a second
    rtcPollLast=millis();
    DateTime now = rtc.now();
    if(rtcSecLast != now.second()) {
      //decrement timer?
      if(fnSet==0 && fn==0){ //display clock directly
        byte hr = now.hour();
        if(EEPROM.read(optsLoc+1)==1) hr = (hr==0?12:(hr>12?hr-12:hr));
        editDisplay(hr, 0, 1, EEPROM.read(optsLoc+4));
        editDisplay(now.minute(), 2, 3, true);
        editDisplay(now.second(), 4, 5, true);
      }
    } 
  }
}


////////// Display data formatting //////////
void updateDisplay(){
  //Run as needed to update display when the value being shown on it has changed
  //(Ticking time and date are an exception - see checkRTC() )
  //This formats the new value and puts it in displayNext[] for cycleDisplay() to pick up
  switch(fn){ //which function are we displaying?
    case 255: //SETUP menu 
    //log("Per SETUP, updating display to key="); log(String(fnSet)); log(", val="); log(String(opts[fnSet])); log("\n");
      editDisplay(fnSet, 4, 5, false); //current option key, displayed on little tubes (4-5)
      editDisplay(fnSetVal, 0, 3, (fnSetValMax==1439?EEPROM.read(optsLoc+3):false)); //current option value, on big tubes (0-3) - if a time, use leading zeros if enabled
      break;
    case 1: //date - running display taken care of by checkRTC()
      if(fnSet) {
        //Stopping place: make date set; make checkRTC display normal clock and date
      }
    break;
    case 2: //alarm
      if(fnSet) {
        editDisplay(fnSetVal/60, 0, 1, EEPROM.read(optsLoc+4));
        editDisplay(fnSetVal%60, 2, 3, true);
        blankDisplay(4, 5);
      }
      else {
        editDisplay(EEPROM.read(alarmTimeLoc)/60, 0, 1, EEPROM.read(optsLoc+4));
        editDisplay(EEPROM.read(alarmTimeLoc)%60, 2, 3, true);
        editDisplay(EEPROM.read(alarmOnLoc),4,5,false);
      }
    break;
    case 3: //timer
    break;
    default: //time - running display taken care of by checkRTC()
      if(fnSet){
        editDisplay(fnSetVal/60, 0, 1, EEPROM.read(optsLoc+4));
        editDisplay(fnSetVal%60, 2, 3, true);
        blankDisplay(4, 5);
      }
      break;
  } //end switch fn
} //end updateDisplay()

void editDisplay(int n, byte posStart, byte posEnd, bool leadingZeros){
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank tube
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4 tubes --> 0015)
  int place;
  for(int i=0; i<=posEnd-posStart; i++){
    //place = int(pow(10,i)); TODO PROBLEM: int(pow(10,2))==99 and int(pow(10,3))==999. Why??????????
    switch(i){
      case 0: place=1; break;
      case 1: place=10; break;
      case 2: place=100; break;
      case 3: place=1000; break;
      default: break;
    }
    displayNext[posEnd-i] = (i==0&&n==0 ? 0 : (n>=place ? (n/place)%10 : (leadingZeros?0:15)));
    //log("In "); logint(place); log("s place at display position "); logint(posEnd-i); log(", we put "); if(displayNext[posEnd-i]==15) log("a blank"); else logint(displayNext[posEnd-i]); log(".\n"); //log(" n/place="); logint(n); log("/"); logint(place); log("="); logint(n/place); log("...(n/place)%10="); logint((n/place)%10); log("\n");
  }
  //log("After editDisplay(), displayNext is {");
  //for(byte i=0; i<=5; i++) { if(i!=0){log(",");} if(displayNext[i]==15) log("-"); else logint(displayNext[i]); }
  //log("}\n");
} //end editDisplay()
void blankDisplay(byte posStart, byte posEnd){
  for(byte i=posStart; i<=posEnd; i++) displayNext[i]=15;
} //end blankDisplay();


////////// Hardware outputs //////////
void initOutputs() {
  for(byte i=0; i<4; i++) pinMode(binOutA[i],OUTPUT);
  for(byte i=0; i<4; i++) pinMode(binOutB[i],OUTPUT);
  for(byte i=0; i<3; i++) pinMode(anodes[i],OUTPUT);
  pinMode(10, OUTPUT); //Alarm signal pin
}

void cycleDisplay(){
  bool dim = 0;//(opts[2]>0?true:false); //Under normal circumstances, dim constantly if the time is right
  if(fnSet>0) { //but if we're setting, dim for every other 500ms since we started setting
    if(setStartLast==0) setStartLast = millis();
    dim = 1-(((millis()-setStartLast)/500)%2);
  } else {
    if(setStartLast>0) setStartLast=0;
  }
  
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
  
  // Loop thru and update all the arrays, and fades.
  for( int i = 0 ; i < 6 ; i ++ ) {
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

void setCathodes(int decValA, int decValB){
  bool binVal[4]; //4-bit binary number with values [1,2,4,8]
  decToBin(binVal,decValA); //have binary value of decVal set into binVal
  for(byte i=0; i<4; i++) digitalWrite(binOutA[i],binVal[i]); //set bin inputs of SN74141
  decToBin(binVal,decValB);
  for(byte i=0; i<4; i++) digitalWrite(binOutB[i],binVal[i]); //set bin inputs of SN74141
} //end setCathodes()

void decToBin(bool binVal[], int i){
  //binVal is a reference (modify in place) of a binary number bool[4] with values [1,2,4,8]
  if(i<0 || i>15) i=15; //default value, turns tubes off
  binVal[3] = int(i/8)%2;
  binVal[2] = int(i/4)%2;
  binVal[1] = int(i/2)%2;
  binVal[0] = i%2;
} //end decToBin()