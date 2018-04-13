//Light a cathode/anode combo full time, no multiplexing

const byte mainAdjUp = A3; //press to change cathode
const byte mainAdjDn = A2; //press to change anode

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {2,3,4,5};
byte binOutB[4] = {6,7,8,9};
//3 pins out to anode channel switches
byte anodes[3] = {11,12,13};

byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
byte curCathode = 0;
byte curAnode = 0;

void setup(){
  Serial.begin(9600);
  initOutputs();
  initInputs();
  updateDisplay();
}

void loop(){
  checkInputs();
  delay(10); //in case of switch bounce?
}

void initInputs(){
  pinMode(mainAdjUp, INPUT_PULLUP);
  pinMode(mainAdjDn, INPUT_PULLUP);
}
void checkInputs(){
  checkBtn(mainAdjUp);
  checkBtn(mainAdjDn);
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
    btnCur = btn;
    if(btn==mainAdjUp) { curCathode++; if(curCathode==10) curCathode = 0; updateDisplay(); }
    else if(btn==mainAdjDn) { curAnode++; if(curAnode==sizeof(anodes)) curAnode = 0; updateDisplay(); }
  }
  //If the button has just been released...
  if(btnCur==btn && bnow==HIGH) {
    btnCur = 0;
  }
}

void initOutputs() {
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
}

void updateDisplay() {
  for(byte i=0; i<sizeof(anodes); i++) { digitalWrite(anodes[i],LOW); } //all anodes off
  setCathodes(curCathode,curCathode);
  digitalWrite(anodes[curAnode],HIGH);
}

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