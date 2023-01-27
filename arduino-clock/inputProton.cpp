//"Proton" inputs are a more diverse set of controls, namely the buttons and switches on a Proton 320 clock radio
//todo pull network support stuff from simple

#include <arduino.h>
#include "arduino-clock.h"

#ifdef INPUT_PROTON //see arduino-clock.ino Includes section

#include "inputProton.h"

//Needs access to RTC timestamps
#ifdef RTC_DS3231
  #include "rtcDS3231.h" //if RTC_DS3231 is defined in config – for an I2C DS3231 RTC module
#endif
#ifdef RTC_MILLIS
  #include "rtcMillis.h" //if RTC_MILLIS is defined in config – for a fake RTC based on millis
#endif
//Needs to be able to start network
#if defined(NETWORK_NINA)
  #include "networkNINA.h" //enables WiFi/web-based config/NTP sync on Nano 33 IoT WiFiNINA
#elif defined(NETWORK_ESP32)
  #include "networkESP32.h" //enables WiFi/web-based config/NTP sync on esp32 //TODO
#endif
//Needs access to storage
#include "storage.h" //for persistent storage - supports both AVR EEPROM and SAMD flash (including esp32? TODO find out)
//Needs access to display to blink it
#ifdef DISPLAY_NIXIE
  #include "dispNixie.h" //if DISPLAY_NIXIE is defined in config - for a SN74141-multiplexed nixie array
#endif
#ifdef DISPLAY_MAX7219
  #include "dispMAX7219.h" //if DISPLAY_MAX7219 is defined in config - for a SPI MAX7219 8x8 LED array
#endif
#ifdef DISPLAY_HT16K33
  #include "dispHT16K33.h" //if DISPLAY_HT16K33 is defined in config - for an I2C 7-segment LED display
#endif

#ifndef HOLDSET_SLOW_RATE
#define HOLDSET_SLOW_RATE 125
#endif
#ifndef HOLDSET_FAST_RATE
#define HOLDSET_FAST_RATE 20
#endif
#ifndef DEBOUNCE_DUR
#define DEBOUNCE_DUR 150 //ms
#endif

//For momentary inputs
byte inputCur = 0; //Button currently in use - only one allowed at a time
byte inputCurHeld = 0; //Button hold thresholds: 0=none, 1=unused, 2=short, 3=long, 4=verylong, 5=superlong, 10=set by inputStop()

//For switch inputs
//We'll have var representations of each, so we can detect changes
bool inputAl1 = false;
bool inputAl2 = false;
bool inputAl1R = false; //TODO we may not need these - not sure if the microcontroller handles
bool inputAl2R = false;
bool inputSw0 = false; //rear switch bit 0
bool inputSw1 = false; //rear switch bit 1

unsigned long inputLast = 0; //When an input last took place, millis()
int inputLastTODMins = 0; //When an input last took place, time of day. Used in paginated functions so they all reflect the time of day when the input happened.

bool initInputs(){
  //TODO are there no "loose" pins left floating after this? per https://electronics.stackexchange.com/q/37696/151805
  
  pinMode(CTRL_OFF, INPUT_PULLUP); //momentary
  pinMode(CTRL_ON, INPUT_PULLUP); //momentary
  pinMode(CTRL_AL1, INPUT_PULLUP); //switch on/off
  pinMode(CTRL_AL2, INPUT_PULLUP); //switch on/off
  pinMode(CTRL_AL1R, INPUT_PULLUP); //switch radio/buzzer
  pinMode(CTRL_AL2R, INPUT_PULLUP); //switch radio/buzzer
  pinMode(CTRL_SNOOZE, INPUT_PULLUP); //momentary (akin to SEL, for now)
  pinMode(CTRL_SLEEP, INPUT_PULLUP); //momentary (akin to ALT, for now)
  pinMode(CTRL_ADJ_UP, INPUT_PULLUP); //momentary
  pinMode(CTRL_ADJ_DN, INPUT_PULLUP); //momentary
  pinMode(CTRL_SW0, INPUT_PULLUP); //rear switch bit 0
  pinMode(CTRL_SW1, INPUT_PULLUP); //rear switch bit 1
  
  //Set switch input bools
  inputAl1 = readCtrl(CTRL_AL1); setAlarmState(inputAl1? 2: 0); //2 = on without skip. TODO calculate skip here?
  inputAl2 = readCtrl(CTRL_AL2); //TODO implement Al2
  inputAl1R = readCtrl(CTRL_AL1R);
  inputAl2R = readCtrl(CTRL_AL2R);
  inputSw0 = readCtrl(CTRL_SW0); //TODO adopt SW
  inputSw1 = readCtrl(CTRL_SW1);
  
  //Check to see if CTRL_SNOOZE is held at init - facilitates version number display and EEPROM hard init
  delay(100); //prevents the below from firing in the event there's a capacitor stabilizing the input, which can read low falsely
  if(readCtrl(CTRL_SNOOZE)){ inputCur = CTRL_SNOOZE; return true; }
  else return false;
}

bool readCtrl(byte ctrl){
  //TODO any analog-only pins? If so, return analogRead(ctrl)<100
  return !(digitalRead(ctrl)); //false (low) when pressed
}

unsigned long holdLast;
void checkMomentary(byte ctrl, unsigned long now){
  //Polls for changes in momentary buttons (or IMU positioning), LOW = pressed.
  //When a button event has occurred, will call ctrlEvt.
  //Only called by checkInputs() and only for inputs configured as button and/or IMU.
  bool bnow = readCtrl(ctrl);
  //If the button has just been pressed, and no other buttons are in use...
  if(inputCur==0 && bnow) {
    // Serial.print(F("ctrl "));
    // Serial.print(ctrl,DEC);
    // Serial.println(F(" pressed"));
    inputCur = ctrl; inputCurHeld = 0; inputLast = now; inputLastTODMins = rtcGetTOD();
    //Serial.println(); Serial.println(F("ich now 0 per press"));
    ctrlEvt(ctrl,1,inputCurHeld); //hey, the button has been pressed
    //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after press > ctrlEvt"));
  }
  //If the button is being held...
  if(inputCur==ctrl && bnow) {
    //If the button has passed a hold duration threshold... (ctrlEvt will only act on these for Sel/Alt)
    if((unsigned long)(now-inputLast)>=CTRL_HOLD_SUPERLONG_DUR && inputCurHeld < 5){
      ctrlEvt(ctrl,5,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 5;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 5 hold > ctrlEvt"));
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_VERYLONG_DUR && inputCurHeld < 4){
      ctrlEvt(ctrl,4,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 4;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 4 hold > ctrlEvt"));
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_LONG_DUR && inputCurHeld < 3){
      ctrlEvt(ctrl,3,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 3;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 3 hold > ctrlEvt"));
    }
    else if((unsigned long)(now-inputLast)>=CTRL_HOLD_SHORT_DUR && inputCurHeld < 2) {
      //Serial.print(F("ich was ")); Serial.println(inputCurHeld,DEC);
      ctrlEvt(ctrl,2,inputCurHeld); if(inputCurHeld<10) inputCurHeld = 2;
      //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" after 2 hold > ctrlEvt"));
      holdLast = now; //starts the repeated presses code going
    }
    //While Up/Dn are being held, send repeated presses to ctrlEvt
    #if defined(INPUT_UPDN_BUTTONS) || defined(INPUT_IMU)
      if((ctrl==CTRL_UP || ctrl==CTRL_DN) && inputCurHeld >= 2){
        if((unsigned long)(now-holdLast)>=(inputCurHeld>=3?HOLDSET_FAST_RATE:HOLDSET_SLOW_RATE)){ //could make it nonlinear?
          holdLast = now;
          ctrlEvt(ctrl,1,inputCurHeld);
        }
      }
    #endif
  }
  //If the button has just been released...
  if(inputCur==ctrl && !bnow) {
    inputCur = 0;
    //Only act if the button hasn't been stopped
    if(inputCurHeld<10) ctrlEvt(ctrl,0,inputCurHeld); //hey, the button was released after inputCurHeld
    //Serial.print(F("ich now ")); Serial.print(inputCurHeld,DEC); Serial.println(F(" then 0 after release > ctrlEvt"));
    inputCurHeld = 0;
  }
}

void checkSwitch(byte ctrl) {
  //TODO is polling really the best way? possible to do interrupts?
  //Whereas checkMomentary calls ctrlEvt, this handles events directly
  //These also take precedence over other inputs, and therefore call inputStop
  switch(ctrl) {
    case CTRL_AL1:
      if(readCtrl(ctrl) != inputAl1) { inputAl1 = !inputAl1;
        inputStop();
        setAlarmState(inputAl1? 2: 0);
        goToFn(FN_ALARM); //temporarily display (will clear per checkRTC) - TODO can you still set?
      } break;
    case CTRL_AL2:
      if(readCtrl(ctrl) != inputAl2) { inputAl2 = !inputAl2;
        //inputStop();
        //TODO set alarm state and go to FN_ALARM2
      } break;
    case CTRL_AL1R:
      if(readCtrl(ctrl) != inputAl1R) { inputAl1R = !inputAl1R;
        //inputStop();
        //TODO do we need to handle this in code?
      } break;
    case CTRL_AL2R:
      if(readCtrl(ctrl) != inputAl2R) { inputAl2R = !inputAl2R;
        //inputStop();
        //TODO do we need to handle this in code?
      } break;
    case CTRL_SW0:
      if(readCtrl(ctrl) != inputSw0) { inputSw0 = !inputSw0;
        //inputStop();
        //TODO are we using this?
      } break;
    case CTRL_SW1:
      if(readCtrl(ctrl) != inputSw1) { inputSw1 = !inputSw1;
        //inputStop();
        //TODO are we using this?
      } break;
    default: break;
  }
}

void inputStop(){
  //In some cases, when handling momentary evt 1/2/3/4/5, we may call this so following events 2/3/4/5/0 won't cause unintended behavior (e.g. after a fn change, or going in or out of set)
  inputCurHeld = 10;
  //Serial.println(F("ich now 10 per momentaryStop"));
}

void checkInputs(){
  unsigned long now = millis(); //this will be the recorded time of any input change
  //Debounce
  if((unsigned long)(now-inputLast)<DEBOUNCE_DUR) return;
  
  //These will call ctrlEvt
  checkMomentary(CTRL_OFF,now);
  checkMomentary(CTRL_ON,now);
  checkMomentary(CTRL_SNOOZE,now);
  checkMomentary(CTRL_SLEEP,now);
  checkMomentary(CTRL_ADJ_UP,now);
  checkMomentary(CTRL_ADJ_DN,now);

  //These will handle directly
  checkSwitch(CTRL_AL1);
  checkSwitch(CTRL_AL2);
  checkSwitch(CTRL_AL1R);
  checkSwitch(CTRL_AL2R);
  checkSwitch(CTRL_SW0);
  checkSwitch(CTRL_SW1);
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

void ctrlEvt(byte ctrl, byte evt, byte evtLast, bool velocity){
  //Handle events from momentary controls, based on current fn and set state.
  //Moved here from main code, since the functionality here reflects the available controls (cf. inputSimple).
  
  //evt: 1=press, 2=short hold, 3=long hold, 4=verylong, 5=superlong, 0=release.
  //We only handle press evts for CTRL_UP and CTRL_DN, due to legacy of inputSimple potentially using encoders here,
  //and that's the only evt encoders generate, so input.cpp sends repeated presses if these are held.
  //But for other momentary controls, we can handle different hold states here.

  //If the version display is showing, ignore all else until CTRL_SNOOZE is released (cancel) or long-held (cancel and eeprom reset)
  if(getCurFn()==FN_VERSION){
    if(ctrl==CTRL_SNOOZE && (evt==0 || evt==5)){ //release or superlong hold
      if(evt==5){ initEEPROM(true); commitEEPROM(); } //superlong hold: reset EEPROM
      setCurFn(FN_TOD);
      inputStop(); updateDisplay();
      #ifdef NETWORK_H
        initNetwork(); //we didn't do this earlier since the wifi connect makes the clock hang
      #endif
      return;
    } else {
      return; //ignore other momentary controls
    }
  }
  
  //Handle presses of specific, dedicated controls
  if(ctrl==CTRL_ON) {
    if(evt==1) switchPower(1); //stops signal, snooze, and sleep timer. Do we need to change fn or exit setting?
    inputStop();
    return;
  }
  
  if(ctrl==CTRL_OFF) {
    if(evt==1) switchPower(0); //stops signal, snooze, and sleep timer. 
    inputStop();
    return; 
  }
  
  //This has been moved below
//   if(ctrl==CTRL_SLEEP) {
//     if(evt==1) {
//       timerStart(); //will start it from sleep duration, if stopped
//       goToFn(FN_TIMER);
//     }
//     //don't inputstop, because we might set this way
//     return;
//   }
  
//   //A more hardcore approach
//   if(evt==1 && (ctrl==CTRL_ON || ctrl==CTRL_OFF || ctrl==CTRL_SLEEP)) {
//     signalStop();
//     stopSnooze();
//     timerClear(); //sleep //TODO confirm it's in sleep mode
//     //networkStop(); something of this nature?????
//     clearSet();
//     goToFn(FN_TOD);
//     inputStop();
//     //Then do the specific thing
//     if(ctrl==CTRL_ON) {
//       
//     }
//   }

  //If the signal is going and CTRL_SNOOZE is pressed, it should silence and, if from alarm, start snooze
  if(getSignalRemain()>0 && evt==1 && ctrl==CTRL_SNOOZE) {
    signalStop();
    if(getSignalSource()==FN_ALARM || getSignalSource()==FN_ALARM2) { //If this was the alarm
      if((readEEPROM(42,false)==1) {
        startSnooze();
      }
    }
    inputStop();
    return;
  }
  
  //If the snooze is going, pressing CTRL_OFF should cancel it, with a signal - this is covered by switchPower()

  // TODO NIXIE potentially - solve in inputSimple first - probably move to main code
  
  //Is it a press for an un-off?
  startUnoff(); //always do this, so continued button presses during an unoff keep the display alive
  if(getDisplayBrightness()==0 && evt==1) {
    updateDisplay();
    inputStop();
    return;
  }
  
  #ifdef NETWORK_H
    //CTRL_SNOOZE very long hold: start admin
    if(evt==4 && ctrl==CTRL_SNOOZE) {
      networkStartAdmin();
      return;
    }
    //CTRL_SNOOZE super long hold: start AP (TODO would we rather it forget wifi?)
    if(evt==5 && ctrl==CTRL_SNOOZE) {
      networkStartAP();
      return;
    }
  #endif
  
  if(getCurFn() < FN_OPTS) { //normal fn running/setting (not in settings menu)

    if(evt==3 && ctrl==CTRL_SNOOZE) { //CTRL_SNOOZE long hold: enter settings menu
      //inputStop(); commented out to to enable evt==4 and evt==5 per above
      setCurFn(FN_OPTS);
      clearSet(); //don't need updateDisplay() here because this calls updateRTC with force=true
      return;
    }
    
    if(!getFnIsSetting()) { //fn running
      if(ctrl==CTRL_SNOOZE && evt==2) { //CTRL_SNOOZE hold: enter setting mode
        setByFn();
        //could consider not doing this for FN_TIMER, to force user to use CTRL_SLEEP, but meh
        return;
      }
      else if(ctrl==CTRL_SNOOZE && evt==0) { //sel release (
        //Proton does not handle adj press here (cf. inputSimple) but could, if we decide to move away from hold-set concept TODO
        //we can't handle sel press here because, if attempting to enter setting mode, it would switch the fn first
        
        //we will be changing fn, but unlike inputSimple, we don't fnScroll here
        if(getCurFn()==FN_TIMER && !getTimerRun()) timerClear(); //if timer is stopped, clear it - but probably won't be used in Proton context
        if(getCurFn()==FN_TOD) goToFn(FN_DATE); //TODO goToFn vs directly changing function
        else goToFn(FN_TOD);
        checkRTC(true); //updates display
      } //end sel release
      else if(ctrl==CTRL_SLEEP) {
        if(evt==1) { //activate
          timerStart(); //will start it from sleep duration, if stopped
          goToFn(FN_TIMER);
        }
        if(evt==2) {
          setByFn();
          inputStop(); //I think?
        }
        //if 0 it's fine
      } //end alt
    } //end fn running

    else { //fn setting
      if(evt==1) { //press
        //TODO could we do release/shorthold on CTRL_SNOOZE so we can exit without making changes?
        //currently no, because we don't inputStop() when short hold goes into fn setting, in case long hold may go to settings menu
        //so we can't handle a release because it would immediately save if releasing from the short hold.
        //Consider recording the input start time when going into fn setting so we can distinguish its release from a future one
        //TODO the above can be revisited now that we pass evtLast
        if(ctrl==CTRL_SNOOZE || ctrl==CTRL_SLEEP) { //CTRL_SNOOZE or CTRL_SLEEP push: go to next setting or save and exit setting mode
          inputStop(); //not waiting for hold, so can stop listening here
          setByFn();
          return;
        } //end CTRL_SNOOZE push
        if(ctrl==CTRL_UP) doSet(velocity ? 10 : 1);
        if(ctrl==CTRL_DN) doSet(velocity ? -10 : -1);
      } //end if evt==1
    } //end fn setting
    
  } //end normal fn running/setting
  
  else { //settings menu setting - to/from EEPROM
    
    byte opt = fn-FN_OPTS; //current setting index
    
    if(evt==2 && ctrl==CTRL_SNOOZE) { //CTRL_SNOOZE short hold: exit settings menu
      inputStop();
      setOpt(opt); clearSet(); //if we were setting, writes setting val to EEPROM
      //TODO test this - if we were not setting, it should start but then immediately cancel via
      setCurFn(FN_TOD);
      //we may have changed lat/long/GMT/DST settings so recalc those
      calcSun(); //TODO pull from clock
      isDSTByHour(rtcGetYear(),rtcGetMonth(),rtcGetDate(),rtcGetHour(),true);
      return;
    }
    
    if(!getFnIsSetting()){ //setting number
      if(ctrl==CTRL_SNOOZE && evt==0 && evtLast<3) { //CTRL_SNOOZE release (but not after holding to get into the menu): enter setting value
        setOpt(opt);
      }
      if(ctrl==CTRL_UP && evt==1) fnOptScroll(1); //next one up or cycle to beginning
      if(ctrl==CTRL_DN && evt==1) fnOptScroll(0); //next one down or cycle to end?
      updateDisplay();
    } //end setting number

    else { //setting value
      if(ctrl==CTRL_SNOOZE && evt==0) { //CTRL_SNOOZE release: save value and exit
        setOpt(opt);
      }
      if(evt==1 && (ctrl==CTRL_UP || ctrl==CTRL_DN)){
        if(ctrl==CTRL_UP) doSet(velocity ? 10 : 1);
        if(ctrl==CTRL_DN) doSet(velocity ? -10 : -1);
        updateDisplay(); //may also make sounds for sampling
      }
    }  //end setting value
  } //end settings menu setting
  
} //end ctrlEvt

#endif //INPUT_SIMPLE