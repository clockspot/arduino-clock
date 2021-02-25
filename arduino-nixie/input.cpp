#ifdef INPUT //only compile when requested (when included in main file)
#ifndef INPUT_SRC //include once only
#define INPUT_SRC

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#ifdef INPUT_UPDN_ROTARY
  #include <Encoder.h> //Paul Stoffregen - install in your Arduino IDE
  Encoder rot(CTRL_R1,CTRL_R2);  //TODO may need to reverse
#endif

//IME positioning works like a button. TODO change these to inputCur
byte btnCur = 0; //Momentary button currently in use - only one allowed at a time
byte btnCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by btnStop()

unsigned long inputLast = 0; //When an input last took place, millis()
int inputLastTODMins = 0; //When an input last took place, time of day. Used in paginated functions so they all reflect the time of day when the input happened.

void initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  #ifdef INPUT_BUTTONS
    pinMode(CTRL_SEL, INPUT_PULLUP);
    #if CTRL_ALT!=0
      pinMode(CTRL_ALT, INPUT_PULLUP);
    #endif
    #ifdef INPUT_UPDN_BUTTONS
      pinMode(CTRL_UP, INPUT_PULLUP);
      pinMode(CTRL_DN, INPUT_PULLUP);
    #endif
  #endif
  #ifdef INPUT_UPDN_ROTARY
    //TODO init rotary
  #endif
  #ifdef INPUT_IMU
    //TODO init imu
  #endif
}

bool readInput(byte pin){ //TODO when does the main code call this
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
    btnCur = btn; btnCurHeld = 0; inputLast = now; inputLastTODMins = rtcGetTOD();
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(btnCur==btn && bnow==LOW) {
    if((unsigned long)(now-inputLast)>=CTRL_HOLD_LONG_DUR && btnCurHeld < 3) { //account for rollover
      btnCurHeld = 3;
      ctrlEvt(btn,3); //hey, the button has been long-held
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_SHORT_DUR && btnCurHeld < 2) {
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

bool rotVel = 0; //high velocity setting (x10 rather than x1)
#ifdef INPUT_UPDN_ROTARY
unsigned long rotLastStep = 0; //timestamp of last completed step (detent)
int rotLastVal = 0;
void checkRot(){
  //Changes in rotary encoder. When rotation(s) occur, will call ctrlEvt to simulate btn presses. During setting, ctrlEvt will take rotVel into account.
  int rotCurVal = rot.read();
  if(rotCurVal!=rotLastVal){ //we've sensed a state change
    rotLastVal = rotCurVal;
    if(rotCurVal>=4 || rotCurVal<=-4){ //we've completed a step of 4 states (this library doesn't seem to drop states much, so this is reasonably reliable)
      unsigned long now = millis();
      inputLast = now; inputLastTODMins = rtcGetTOD();
      if((unsigned long)(now-rotLastStep)<=ROT_VEL_START) rotVel = 1; //kick into high velocity setting (x10)
      else if((unsigned long)(now-rotLastStep)>=ROT_VEL_STOP) rotVel = 0; //fall into low velocity setting (x1)
      rotLastStep = now;
      while(rotCurVal>=4) { rotCurVal-=4; ctrlEvt(CTRL_UP,1); }
      while(rotCurVal<=-4) { rotCurVal+=4; ctrlEvt(CTRL_DN,1); }
      rot.write(rotCurVal);
    }
  }
} //end checkRot()
#endif

void checkInputs(){
  //if inputs have changed, this will do things + updateDisplay as needed
  //TODO potential issue: if user only means to rotate or push encoder but does both?
  #ifdef INPUT_BUTTONS
    checkBtn(CTRL_SEL); //select
    #if CTRL_ALT!=0 //alt (if equipped)
      checkBtn(CTRL_ALT);
    #endif
    #ifdef INPUT_UPDN_BUTTONS
      checkBtn(CTRL_UP);
      checkBtn(CTRL_DN);
      doSetHold(false); //if inputs have been held, this will do more things + updateDisplay as needed
    #endif
  #endif
  #ifdef INPUT_UPDN_ROTARY
    checkRot(); //calls ctrlEvt
  #endif
}

#endif
#endif