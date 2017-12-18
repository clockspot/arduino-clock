// #include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 rtc;
#include <ooPinChangeInt.h>
#include <AdaEncoder.h>

AdaEncoder mainRot = AdaEncoder('a',A1,A0);

word val = 500;

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


////////// Main code control //////////

void setup(){
  Serial.begin(57600);
  Wire.begin();
  rtc.begin();
  if(!rtc.isrunning()) rtc.adjust(DateTime(2017,1,1,0,0,0)); //TODO test
  initOutputs();
  initInputs();
  
  //mainRot.write(rotCount);
  
  editDisplay(val,0,3,false);
  
  //initEEPROM(readInput(mainSel)==LOW);
  //debugEEPROM();
  //setCaches();
}

void loop(){
  //Things done every "clock cycle"
  checkRTC(); //if clock has ticked, decrement timer if running, and updateDisplay
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  
  //long mainRotNew = mainRot.read();
  
  
  //doSetHold(); //if inputs have been held, this will do more things + updateDisplay as needed
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
}


////////// Control inputs //////////
void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  //Buttons
  //pinMode(A0, INPUT_PULLUP);
  //pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A6, INPUT); digitalWrite(A6, HIGH);
  pinMode(A7, INPUT); digitalWrite(A7, HIGH);
  
  //Encoders
  //If using rotary encoders, capture their initial state
  //if(mainAdjType==2) checkRot(mainAdjA,mainAdjB,mainRotLast,false);
  //if(altAdjType==2) checkRot(altAdjA,altAdjB,altRotLast,false);
}

void checkInputs(){
  AdaEncoder *thisEncoder=NULL;
  thisEncoder = AdaEncoder::genie();
  if(thisEncoder!=NULL) {
    int8_t clicks = thisEncoder->query();
    Serial.print(thisEncoder->getID()); Serial.print(':'); Serial.println(clicks);
    //editDisplay(clicks,4,5,false);
    //thisEncoder->getID();
    val += clicks;
    editDisplay(val,0,3,false);
  }
}

////////// Clock ticking and timed event triggering //////////
unsigned long rtcPollLast = 0; //maybe don't poll the RTC every loop? would that be good?
byte rtcSecLast = 61;
void checkRTC(){
  //Checks for new time-of-day second; decrements timer; checks for timed events;
  //updates display for running time or date.
  if(rtcPollLast<millis()+50) { //check every 1/20th of a second
    rtcPollLast=millis();
    //Check for timeouts based on millis
    //Update things based on RTC
    DateTime now = rtc.now();
    if(rtcSecLast != now.second()) {
      editDisplay(now.second(), 4, 5, true); //seconds
    } 
  }
}

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