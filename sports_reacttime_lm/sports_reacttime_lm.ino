//For sports exhibit: how many times can you press Sel in N seconds?
//For 6-tube display

#include "configs/v8-cylinder.h"

// Hardware inputs and value setting
byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()
unsigned long inputLast = 0; //When a button was last pressed
unsigned long inputLast2 = 0; //Second-to-last of above
//TODO the math between these two may fail very rarely due to millis() rolling over while setting. Need to find a fix. I think it only applies to the rotary encoder though.

byte isRunning = 0; //1 if waiting, 2 if timing
unsigned long runStart = 0;
unsigned long waitDur = 0;

////////// Main code control //////////

void setup(){
  Serial.begin(9600);
  initInputs();
  delay(100);
  initOutputs(); //depends on some EEPROM settings
  editDisplay(0,0,3,1,0);
  blankDisplay(5,5,0);
}

void loop(){
  //TODO does not account for millis rollover - should be restarted at least once every 49 days
  //while (millis() < start + ms) ;  // BUGGY version
  //while (millis() - start < ms) ;  // CORRECT version
  //https://arduino.stackexchange.com/a/12588
  if(isRunning==1) {
    unsigned long now = millis();
    if(now-runStart >= waitDur) { //done waiting
      isRunning = 2;
      runStart = now;
      tone(piezoPin,getHz(beepTone),50);
      updateDisplay();
    }
  }
  if(isRunning==2) updateDisplay();
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
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
    btnCur = btn; btnCurHeld = 0; inputLast2 = inputLast; inputLast = now;
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


////////// Input handling and value setting //////////

void ctrlEvt(byte ctrl, byte evt){
  // Serial.print(F("Button "));
  // Serial.print(ctrl,DEC);
  // Serial.println(F(" pressed"));
  //Handle control events (from checkBtn or checkRot), based on current fn and set state.
  //evt: 1=press, 2=short hold, 3=long hold, 0=release.
  if(ctrl==mainSel && evt==1) { //only event we care about
    if(isRunning==0) {
      waitDur = random(2000,10000); //somewhere between 2 and 10 seconds
      runStart = millis();
      blankDisplay(0,5,0);
      tone(piezoPin,getHz(beepTone-7), 50);
      isRunning = 1;
    } else if(isRunning==1) {
      //nada. loop changes isRunning from 1 to 2
    } else if(isRunning==2) {
      tone(piezoPin,getHz(beepTone+5), 50);
      isRunning = 0;
    }
  } //end if evt==1
} //end ctrlEvt


////////// Display data formatting //////////

byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start. Change this to change tubes.
byte displayLast[6] = {11,11,11,11,11,11}; //for noticing changes to displayNext and fading the display to it
byte scrollDisplay[6] = {15,15,15,15,15,15}; //For animating a value into displayNext from right, and out to left

void updateDisplay(){
  editDisplay((millis()-runStart)/10, 0, 3, 1, 0);
} //end updateDisplay()

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


////////// Hardware outputs //////////

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {outA1,outA2,outA3,outA4};
byte binOutB[4] = {outB1,outB2,outB3,outB4};
//3 pins out to anode channel switches
byte anodes[3] = {anode1,anode2,anode3};

const int fadeDur = 3; //ms - each multiplexed pair of digits appears for this amount of time per cycle
const int dimDur = 4; //ms - portion of fadeDur that is left dark during dim times
int fadeNextDur = 0; //ms - during fade, incoming digit's portion of fadeDur
int fadeLastDur = fadeDur; //ms - during fade, outgoing digit's portion of fadeDur
unsigned long fadeStartLast = 0; //millis - when the last digit fade was started
unsigned long setStartLast = 0; //to control flashing during start

void initOutputs() {
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
  if(piezoPin>=0) pinMode(piezoPin, OUTPUT);
  if(ledPin>=0) pinMode(ledPin, OUTPUT); analogWrite(ledPin,0); //don't want it
}

void cycleDisplay(){
  //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
  setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
  digitalWrite(anodes[0], HIGH); //Turn on tubes
  delay(fadeLastDur);//-(dim?dimDur:0)); //Display for fade-out cycles
  setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
  delay(fadeNextDur);//-(dim?dimDur:0)); //Display for fade-in cycles
  digitalWrite(anodes[0], LOW); //Turn off tubes

  //Anode channel 1: tubes #4 (sec x10) and #1 (hour x1)
  setCathodes(displayLast[4],displayLast[1]);
  digitalWrite(anodes[1], HIGH);
  delay(fadeLastDur);
  setCathodes(displayNext[4],displayNext[1]);
  delay(fadeNextDur);
  digitalWrite(anodes[1], LOW);

  //Anode channel 2: tubes #0 (hour x10) and #3 (min x1)
  setCathodes(displayLast[0],displayLast[3]);
  digitalWrite(anodes[2], HIGH);
  delay(fadeLastDur);
  setCathodes(displayNext[0],displayNext[3]);
  delay(fadeNextDur);
  digitalWrite(anodes[2], LOW);
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

word getHz(byte note){
  //Given a piano key note, return frequency
  char relnote = note-49; //signed, relative to concert A
  float reloct = relnote/12.0; //signed
  word mult = 440*pow(2,reloct);
  return mult;
}