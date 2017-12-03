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
const byte mainAdjA = A1; //main up/down buttons or rotary encoder - must be equipped
const byte mainAdjB = A0;
const byte mainAdjType = 2;

////////// Function prototypes, global consts and vars //////////

// Hardware inputs
//unsigned long inputSampleLast = 0; //millis() of last time inputs (and RTC) were sampled
//const byte inputSampleDur = 50; //input sampling frequency (in ms) to avoid bounce
//unsigned long inputLast = 0; //When a button was last pressed / knob was last turned
//unsigned long inputLast2 = 0; //Second-to-last of above
//byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()
bool mainRotLast[2] = {0,0}; //last state of main rotary encoder inputs
void initInputs(); //Set pinModes for inputs; capture initial state of knobs (rotary encoders, if equipped).
void checkInputs(); //Run at sample rate. Calls checkBtn() for each eligible button and ctrlEvt() for each knob event.
bool readInput(byte pin); //Does analog or digital read depending on which type of pin it is.
//void checkBtn(byte btn); //Calls ctrlEvt() for each button event
//void btnStop(); //Stops further events from a button until it's released, to prevent unintended behavior after an event is handled.
void checkRot(byte rotA, byte rotB, bool last[], bool triggerEvent);

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
void setCathodes(byte decValA, byte decValB);
void decToBin(bool binVal[], byte i);


////////// Includes and main code control //////////
// #include <EEPROM.h>
// #include <Wire.h>
// #include <RTClib.h>
// RTC_DS1307 rtc;

void setup(){
  //Serial.begin(57600);
  //Wire.begin();
  //rtc.begin();
  //if(!rtc.isrunning()) rtc.adjust(DateTime(2017,1,1,0,0,0)); //TODO test
  initOutputs();
  initInputs();
  //initEEPROM(readInput(mainSel)==LOW);
  //debugEEPROM();
  //setCaches();
}

void loop(){
  //Things done every "clock cycle"
  //checkRTC(); //if clock has ticked, decrement timer if running, and updateDisplay
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  //doSetHold(); //if inputs have been held, this will do more things + updateDisplay as needed
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
  //If using rotary encoders, capture their initial state
  if(mainAdjType==2) checkRot(mainAdjA,mainAdjB,mainRotLast,false);
  //if(altAdjType==2) checkRot(altAdjA,altAdjB,altRotLast,false);
}

void checkInputs(){
  // TODO can all this if/else business be defined at load instead of evaluated every sample?
  //if(millis() >= inputSampleLast+inputSampleDur) { //time for a sample
    //inputSampleLast = millis();
    //potential issue: if user only means to rotate or push encoder but does both?
    //checkBtn(mainSel); //main select
    //if(mainAdjType==2)
      checkRot(mainAdjA,mainAdjB,mainRotLast,true); //main rotary encoder
    //else {
      //checkBtn(mainAdjA); checkBtn(mainAdjB);//main adj buttons
    //}
    //if(altSel!=0) checkBtn(altSel); //alt select (if equipped)
    //if(altAdjType==2) checkRot(altAdj,altRotLast,true); //alt rotary encoder (if equipped)
    //else
    //if(altAdjType==1) { checkBtn(altAdjA); checkBtn(altAdjB); } //alt adj buttons
  //} //end if time for a sample
}
bool readInput(byte pin){
  if(pin==A6 || pin==A7) return analogRead(pin)<100?0:1; //analog-only pins
  else return digitalRead(pin);
}

word numMoves = 0;
void checkRot(byte rotA, byte rotB, bool last[], bool triggerEvent){
  //Changes in rotary encoders.
  //When an encoder has changed, will call ctrlEvt(ctrl,1)
  //mimicking an up/down adj button press
  //TODO do we need to watch the rotation direction w/ last2[] to accommodate for inputs changing faster than we sample?
  //if(btnCur==0) { //only do if a button isn't pressed
    bool newA = readInput(rotA);
    bool newB = readInput(rotB);
    //if(newA != last[0] && triggerEvent) ctrlEvt(newA==last[1] ? rotA : rotB, 1);
    //If A changed, we'll pretend B didn't - see TODO above
    //else if(newB != last[1] && triggerEvent) ctrlEvt(newB==last[0] ? rotB : rotA, 1);
    
    if(newA!=last[0] || newB!=last[1]) {
      numMoves++;
      //Big tubes: show the number of moves we've had
      editDisplay(numMoves,0,3,false);
      //Small tubes: show the state of each of the encoder's pins
      editDisplay(newA,4,4,false);
      editDisplay(newB,5,5,false);
    }
    
    last[0] = newA;
    last[1] = newB;
  //}//end if button isn't pressed
}//end checkRot



////////// Display data formatting //////////
// void updateDisplay(){
//   //Run as needed to update display when the value being shown on it has changed
//   //(Ticking time and date are an exception - see checkRTC() )
//   //This formats the new value and puts it in displayNext[] for cycleDisplay() to pick up
//   if(fnSet) { //setting
//     //little tubes:
//     if(fn==255) editDisplay(fnSet, 4, 5, false); //setup menu: current option key
//     else blankDisplay(4, 5); //fn setting: blank
//     //big tubes:
//     if(fnSetValMax==1439) { //value is a time of day
//       editDisplay(fnSetVal/60, 0, 1, EEPROM.read(optsLoc[4])); //hours with leading zero
//       editDisplay(fnSetVal%60, 2, 3, true);
//     } else editDisplay(fnSetVal, 0, 3, false); //some other type of value
//   }
//   else { //fn running
//     switch(fn){
//       //case 0: //time taken care of by checkRTC()
//       //case 1: //date taken care of by checkRTC()
//       case 2: //alarm
//       case 3: //timer
//       break;
//     }
//   }
// } //end updateDisplay()

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
void initOutputs() {
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
  pinMode(10, OUTPUT); //Alarm signal pin
}

void cycleDisplay(){
  bool dim = 0;//(opts[2]>0?true:false); //Under normal circumstances, dim constantly if the time is right
  // if(fnSet>0) { //but if we're setting, dim for every other 500ms since we started setting
  //   if(setStartLast==0) setStartLast = millis();
  //   dim = 1-(((millis()-setStartLast)/500)%2);
  // } else {
  //   if(setStartLast>0) setStartLast=0;
  // }
  
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