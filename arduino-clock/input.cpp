#include <arduino.h>
#include "arduino-clock.h"

#include "input.h"

//Needs access to RTC timestamps
#include "rtcDS3231.h"
#include "rtcMillis.h"

#ifndef HOLDSET_SLOW_RATE
#define HOLDSET_SLOW_RATE 125
#endif
#ifndef HOLDSET_FAST_RATE
#define HOLDSET_FAST_RATE 20
#endif
#ifndef DEBOUNCE_DUR
#define DEBOUNCE_DUR 150 //ms
#endif

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#ifdef INPUT_UPDN_ROTARY
  #include <Encoder.h> //Paul Stoffregen - install in your Arduino IDE
  Encoder rot(CTRL_UP,CTRL_DN);  //TODO may need to reverse
#endif
#ifdef INPUT_IMU
  #include <Arduino_LSM6DS3.h>

  int8_t imuRoll, imuPitch;
  unsigned long imuLastChange;

  void readIMU(unsigned long now){
    //IMU.readAcceleration will give us three values, but we only care about two,
    //since we are only reading the clock being tilted left/right (roll) and back/front (pitch).
    float roll, pitch, nah;
    /*
    Now we decide how to read the IMU into those values, per the orientation of the Nano:
    USB   IC     Roll Pitch
    Up    Front    y   -z
          Back    -y    z
          Left     z    y
          Right   -z   -y
    Down  Front   -y   -z
          Back     y    z
          Left     z   -y
          Right   -z    y
    Left  Front    x   -z
          Back     x    z
          Up       x   -y
          Down     x    y
    Right Front   -x   -z
          Back    -x    z
          Up      -x   -y
          Down    -x    y
    Front Left     z   -x
          Right   -z   -x
          Up      -y   -x
          Down     y   -x
    Back  Left     z    x
          Right   -z    x
          Up       y    x
          Down    -y    x
    Or, more succinctly:
    USB Left:  Roll  = x
    USB Right: Roll  = -x
    IC  Left:  Roll  = z
    IC  Right: Roll  = -z
    USB Front: Pitch = -x
    USB Back:  Pitch = x
    IC  Front: Pitch = -z
    IC  Back:  Pitch = z
    ...and then a bunch of nonsense to capture y  >:(
    There might be a clever way to encode this in future,
    but for now let's hardcode it with preprocessor directives
    TODO may also want to apply an offset in case the clock is naturally slightly tilted?
    */

    #ifdef USB_DIR_UP
      #ifdef IC_DIR_FRONT //y, -z (roll, pitch)
        IMU.readAcceleration(nah, roll, pitch); pitch = -pitch;
      #endif
      #ifdef IC_DIR_BACK //-y, z
        IMU.readAcceleration(nah, roll, pitch); roll = -roll;
      #endif
      #ifdef IC_DIR_LEFT //z, y
        IMU.readAcceleration(nah, pitch, roll);
      #endif
      #ifdef IC_DIR_RIGHT //-z, -y
        IMU.readAcceleration(nah, pitch, roll); roll = -roll; pitch = -pitch;
      #endif
    #endif //USB_DIR_UP

    #ifdef USB_DIR_DOWN
      #ifdef IC_DIR_FRONT //-y, -z
        IMU.readAcceleration(nah, roll, pitch); roll = -roll; pitch = -pitch;
      #endif
      #ifdef IC_DIR_BACK //y, z
        IMU.readAcceleration(nah, roll, pitch);
      #endif
      #ifdef IC_DIR_LEFT //z, -y
        IMU.readAcceleration(nah, pitch, roll); pitch = -pitch;
      #endif
      #ifdef IC_DIR_RIGHT //-z, y
        IMU.readAcceleration(nah, pitch, roll); roll = -roll;
      #endif
    #endif //USB_DIR_DOWN

    #ifdef USB_DIR_LEFT
      #ifdef IC_DIR_FRONT //x, -z
        IMU.readAcceleration(roll, nah, pitch); pitch = -pitch;
      #endif
      #ifdef IC_DIR_BACK //x, z
        IMU.readAcceleration(roll, nah, pitch);
      #endif
      #ifdef IC_DIR_UP //x, -y
        IMU.readAcceleration(roll, pitch, nah); pitch = -pitch;
      #endif
      #ifdef IC_DIR_DOWN //x, y
        IMU.readAcceleration(roll, pitch, nah);
      #endif
    #endif //USB_DIR_LEFT

    #ifdef USB_DIR_RIGHT
      #ifdef IC_DIR_FRONT //-x, -z
        IMU.readAcceleration(roll, nah, pitch); roll = -roll; pitch = -pitch;
      #endif
      #ifdef IC_DIR_BACK //-x, z
        IMU.readAcceleration(roll, nah, pitch); roll = -roll;
      #endif
      #ifdef IC_DIR_UP //-x, -y
        IMU.readAcceleration(roll, pitch, nah); roll = -roll; pitch = -pitch;
      #endif
      #ifdef IC_DIR_DOWN //-x, y
        IMU.readAcceleration(roll, pitch, nah); roll = -roll;
      #endif
    #endif //USB_DIR_RIGHT

    #ifdef USB_DIR_FRONT
      #ifdef IC_DIR_LEFT //z, -x
        IMU.readAcceleration(pitch, nah, roll); pitch = -pitch;
      #endif
      #ifdef IC_DIR_RIGHT //-z, -x
        IMU.readAcceleration(pitch, nah, roll); roll = -roll; pitch = -pitch;
      #endif
      #ifdef IC_DIR_UP //-y, -x
        IMU.readAcceleration(pitch, roll, nah); roll = -roll; pitch = -pitch;
      #endif
      #ifdef IC_DIR_DOWN //y, -x
        IMU.readAcceleration(pitch, roll, nah); pitch = -pitch;
      #endif
    #endif //USB_DIR_FRONT

    #ifdef USB_DIR_BACK
      #ifdef IC_DIR_LEFT //z, x
        IMU.readAcceleration(pitch, nah, roll);
      #endif
      #ifdef IC_DIR_RIGHT //-z, x
        IMU.readAcceleration(pitch, nah, roll); roll = -roll;
      #endif
      #ifdef IC_DIR_UP //y, x
        IMU.readAcceleration(pitch, roll, nah);
      #endif
      #ifdef IC_DIR_DOWN //-y, x
        IMU.readAcceleration(pitch, roll, nah); roll = -roll;
      #endif
    #endif //USB_DIR_BACK
      
    //should activate (>=1) at 30 degrees (reading of >=1/3)
    roll *= 3;
    pitch *= 3;
    
    //only update imuLastChange if the value has changed
    if((int)roll !=imuRoll)  { imuRoll  = (int)roll;  imuLastChange = now; }
    if((int)pitch!=imuPitch) { imuPitch = (int)pitch; imuLastChange = now; }
    
  } //end readIMU  
  
#endif //INPUT_IMU

byte inputCur = 0; //Momentary button (or IMU position) currently in use - only one allowed at a time
byte inputCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=verylong, 5=superlong, 10=set by inputStop()

unsigned long inputLast = 0; //When an input last took place, millis()
int inputLastTODMins = 0; //When an input last took place, time of day. Used in paginated functions so they all reflect the time of day when the input happened.

bool initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  #ifdef INPUT_BUTTONS
    pinMode(CTRL_SEL, INPUT_PULLUP);
    if(CTRL_ALT>0){ //preprocessor directives don't seem to work for this when e.g. "A7"
      pinMode(CTRL_ALT, INPUT_PULLUP);
    }
    #ifdef INPUT_UPDN_BUTTONS
      pinMode(CTRL_UP, INPUT_PULLUP);
      pinMode(CTRL_DN, INPUT_PULLUP);
    #endif
  #endif
  #ifdef INPUT_UPDN_ROTARY
    //rotary needs no init here
  #endif
  #ifdef INPUT_IMU
    //if(!IMU.begin()){ Serial.println(F("Failed to initialize IMU!")); while(1); }
    IMU.begin();
    //Serial.println(F("IMU initialized"));
  #endif
  //Check to see if CTRL_SEL is held at init - facilitates version number display and EEPROM hard init
  delay(100); //prevents the below from firing in the event there's a capacitor stabilizing the input, which can read low falsely
  if(readBtn(CTRL_SEL)){ inputCur = CTRL_SEL; return true; }
  else return false;
}

bool readBtn(byte btn){
  //Reads momentary button and/or IMU position, as equipped
  #ifdef INPUT_BUTTONS
    if(btn>0){ //skip disabled alt
      if(btn==A6 || btn==A7) return analogRead(btn)<100; //analog-only pins
      else return !(digitalRead(btn)); //false (low) when pressed
    }
  #endif
  #ifdef INPUT_IMU
    //report using current roll/pitch values
    switch(btn){
      case CTRL_SEL: return imuPitch < 0; break; //clock tilted backward (dial up)
      case CTRL_ALT: return imuPitch > 0; break; //clock tilted forward (dial down)
      case CTRL_DN:  return imuRoll < 0; break; //clock tilted left
      case CTRL_UP:  return imuRoll > 0; break; //clock tilted right
      default: break;
    }
  #endif
  return 0;
}

unsigned long holdLast;
void checkBtn(byte btn, unsigned long now){
  //Polls for changes in momentary buttons (or IMU positioning), LOW = pressed.
  //When a button event has occurred, will call ctrlEvt in main code.
  //Only called by checkInputs() and only for inputs configured as button and/or IMU.
  bool bnow = readBtn(btn);
  //If the button has just been pressed, and no other buttons are in use...
  if(inputCur==0 && bnow) {
    // Serial.print(F("Btn "));
    // Serial.print(btn,DEC);
    // Serial.println(F(" pressed"));
    inputCur = btn; inputCurHeld = 0; inputLast = now; inputLastTODMins = rtcGetTOD();
    //Serial.println(); Serial.println(F("ich now 0 per press"));
    ctrlEvt(btn,1,inputCurHeld); //hey, the button has been pressed
    //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after press > ctrlEvt"));
  }
  //If the button is being held...
  if(inputCur==btn && bnow) {
    //If the button has passed a hold duration threshold... (ctrlEvt will only act on these for Sel/Alt)
    if((unsigned long)(now-inputLast)>=CTRL_HOLD_SUPERLONG_DUR && inputCurHeld < 5){
      ctrlEvt(btn,5,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 5;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 5 hold > ctrlEvt"));
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_VERYLONG_DUR && inputCurHeld < 4){
      ctrlEvt(btn,4,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 4;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 4 hold > ctrlEvt"));
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_LONG_DUR && inputCurHeld < 3){
      ctrlEvt(btn,3,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 3;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 3 hold > ctrlEvt"));
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_SHORT_DUR && inputCurHeld < 2) {
      //Serial.print(F("ich was ")); Serial.println(inputCurHeld,DEC);
      ctrlEvt(btn,2,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 2;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 2 hold > ctrlEvt"));
      holdLast = now; //starts the repeated presses code going
    }
    //While Up/Dn are being held, send repeated presses to ctrlEvt
    #if defined(INPUT_UPDN_BUTTONS) || defined(INPUT_IMU)
      if((btn==CTRL_UP || btn==CTRL_DN) && inputCurHeld >= 2){
        if((unsigned long)(now-holdLast)>=(inputCurHeld>=3?HOLDSET_FAST_RATE:HOLDSET_SLOW_RATE)){ //could make it nonlinear?
          holdLast = now;
          ctrlEvt(btn,1,inputCurHeld);
        }
      }
    #endif
  }
  //If the button has just been released...
  if(inputCur==btn && !bnow) {
    inputCur = 0;
    //Only act if the button hasn't been stopped
    if(inputCurHeld<10) ctrlEvt(btn,0,inputCurHeld); //hey, the button was released after inputCurHeld
    //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" then 0 after release > ctrlEvt"));
    inputCurHeld = 0;
  }
}
void inputStop(){
  //In some cases, when handling btn evt 1/2/3/4/5, we may call this so following events 2/3/4/5/0 won't cause unintended behavior (e.g. after a fn change, or going in or out of set)
  inputCurHeld = 10;
  //Serial.println(F("ich now 10 per inputStop"));
}

#ifdef INPUT_UPDN_ROTARY
bool rotVel = 0; //high velocity setting (x10 rather than x1)
unsigned long rotLastStep = 0; //timestamp of last completed step (detent)
int rotLastVal = 0;
void checkRot(unsigned long now){
  //Changes in rotary encoder. When rotation(s) occur, will call ctrlEvt to simulate btn presses. During setting, ctrlEvt will take rotVel into account.
  int rotCurVal = rot.read();
  if(rotCurVal!=rotLastVal){ //we've sensed a state change
    rotLastVal = rotCurVal;
    if(rotCurVal>=4 || rotCurVal<=-4){ //we've completed a step of 4 states (this library doesn't seem to drop states much, so this is reasonably reliable)
      inputLast = now; inputLastTODMins = rtcGetTOD();
      if((unsigned long)(now-rotLastStep)<=ROT_VEL_START) rotVel = 1; //kick into high velocity setting (x10)
      else if((unsigned long)(now-rotLastStep)>=ROT_VEL_STOP) rotVel = 0; //fall into low velocity setting (x1)
      rotLastStep = now;
      while(rotCurVal>=4) { rotCurVal-=4; ctrlEvt(CTRL_UP,1,inputCurHeld,rotVel); }
      while(rotCurVal<=-4) { rotCurVal+=4; ctrlEvt(CTRL_DN,1,inputCurHeld,rotVel); }
      rot.write(rotCurVal);
    }
  }
} //end checkRot()
#endif

void checkInputs(){
  unsigned long now = millis(); //this will be the recorded time of any input change
  //Debounce
  if((unsigned long)(now-inputLast)<DEBOUNCE_DUR) return;
  
  //TODO potential issue: if user only means to rotate or push encoder but does both?
  #ifdef INPUT_IMU
    readIMU(); //captures IMU state for checkBtn/readBtn to look at
  #endif
  //checkBtn calls readBtn which will read button and/or IMU as equipped
  //We just need to only call checkBtn if one or the other is equipped
  #if defined(INPUT_BUTTONS) || defined(INPUT_IMU)
    checkBtn(CTRL_SEL,now);
    if(CTRL_ALT>0){ //preprocessor directives don't seem to work for this when e.g. "A7"
      checkBtn(CTRL_ALT,now);
    }
    #if defined(INPUT_UPDN_BUTTONS) || defined(INPUT_IMU)
      checkBtn(CTRL_UP,now);
      checkBtn(CTRL_DN,now);
    #endif
  #endif
  #ifdef INPUT_UPDN_ROTARY
    checkRot();
  #endif
}

void setInputLast(unsigned long increment){
  //Called when other code changes the displayed fn, as though the human user had done it
  //(which in many cases they did, but indirectly, such as via settings page - but also automatic date display)
  //If increment is specified, we just want to move inputLast up a bit
  if(increment) inputLast+=increment;
  //else we want to set both inputLast and the TODMins snapshot to now
  inputLast = millis(); inputLastTODMins = rtcGetTOD();
}
unsigned long getInputLast(){
  return inputLast;
}
int getInputLastTODMins(){
  //Used to ensure paged displays (e.g. calendar) use the same TOD for all pages
  return inputLastTODMins;
}