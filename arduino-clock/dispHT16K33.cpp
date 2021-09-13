#include <arduino.h>
#include "arduino-clock.h"

#ifdef DISPLAY_HT16K33 //see arduino-clock.ino Includes section

#include "dispHT16K33.h"
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

Adafruit_7segment matrix = Adafruit_7segment();

//these can be overridden in your config .h
#ifndef BRIGHTNESS_FULL
#define BRIGHTNESS_FULL 15 //out of 0-15
#endif
#ifndef BRIGHTNESS_DIM
#define BRIGHTNESS_DIM 0
#endif

byte curBrightness = 255; //represents current display normal/dim/off state – compare to displayBrightness as passed to cycleDisplay. Start at 255 to force an adjustment at startup
#ifdef LIGHTSENSOR
word curAmbientLightLevel = 0; //represents current ambient light level reading (as tweened by main code)
#endif

unsigned long displayBlinkStart = 0; //when nonzero, display should briefly blank

void initDisplay(){
  matrix.begin(DISPLAY_ADDR);
  //initial brightness will be set at first cycleDisplay
}

byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start.

void sendToHT16K33(byte posStart, byte posEnd){ //"private"
  //Called by editDisplay and blankDisplay. Needed in lieu of what cycleDisplay does for nixies.
  for(byte i=posStart; i<=posEnd; i++){
    if(i>=DISPLAY_SIZE) return;
    if(displayNext[i]>9) matrix.writeDigitRaw((i>=2?i+1:i),0); //blank - skip pos 2 (colon)
    else matrix.writeDigitNum((i>=2?i+1:i),displayNext[i]);
  }
  matrix.writeDisplay();
}

unsigned long setStartLast = 0; //to control flashing during start
bool setBlinkState = 0;
void cycleDisplay(byte displayBrightness, bool useAmbient, word ambientLightLevel, byte fnSetPg){
  unsigned long now = millis();
  //HT16K33 handles its own cycling - just needs display data updates.
  //But we do need to check if the blink should be over, and whether brightness has changed (or should change, per setting).
  
  //If we're in the middle of a blink, see if it's time to end it
  if(displayBlinkStart){
    if((unsigned long)(now-displayBlinkStart)>=500){ displayBlinkStart = 0; sendToHT16K33(0,5); }
  }
  
  //Check if it's time to change brightness - either due to setting flashing or change in displayBrightness/ambientLightLevel inputs
  if(fnSetPg>0) { //setting - dim for every other 500ms - using the brightest and dimmest states of the display (TODO too crude?)
    if(setStartLast==0) {
      setStartLast = now;
      setBlinkState = 1;
      //When starting, if we're using ambient at less than 2/3 brightness, set curBrightness to dim, so we'll start bright
      if(useAmbient && ambientLightLevel<170) curBrightness = 1;
    }
    bool blinkModulus = ((unsigned long)(now-setStartLast)/500)%2;
    if(setBlinkState!=blinkModulus) { //will occur every 500ms
      setBlinkState = blinkModulus;
      //If we were dim at start (curBrightness), invert setBlinkState to "start" at 1
      matrix.setBrightness(((curBrightness==1?1:0)-setBlinkState)? BRIGHTNESS_FULL: BRIGHTNESS_DIM);
    }
  }
  
  else { //not setting - defer to what other code has set displayBrightness to (and ambientLightLevel if applicable)
#ifdef LIGHTSENSOR
    //if using ambient lighting and: it has changed, or if returning from setting, or if brightness normality has changed
    if(useAmbient && (curAmbientLightLevel != ambientLightLevel || setStartLast>0 || curBrightness != displayBrightness)) {
      curAmbientLightLevel = ambientLightLevel;
      if(displayBrightness==2) { //If currently normal brightness (so we should display per ambient light level) - otherwise see below code
        //Convert the ambient light level (0-255, per LUX_DIM-LUX_FULL) to the corresponding brightness value for the actual display hardware, within the desired range (BRIGHTNESS_DIM-BRIGHTNESS_FULL)
        matrix.setBrightness(BRIGHTNESS_DIM + ((long)curAmbientLightLevel * (BRIGHTNESS_FULL - BRIGHTNESS_DIM) /255));
      }
    }
#endif
    //if brightness normality has changed
    if(curBrightness != displayBrightness) {
      curBrightness = displayBrightness;
      if(curBrightness==0) for(int i=0; i<DISPLAY_SIZE; i++) { matrix.writeDigitRaw((i>=2?i+1:i),0); } //force dark
      if(curBrightness==1) matrix.setBrightness(BRIGHTNESS_DIM);
      if(curBrightness==2) { //normal brightness - only set if no light sensor, or not using light sensor
#ifdef LIGHTSENSOR
        if(!useAmbient) matrix.setBrightness(BRIGHTNESS_FULL);
#else
        matrix.setBrightness(BRIGHTNESS_FULL);
#endif
      }
    }
    if(setStartLast>0) setStartLast=0; //remove setting flag if needed
  } //end if not setting
  
} //end cycleDisplay

void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade){
  if(curBrightness==-1) return;
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank the digit
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4-digit displays --> 0015)
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
  }
  sendToHT16K33(posStart,posEnd); //TODO consider moving this to cycleDisplay if the value has changed - better sync with brightness change?
  
  // for(int i=0; i<DISPLAY_SIZE; i++) {
  //   if(displayNext[i]>9) Serial.print(F("-"));
  //   else Serial.print(displayNext[i],DEC);
  // }
  // Serial.println();
}
void blankDisplay(byte posStart, byte posEnd, byte fade){
  for(byte i=posStart; i<=posEnd; i++) { displayNext[i]=15; }
  sendToHT16K33(posStart,posEnd);
  //cycleDisplay(); //fixes brightness - can we skip this?
}

//void startScroll() {}

void displayBlink(){
  //TODO why doesn't this work
  for(int i=0; i<DISPLAY_SIZE; i++) matrix.writeDigitRaw((i>=2?i+1:i),0); //force dark
  matrix.writeDisplay();
  displayBlinkStart = millis();
}

//void checkEffects(bool force){}

#endif //DISPLAY_HT16K33