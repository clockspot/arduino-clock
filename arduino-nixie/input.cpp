#ifdef INPUT //only compile when requested (when included in main file)
#ifndef INPUT_SRC //include once only
#define INPUT_SRC

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#ifdef INPUT_UPDN_ROTARY
  #include <Encoder.h> //Paul Stoffregen - install in your Arduino IDE
  Encoder rot(CTRL_R1,CTRL_R2);  //TODO may need to reverse
#endif

byte inputCur = 0; //Momentary button (or IMU position) currently in use - only one allowed at a time
byte inputCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=set by inputStop()

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

bool readInput(byte pin){
  if(pin==A6 || pin==A7) return analogRead(pin)<100?0:1; //analog-only pins
  else return digitalRead(pin);
  //TODO add support for IMU
}

unsigned long holdLast;
void checkBtn(byte btn){
  //Changes in momentary buttons (or IMU positioning), LOW = pressed.
  //When a button event has occurred, will call ctrlEvt in main code.
  //Only called by checkInputs() and only for buttons configured.
  bool bnow = readInput(btn);
  unsigned long now = millis();
  //If the button has just been pressed, and no other buttons are in use...
  if(inputCur==0 && bnow==LOW) {
    inputCur = btn; inputCurHeld = 0; inputLast = now; inputLastTODMins = rtcGetTOD();
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(inputCur==btn && bnow==LOW) {
    //If the button has passed a hold duration threshold... (ctrlEvt will only act on these for Sel/Alt)
    if((unsigned long)(now-inputLast)>=CTRL_HOLD_LONG_DUR && inputCurHeld < 3) { //account for rollover
      inputCurHeld = 3;
      ctrlEvt(btn,3); //hey, the button has been long-held
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_SHORT_DUR && inputCurHeld < 2) {
      inputCurHeld = 2;
      ctrlEvt(btn,2); //hey, the button has been short-held
      holdLast = now; //starts the repeated presses code going
    }
    //While Up/Dn are being held, send repeated presses to ctrlEvt
    #ifdef INPUT_UPDN_BUTTONS
      if((btn==CTRL_UP || btn==CTRL_DN) && inputCurHeld >= 2){
        if((unsigned long)(now-holdLast)>=(inputCurHeld==3?20:125)){ //TODO could make it nonlinear
          holdLast = now;
          ctrlEvt(btn,1);
          // if(fnSetPg!=0 && (inputCur==CTRL_UP || inputCur==CTRL_DN) ){ //if we're setting, and this is an adj btn
          //   bool dir = (inputCur==CTRL_UP ? 1 : 0);
          //   //If short hold, or long hold but high velocity isn't supported, use low velocity (delta=1)
          //   if(inputCurHeld==2 || (inputCurHeld==3 && fnSetValVel==false)) doSet(dir?1:-1);
          //   //else if long hold, use high velocity (delta=10)
          //   else if(inputCurHeld==3) doSet(dir?10:-10);
          // }
        }
      }
    #endif
  }
  //If the button has just been released...
  if(inputCur==btn && bnow==HIGH) {
    inputCur = 0;
    if(inputCurHeld < 4) ctrlEvt(btn,0); //hey, the button was released
    inputCurHeld = 0;
  }
}
void inputStop(){
  //In some cases, when handling btn evt 1/2/3, we may call this so following events 2/3/0 won't cause unintended behavior (e.g. after a fn change, or going in or out of set)
  inputCurHeld = 4;
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

bool checkForHeldButtonAtStartup(){
  //This is a bit outside the standard ctrlEvt functionality due to happening at startup
  bool ret = (readInput(CTRL_SEL)==LOW);
  //prevent the held button from doing anything else, if applicable
  //TODO expand this so short and long hold can happen at startup
  inputCur = CTRL_SEL; inputStop();
  return ret;
}

void checkInputs(){
  //TODO potential issue: if user only means to rotate or push encoder but does both?
  #ifdef INPUT_BUTTONS
    checkBtn(CTRL_SEL); //select
    #if CTRL_ALT!=0 //alt (if equipped)
      checkBtn(CTRL_ALT);
    #endif
    #ifdef INPUT_UPDN_BUTTONS
      checkBtn(CTRL_UP);
      checkBtn(CTRL_DN);
    #endif
  #endif
  #ifdef INPUT_UPDN_ROTARY
    checkRot(); //calls ctrlEvt
  #endif
}

#endif
#endif