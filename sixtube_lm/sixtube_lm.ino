// Code for Arduino Nano in RLB Designs IN-12/17 clock v5.0
// featuring timekeeping by DS1307 RTC and six digits multiplexed 3x2 via two SN74141 driver chips
// Originally written by Robin Birtles and Chris Gerekos based on http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt
// Refactored and expanded by Luke McKenzie (luke@theclockspot.com)


////////// Configuration consts //////////

// These are the RLB board connections to Arduino analog input pins
// (inited HIGH; connection to ground (LOW) is a button press)
// A0-A3 are analog pins that support digitalRead()
// A4-A5 are reserved for I2C communication with the RTC
// A6-A7 are analog-only pins that require analogRead(). To use, install a physical pull-up resistor (1K to +5V).
// S1/PL13 = Reset
// S2/PL5 = A1
// S3/PL6 = A0
// S4/PL7 = A6
// S5/PL8 = A3
// S6/PL9 = A2
// S7/PL14 = A7

// What input is associated with each control?
const byte mainSel = A2; //main select button - must be equipped //TODO test this with A6
const byte mainAdj[2] = {A1,A0}; //main up/down buttons or rotary encoder - must be equipped
const byte altSel = 0; //alt select button - if unequipped, set to 0
const byte altAdj[2] = {A6,A3}; //alt up/down buttons or rotary encoder - if unequipped, need not define this

// What type of adj controls are equipped?
// 1 = momentary buttons. 2 = quadrature rotary encoder. 
const byte mainAdjType = 1;
const byte altAdjType = 1; //if unquipped, set to 0

// In normal running mode, what do the controls do?
// 255 = nothing, 254 = cycle through functions, 0-3 = go to specific function (see fn)
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


////////// Major global vars/consts //////////
int rtcLast[6] = {0,0,0,0,0,-1}; //y,m,d,h,i,s. -1 to force initial set
byte fnCt = 4; //number of functions in the clock
byte fn = 0; //currently displayed function: 0=time, 1=date, 2=alarm, 3=timer, 255=SETUP menu
byte fnSet = 0; //whether this function is currently being set, and which option/page it's on
//parameters of value currently being set, if any
int fnSetVal; //the value currently being set, if any
int fnSetValMin; //min possible
int fnSetValMax; //max possible
bool fnSetValVel; //whether it supports velocity setting
//Values we could be setting:
//Time (mins): 0–1439, supports velocity
//Year: 2000–32767, supports velocity
//Month: 1-12, no velocity
//Date: 1-[max for this month], no velocity
//or some other number
byte setupOptsCt = 5; //number of options in the Setup menu, 1-index
byte setupOpts[6] = {0,2,1,0,0,42}; //values of options – see "setup opts details" for what these do

byte displayNext[6] = {15,15,15,15,15,15}; //Blank tubes at start. When display should change, put it here
//int btnPresses = 0;


////////// Includes and main code control //////////

//#include <EEPROM.h>
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
  checkRTC();
  updateDisplay(); //initial fill of data
}

void loop(){
  //Things done every "clock cycle"
  checkRTC(); //if clock has ticked, decrement timer if running, and updateDisplay
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  fnSetHoldCheck(); //if inputs have been held, this will do more things + updateDisplay as needed
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
}


////////// Control inputs //////////
unsigned long inputSampleLast = 0; //millis() of last time inputs were sampled
int inputSampleDur = 50; //input sampling frequency (in ms) to avoid bounce
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
    checkBtn(mainAdj[0]); checkBtn(mainAdj[1]);//main adj buttons
    //}
    //if(altSel!=0) checkBtn(altSel); //alt select (if equipped)
    //if(altAdjType==2) checkRot(altAdj,altAdjRotLast,true); //alt rotary encoder (if equipped)
    //else
    //if(altAdjType==1) { checkBtn(altAdj[0]); checkBtn(altAdj[1]); } //alt adj buttons
  } //end if time for a sample
}

// void checkRot(rot[],last[],triggerEvent){
//   bool n = readInput(rot[0]);
//
//   if(mainAdjType==1) { //main
//     bool new0 = readInput(mainAdj[0]);
//     if(new0 != mainAdjRotLast[0]) { rotAction(0,(
//     mainAdj,mainAdjRotLast);
//   }
//
//   rotLast[0]=readInput(inputs[1]); rotLast[1]=readInput(inputs[2]);
// }

byte btnCur = 0; //momentary button currently in use - only one allowed at a time
unsigned long btnCurStart = 0; //when the current button was pressed
byte btnCurHeld = 0; //hold thresholds passed: 0=none, 1=reserved TODO, 2=short, 3=long,
//4=mute further actions from this button until after it's released
void checkBtn(byte btn){
  //Changes in momentary buttons, LOW = pressed.
  //When a button event has occurred, will call ctrlEvt(btn,evt)
  //evt: 0=pressed, 1=released, 2=held past med point, 3=held past long point
  bool bnow = readInput(btn);
  //If the button has just been pressed, and no other buttons are in use...
  if(btnCur==0 && bnow==LOW) {
    btnCur = btn; btnCurHeld = 0; btnCurStart = millis();
    log("Btn "); log(String(btn)); log(" has been pressed\n");
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(btnCur==btn && bnow==LOW) {
    if(millis() >= btnCurStart+btnLongHold && btnCurHeld < 3) {
      btnCurHeld = 3;
      log("Btn "); log(String(btn)); log(" has been long-held\n");
      ctrlEvt(btn,3); //hey, the button has been held past the long point
    }
    else if(millis() >= btnCurStart+btnShortHold && btnCurHeld < 2) {
      btnCurHeld = 2;
      log("Btn "); log(String(btn)); log(" has been short-held\n");
      ctrlEvt(btn,2); //hey, the button has been held past the med point
    }
  }
  //If the button has just been released...
  if(btnCur==btn && bnow==HIGH) {
    btnCur = 0;
    log("Btn "); log(String(btn)); log(" has been released\n\n");
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

void ctrlEvt(byte ctrl, byte evt){  
  //Handle button-like events, based on current fn and set state.
  //Currently only main, not alt... TODO
  bool mainSelPush = (ctrl==mainSel && evt==1?1:0);
  bool mainSelHold = (ctrl==mainSel && evt==2?1:0);
  bool mainAdjUpPush = (ctrl==mainAdj[0] && evt==1?1:0);
  bool mainAdjDnPush = (ctrl==mainAdj[1] && evt==1?1:0);
  
  if(fn!=255 && ctrl==mainSel && evt==3) { //joker mainSel long hold: enter SETUP menu
    btnStop(); fn=255; fnSet=1; updateDisplay(); return;
  }
  
  if(fn!=255) { //normal fn running/setting
    if(!fnSet) { //fn running
      if(mainSelHold) { //enter fnSet
        //btnStop(); prevents joker
        switch(fn){
          case 0: setSet((rtcLast[3]*60)+rtcLast[4],0,1439,1,1); break; //time: set mins
          case 1: setSet(rtcLast[0],2000,32767,1,1); break; //date: set year
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
      if(mainAdjUpPush) fnSetDo(1);
      if(mainAdjDnPush) fnSetDo(-1);
      //change (adj hold will be handled by fnSetHoldCheck(), but here we'll need press velocity to treat encoder changes like very rapid button presses
      //no mainSelHold will happen here
      if(mainSelPush) { //go to next option or save and exit fnSet
        btnStop();
        switch(fn){
          case 0: setRTC(3,fnSetVal/60,fnSetVal%60,0); clearSet(); break; //time: save
          case 1: switch(fnSet){
            case 1: setRTC(0,fnSetVal,0,0); setSet(rtcLast[1],1,12,0,2); break; //date: save year, set month
            case 2: setRTC(1,fnSetVal,0,0); setSet(rtcLast[2],1,getDaysInMonth(rtcLast[0],rtcLast[1]),0,3); break; //date: save month, set date
            case 3: setRTC(2,fnSetVal,0,0); clearSet(); break;
            default: break;  
          } break;
          case 2: //alarm
          case 3: //timer
          default: break;
        }
      }
    } //end fn setting
  } //end normal fn running/setting
  else { //setup menu
    if(ctrl==mainSel) {
      if(evt==1) { //mainSel press
        if(fnSet==setupOptsCt) { //that was the last one – exit
          log("SETUP: Exiting after last option\n");
          btnStop();
          fn=0; fnSet=0; updateDisplay(); return; //exit setup
        } else {
          log("SETUP: Proceeding to next option\n");
          fnSet++; updateDisplay(); return; //advance to next option
        }
      }
      if(evt==2) { //mainSel short hold
        log("SETUP: Exiting per mainSel short hold\n");
        btnStop(); //no more events from this press
        fn=0; fnSet=0; updateDisplay(); return; //exit setup
      }
    } //end mainSel
    if((ctrl==mainAdj[0] || ctrl==mainAdj[1]) && evt==1){ //mainAdj press
      log("SETUP: "); log(String(ctrl)); log(" pressed\n");
      btnStop(); //no more events from this press
      int delta = (ctrl==mainAdj[0]?1:-1);
      switch(fnSet){ //This is where we set the ranges for each option
        //setup opts details
        //setupOpts[0] exists but is just a pad to make it 1-index for programming convenience
        case 1: changeInRangeAr(setupOpts, 1, delta, 1, 2); break; //1=12hr, 2=24hr
        case 2: changeInRangeAr(setupOpts, 2, delta, 1, 2); break; //1=m/d/w, 2=d/m/w
        case 3: changeInRangeAr(setupOpts, 3, delta, 0, 2); break; //date during time: 0=no, 1=d->s, 2=date@:30
        case 4: changeInRangeAr(setupOpts, 4, delta, 0, 1); break; //leading zero no/yes
        case 5: changeInRangeAr(setupOpts, 5, delta, 1, 9999); break; //random number
        default: break;
      }
      updateDisplay(); return;
    } //end mainAdj
  }//end setup
}
void setSet(int n, int m, int x, bool v, byte p){
  fnSetVal=n; fnSetValMin=m; fnSetValMax=x; fnSetValVel=v; fnSet=p;
  updateDisplay();
}
void clearSet(){ setSet(0,0,0,0,0); }

void setRTC(byte unit, int v1, int v2, int v3){
  //Use rtcLast as a sort of rtcNext
  if(unit==3) { //time
    rtcLast[3]=v1; rtcLast[4]=v2; rtcLast[5]=v3;
  } else { //date
    rtcLast[unit] = v1; //Set in the new value. But in case current date doesn't exist in new yr/mo, change it
    if(unit<2) { byte dIM = getDaysInMonth(rtcLast[0],rtcLast[1]); if(rtcLast[2]>dIM) rtcLast[2]==dIM; }
  }
  rtc.adjust(DateTime(rtcLast[0],rtcLast[1],rtcLast[2],rtcLast[3],rtcLast[4],rtcLast[5]));
}

//TODO replace these with the fnSet temporary value
void changeInRangeAr(byte ar[], byte i, int delta, byte minVal, byte maxVal){
  ar[i] = changeInRange(ar[i], delta, minVal, maxVal);
}
byte changeInRange(byte curVal, int delta, byte minVal, byte maxVal){
  log("Changing value "); log(String(curVal)); log(" by "); log(String(delta)); log("\n");
  //Designed to work with bytes, unsigned integers between 0 and 255. No looping (for now?)
  if(delta==0) return curVal;
  if(delta>0) if(maxVal-curVal<delta) return maxVal; else return curVal+delta;
  if(delta<0) if(curVal-minVal<abs(delta)) return minVal; else return curVal+delta;
}

void fnSetDo(int delta){
  //Does actual setting of fnSetVal, as told to by ctrlEvt or fnSetHoldCheck, within params of fnSetVal vars
  if(delta>0) if(fnSetValMax-fnSetVal<delta) fnSetVal=fnSetValMax; else fnSetVal=fnSetVal+delta;
  if(delta<0) if(fnSetVal-fnSetValMin<abs(delta)) fnSetVal=fnSetValMin; else fnSetVal=fnSetVal+delta;
  updateDisplay();
}

unsigned long fnSetHoldLast;
void fnSetHoldCheck(){
  //When we're setting via an adj button that's passed a hold point, fire off fnSet commands at intervals
  if(fnSetHoldLast+250<millis()) { //TODO is it a problem this won't sync up with the blinking?
    fnSetHoldLast = millis();
    if(fnSet!=0 && ((mainAdjType==1 && (btnCur==mainAdj[0] || btnCur==mainAdj[1])) || (altAdjType==1 && (btnCur==altAdj[0] || btnCur==altAdj[1]))) ){ //if we're setting, and this is an adj input for which the type is button
      bool dir = (btnCur==mainAdj[0] || btnCur==altAdj[0] ? 1 : 0);
      //If short hold, or long hold but high velocity isn't supported, use low velocity (delta=1)
      if(btnCurHeld==2 || (btnCurHeld==3 && fnSetValVel==false)) fnSetDo(dir?1:-1);
      //else if long hold, use high velocity (delta=10)
      else if(btnCurHeld==3) fnSetDo(dir?10:-10);
    }
  }
}


////////// Display data formatting //////////
unsigned int rtcPollLast = 0; //maybe don't poll the RTC every loop? would that be good?
void checkRTC(){
  //TODO why are the digits flashing before changing? I've got some blocking code happening? is it here?
  //If in fn time or timer, updates display when RTC time changes. Also decrements running timer.
  if(rtcPollLast<millis()+50) { //check every 1/20th of a second
    rtcPollLast=millis();
    DateTime now = rtc.now();
    if(rtcLast[5] != now.second()) {
      rtcLast[0] = now.year();
      rtcLast[1] = now.month();
      rtcLast[2] = now.day();
      rtcLast[3] = now.hour();
      rtcLast[4] = now.minute();
      rtcLast[5] = now.second();
      //TODO decrement timer if appropriate
      if(fn==0 || fn==3) { //clock or timer
        updateDisplay();
      }
    } 
  }
}

void updateDisplay(){
  //Only run as often as the data behind the display changes (clock tick, setting change, etc.)
  //This function takes that data and uses it to edit displayNext[] for cycleDisplay() to pick up
  switch(fn){ //which function are we displaying?
    case 255: //SETUP menu
    log("Per SETUP, updating display to key="); log(String(fnSet)); log(", val="); log(String(setupOpts[fnSet])); log("\n");
    editDisplay(fnSet, 4, 5, false); //current option key, displayed on little tubes (4-5)
    editDisplay(setupOpts[fnSet], 0, 3, false); //current option value, on big tubes (0-3)
    case 1: //date
    break;
    case 2: //alarm
    break;
    case 3: //timer
    break;
    default: //clock
      if(!fnSet){
        byte hr = rtcLast[3];
        if(setupOpts[1]==1) hr = (hr==0?12:(hr>12?hr-12:hr));
        editDisplay(hr, 0, 1, setupOpts[4]);
        editDisplay(rtcLast[4], 2, 3, true);
        editDisplay(rtcLast[5], 4, 5, true);
      } else {
        //display clock setting
        editDisplay(fnSetVal/60, 0, 1, setupOpts[4]);
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
    log("In "); logint(place); log("s place at display position "); logint(posEnd-i); log(", we put "); if(displayNext[posEnd-i]==15) log("a blank"); else logint(displayNext[posEnd-i]); log(".\n"); //log(" n/place="); logint(n); log("/"); logint(place); log("="); logint(n/place); log("...(n/place)%10="); logint((n/place)%10); log("\n");
  }
  log("After editDisplay(), displayNext is {");
  for(byte i=0; i<=5; i++) { if(i!=0){log(",");} if(displayNext[i]==15) log("-"); else logint(displayNext[i]); }
  log("}\n");
} //end editDisplay()
void blankDisplay(byte posStart, byte posEnd){
  for(byte i=posStart; i<=posEnd; i++) displayNext[i]=15;
} //end blankDisplay();


////////// Display & alarm output //////////
//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {2,3,4,5};
byte binOutB[4] = {6,7,8,9};
//3 pins out to anode channel switches
byte anodes[3] = {11,12,13};

void initOutputs() {
  for(byte i=0; i<4; i++) pinMode(binOutA[i],OUTPUT);
  for(byte i=0; i<4; i++) pinMode(binOutB[i],OUTPUT);
  for(byte i=0; i<3; i++) pinMode(anodes[i],OUTPUT);
  pinMode(10, OUTPUT); //Alarm signal pin
}

//int displayNext[6] is the target, defined higher up so other fn's can set it.
float fadeMax = 5.0f;
float fadeStep = 1.0f;

int displayLast[6]={11,11,11,11,11,11}; //What is currently being displayed. We slowly fade away from this.
float displayNextFade[6]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f}; //Fading in displayNext values
float displayLastFade[6]={8.0f,8.0f,8.0f,8.0f,8.0f,8.0f}; //Fading out displayLast values

unsigned long setStartLast = 0; //to control flashing

void cycleDisplay(){
  bool dim = 0;//(setupOpts[2]>0?true:false); //Under normal circumstances, dim constantly if the time is right
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


////////// Misc global utility functions //////////
const byte daysInMonth[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
byte getDaysInMonth(int y, int m){
  if(m==2) return (y%4==0 && (y%100!=0 || y%400==0) ? 29 : 28); else return daysInMonth[m];
}
void log(String s){
  // while (!Serial) ;
  // Serial.print(s);
}
void logint(int i){ log(String(i)); }