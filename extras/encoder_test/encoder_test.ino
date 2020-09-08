// What input is associated with each control?
#define CTRL_SEL A2 //main select button - must be equipped
#define CTRL_UP A1 //main up/down buttons or rotary encoder - must be equipped
#define CTRL_DN A0

#define ROT_VEL_START 80 //if encoder step rate falls below this, kick into high velocity set (x10)
#define ROT_VEL_STOP 500 //if encoder step rate rises above this, drop into low velocity set (x1)

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
#define OUT_A1 2
#define OUT_A2 3
#define OUT_A3 4
#define OUT_A4 5
#define OUT_B1 6
#define OUT_B2 7
#define OUT_B3 8
#define OUT_B4 9
//3 pins out to anode channel switches
#define ANODE_1 11
#define ANODE_2 12
#define ANODE_3 13


#include <Wire.h>
#include <Encoder.h>
Encoder rot(CTRL_DN,CTRL_UP);


byte theVal = 0;


////////// Main code control //////////

void setup(){
  Serial.begin(9600);
  Wire.begin();
  initInputs();
  initOutputs();
  updateDisplay();
}

void loop(){
  checkInputs(); //if inputs have changed, this will do things + updateDisplay as needed
  cycleDisplay(); //keeps the display hardware multiplexing cycle going
  //TODO why isn't the display working
}


////////// Control inputs //////////
void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  pinMode(CTRL_SEL, INPUT_PULLUP);
  pinMode(CTRL_UP, INPUT_PULLUP);
  pinMode(CTRL_DN, INPUT_PULLUP);
}

void checkInputs(){
  checkRot();
}

bool rotVel = 0; //high velocity setting (x10 rather than x1)
unsigned long rotLastStep = 0; //timestamp of last completed step (detent)
int rotLastVal = 0;
void checkRot(){
  int rotCurVal = rot.read();
  if(rotCurVal!=rotLastVal){ //we've sensed a state change
    rotLastVal = rotCurVal;
    Serial.println(rotCurVal,DEC);
    if(rotCurVal>=4 || rotCurVal<=-4){ //we've completed a step of 4 states (this library doesn't seem to drop states much, so this is reasonably reliable)
      unsigned long now = millis();
      if((unsigned long)(now-rotLastStep)<=ROT_VEL_START) rotVel = 1; //kick into high velocity setting (x10)
      else if((unsigned long)(now-rotLastStep)>=ROT_VEL_STOP) rotVel = 0; //fall into low velocity setting (x1)
      rotLastStep = now;
      while(rotCurVal>=4) { rotCurVal-=4; theVal+=1; }
      while(rotCurVal<=-4) { rotCurVal+=4; theVal-=1; }
      rot.write(rotCurVal);
      updateDisplay();
    }
  }
} //end checkRot()


////////// Display data formatting //////////

byte displayNext[6] = {15,15,15,15,15,15};
byte displayLast[6] = {15,15,15,15,15,15};

void updateDisplay(){
  Serial.print(F("---")); Serial.print(theVal,DEC); if(rotVel) Serial.print(F("!")); Serial.println();
  editDisplay(theVal,0,3,false,false);
  blankDisplay(4,5,false);
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
      case 4: place=10000; break;
      case 5: place=100000; break;
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

byte binOutA[4] = {OUT_A1,OUT_A2,OUT_A3,OUT_A4};
byte binOutB[4] = {OUT_B1,OUT_B2,OUT_B3,OUT_B4};
byte anodes[3] = {ANODE_1,ANODE_2,ANODE_3};

const int fadeLastDur = 5;
const int fadeNextDur = 0;

void initOutputs() {
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
}

void cycleDisplay(){
    //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
    setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
    digitalWrite(anodes[0], HIGH); //Turn on tubes
    delay(fadeLastDur); //Display for fade-out cycles
    setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
    delay(fadeNextDur); //Display for fade-in cycles
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