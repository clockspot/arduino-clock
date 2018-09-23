//For testing mods to the v8.0 board

const byte btnSel = A1;
const byte btnAlt = A0;
const byte btnUp = A6;
const byte btnDn = A7;
const byte pinLED = A2;
const byte pinRelay = A3;

byte btnCur = 0; //Momentary button currently in use - only one allowed at a time

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
  digitalWrite(pinLED,0); //0 = LEDs off
  digitalWrite(pinRelay,1); //1 = connected device off
  Serial.println(F("Ready for input"));
}

void loop(){
  checkInputs();
  delay(10); //in case of switch bounce?
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
    switch(btn) {
      case btnSel: Serial.print(F("btnSel")); break;
      case btnAlt: Serial.print(F("btnAlt")); break;
      case btnUp: Serial.print(F("btnUp")); break;
      case btnDn: Serial.print(F("btnDn")); break;
      default: break;
    } //end button printing switch
    Serial.println(F(" pressed"));
    if(btn==btnUp || btn==btnDn) {
      byte curOutput = (btn==btnUp? pinLED: pinRelay);
      bool newState = !digitalRead(curOutput);
      digitalWrite(curOutput, newState);
      Serial.println();
      switch(curOutput) {
        case pinLED:
          if(newState) Serial.println(F("pinLED switched on. LED pins should measure a voltage now."));
          else         Serial.println(F("pinLED switched off. LED pins should measure NO voltage now."));
          break;
        case pinRelay:
          if(newState) Serial.println(F("pinRelay switched on. Relay pins should measure open circuit now (connected device off)."));
          else         Serial.println(F("pinRelay switched off. Relay pins should measure closed circuit now (connected device on)."));
          break;
        default: break;
      } //end output printing switch
    } //end if btn up/down
  } //end if button presed
  //If the button has just been released...
  if(btnCur==btn && bnow==HIGH) {
    btnCur = 0;
  }
}