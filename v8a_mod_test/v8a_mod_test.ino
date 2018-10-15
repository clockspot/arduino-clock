//For testing mods to the v8.0 board, style A, with LED PWM

const byte btnSel = A1; //was A1
const byte btnAlt = A0; //was A0
const byte btnUp = A6; //was A2
const byte btnDn = A7; //was A3
const byte pinLED = 11; //was A6
const byte pinRelay = A3; //was A7
byte binOutA[4] = {2,3,4,5};
byte binOutB[4] = {6,7,8,9};
byte anodes[3] = {16,12,13}; //first was 11

byte btnCur = 0; //Momentary button currently in use - only one allowed at a time

byte ledStateNow = 0;
byte ledStateTarget = 0;

void setup(){
  Serial.begin(9600);
  //0 and 1: set as digital input
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  //2 and 3: set as digital output
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  //4 and 5: for I2C
  //6 and 7: input, but analog pins with hardware pullup resistors, so nothing to do
  analogWrite(pinLED,0); //0 = LEDs off
  digitalWrite(pinRelay,1); //1 = connected device off
  
  //Set up just enough tube output to confirm the changed first anode is working
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); digitalWrite(binOutA[i],LOW); digitalWrite(binOutB[i],LOW); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); digitalWrite(anodes[i],LOW); }
  digitalWrite(anodes[0],HIGH); //the one we want to test

  Serial.println();
  Serial.println(F("Ready for input."));
  Serial.println(F("Display should now read 0 on 3rd and 6th tubes."));
  Serial.println(F("SEL should change them to a 1; ALT should change them to 2."));
  Serial.println(F("UP should fade the LEDs in and out."));
  Serial.println(F("DOWN should toggle the relay."));
}

void loop(){
  checkInputs();
  if(ledStateNow != ledStateTarget) {
    if(ledStateNow > ledStateTarget) { ledStateNow -= 5; }
    else if(ledStateNow < ledStateTarget) { ledStateNow += 5; }
    // Serial.print(ledStateNow,DEC);
    // Serial.print(F(" => "));
    // Serial.println(ledStateTarget,DEC);
    analogWrite(pinLED,ledStateNow);
  }
  delay(5); //in case of switch bounce?
}

void checkInputs(){
  checkBtn(btnSel);
  checkBtn(btnAlt);
  checkBtn(btnUp);
  checkBtn(btnDn);
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
    Serial.println();
    bool newState = LOW;
    switch(btn) {
      case btnSel:
        Serial.println(F("btnSel pressed"));
        digitalWrite(binOutA[0],HIGH);
        digitalWrite(binOutA[1],LOW);
        digitalWrite(binOutB[0],HIGH);
        digitalWrite(binOutB[1],LOW);
        break;
      case btnAlt:
        Serial.println(F("btnAlt pressed"));
        digitalWrite(binOutA[0],LOW);
        digitalWrite(binOutA[1],HIGH);
        digitalWrite(binOutB[0],LOW);
        digitalWrite(binOutB[1],HIGH);
        break;
      case btnUp:
        Serial.println(F("btnUp pressed"));
        ledStateTarget = (ledStateTarget==0?255:0);
        Serial.println();
        if(ledStateTarget==0) Serial.println(F("LED switched off. LED PWM pin should fade to open circuit."));
        else Serial.println(F("LED switched on. LED PWM pin should fade to closed circuit."));
        break;
      case btnDn:
        Serial.println(F("btnDn pressed"));
        newState = !digitalRead(pinRelay);
        digitalWrite(pinRelay, newState);
        Serial.println();
        if(newState) Serial.println(F("Relay switched on. Relay pins should measure open circuit now (connected device off)."));
        else Serial.println(F("Relay switched off. Relay pins should measure closed circuit now (connected device on)."));
        break;
      default: break;
    } //end button printing switch
  } //end if button presed
  //If the button has just been released...
  if(btnCur==btn && bnow==HIGH) {
    Serial.println();
    switch(btn){
      case btnSel:
        Serial.println(F("btnSel released"));
        break;
      case btnAlt:
        Serial.println(F("btnAlt released"));
        break;
      case btnUp:
        Serial.println(F("btnUp released"));
        break;
      case btnDn:
        Serial.println(F("btnDn released"));
        break;
      default: break;
    }
    btnCur = 0;
  }
}