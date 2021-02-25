#ifdef INPUT //only compile when requested (when included in main file)
#ifndef INPUT_SRC //include once only
#define INPUT_SRC

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#ifdef INPUT_UPDN_ROTARY
  #include <Encoder.h> //Paul Stoffregen - install in your Arduino IDE
  Encoder rot(CTRL_R1,CTRL_R2);  //TODO may need to reverse
#endif
#ifdef INPUT_IMU
  #include <Arduino_LSM6DS3.h>
  //If we don't already have inputs defined for Sel/Alt/Up/Dn, use some bogus ones
  #ifndef CTRL_SEL
    #define CTRL_SEL 100
  #endif
  #ifndef CTRL_ALT
    #define CTRL_ALT 101
  #endif
  #ifndef CTRL_UP
    #define CTRL_UP 102
  #endif
  #ifndef CTRL_DN
    #define CTRL_DN 103
  #endif
  //IMU "debouncing"
  int imuYState = 0; //the state we're reporting (-1, 0, 1)
  int imuYTestState = 0; //the state we've actually last seen
  int imuYTestCount = 0; //how many times we've seen it
  int imuZState = 0; //the state we're reporting (-1, 0, 1)
  int imuZTestState = 0; //the state we've actually last seen
  int imuZTestCount = 0; //how many times we've seen it
  void readIMU(){
    float x, y, z;
    IMU.readAcceleration(x,y,z);
    int imuState;
    //Assumes Arduino is oriented with components facing back of clock, and USB port facing up
         if(y<=-0.5) imuState = -1;
    else if(y>= 0.5) imuState = 1;
    else if(y>-0.3 && y<0.3) imuState = 0;
    else imuState = imuYTestState; //if it's not in one of the ranges, treat it as "same"
    if(imuYTestState!=imuState){ imuYTestState=imuState; imuYTestCount=0; }
    if(imuYTestCount<imuTestCountTrigger){ imuYTestCount++; /*Serial.print("Y "); Serial.print(imuYTestState); Serial.print(" "); for(char i=0; i<imuYTestCount; i++) Serial.print("#"); Serial.println(imuYTestCount);*/ if(imuYTestCount==imuTestCountTrigger) imuYState=imuYTestState; }
  
         if(z<=-0.5) imuState = -1;
    else if(z>= 0.5) imuState = 1;
    else if(z>-0.3 && z<0.3) imuState = 0;
    else imuState = imuZTestState;
    if(imuZTestState!=imuState){ imuZTestState=imuState; imuZTestCount=0; }
    if(imuZTestCount<imuTestCountTrigger){ imuZTestCount++; /*Serial.print("Z "); Serial.print(imuZTestState); Serial.print(" "); for(char i=0; i<imuZTestCount; i++) Serial.print("#"); Serial.println(imuZTestCount);*/ if(imuZTestCount==imuTestCountTrigger) imuZState=imuZTestState; }
  }
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
    //rotary needs no init here
  #endif
  #ifdef INPUT_IMU
    //if(!IMU.begin()){ Serial.println("Failed to initialize IMU!"); while(1); }
    IMU.begin();
  #endif
}

bool readBtn(byte btn){
  //Reads momentary button and/or IMU position, as equipped
  //Returns true if one or both are "pressed"
  bool btnPressed = false;
  bool imuPressed = false;
  #ifdef INPUT_BUTTONS
    if(btn!=0){ //skip disabled alt
      if(btn==A6 || btn==A7) btnPressed = analogRead(btn)<100; //analog-only pins
      else btnPressed = !(digitalRead(btn)); //false (low) when pressed
    }
  #endif
  #ifdef INPUT_IMU
    switch(btn){
      //Assumes Arduino is oriented with components facing back of clock, and USB port facing up
      //TODO support other orientations
      case CTRL_SEL: imuPressed = imuZState<0; break; //clock tilted backward
      case CTRL_ALT: imuPressed = imuZState>0; break; //clock tilted forward
      case CTRL_DN:  imuPressed = imuYState<0; break; //clock tilted left
      case CTRL_UP:  imuPressed = imuYState>0; break; //clock tilted right
      default: break;
    }
  #endif
  return (btnPressed || imuPressed);
}

unsigned long holdLast;
void checkBtn(byte btn){
  //Polls for changes in momentary buttons (or IMU positioning), LOW = pressed.
  //When a button event has occurred, will call ctrlEvt in main code.
  //Only called by checkInputs() and only for inputs configured as button and/or IMU.
  bool bnow = readBtn(btn);
  unsigned long now = millis();
  //If the button has just been pressed, and no other buttons are in use...
  if(inputCur==0 && bnow) {
    inputCur = btn; inputCurHeld = 0; inputLast = now; inputLastTODMins = rtcGetTOD();
    ctrlEvt(btn,1); //hey, the button has been pressed
  }
  //If the button is being held...
  if(inputCur==btn && bnow) {
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
        }
      }
    #endif
  }
  //If the button has just been released...
  if(inputCur==btn && !bnow) {
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
  bool ret = readBtn(CTRL_SEL);
  //prevent the held button from doing anything else, if applicable
  //TODO expand this so short and long hold can happen at startup
  inputCur = CTRL_SEL; inputStop();
  return ret;
}

void checkInputs(){
  //TODO potential issue: if user only means to rotate or push encoder but does both?
  #ifdef INPUT_IMU
    readIMU(); //captures IMU state for checkBtn/readBtn to look at
  #endif
  //checkBtn calls readBtn which will read button and/or IMU as equipped
  //We just need to only call checkBtn if one or the other is equipped
  #if defined(INPUT_BUTTONS) || defined(INPUT_IMU)
    checkBtn(CTRL_SEL);
    #if CTRL_ALT!=0
      checkBtn(CTRL_ALT);
    #endif
    #if defined(INPUT_UPDN_BUTTONS) || defined(INPUT_IMU)
      checkBtn(CTRL_UP);
      checkBtn(CTRL_DN);
    #endif
  #endif
  #ifdef INPUT_UPDN_ROTARY
    checkRot();
  #endif
}

#endif
#endif