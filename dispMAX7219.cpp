#include <arduino.h>
#include "arduino-clock.h"

#ifdef DISPLAY_MAX7219 //see arduino-clock.ino Includes section

#include "dispMAX7219.h"
#include <SPI.h> //Arduino - for SPI access to MAX7219
#include <LedControl.h> //Eberhard Farle's LedControl library - http://wayoda.github.io/LedControl

LedControl lc=LedControl(DIN_PIN,CLK_PIN,CS_PIN,NUM_MAX);

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
  for(int i=0; i<NUM_MAX; i++) {
    lc.shutdown(i,false);
    //lc.setIntensity(i,curBrightness);
  }
}

//TODO can we move this into flash with e.g. PROGMEM? or does that happen already?
const char bignumWidth = 5;
byte bignum[50]={ //First four digits - chicagolike 5x8
  B01111110, B11111111, B10000001, B11111111, B01111110, // 0
  B00000000, B00000100, B11111110, B11111111, B00000000, // 1
  B11000010, B11100001, B10110001, B10011111, B10001110, // 2
  B01001001, B10001101, B10001111, B11111011, B01110001, // 3
  B00111000, B00100100, B11111110, B11111111, B00100000, // 4
  B01001111, B10001111, B10001001, B11111001, B01110001, // 5
  B01111110, B11111111, B10001001, B11111001, B01110000, // 6 more squared tail
//B01111100, B11111110, B10001011, B11111001, B01110000, // 6 original
  B00000001, B11110001, B11111001, B00001111, B00000111, // 7
  B01110110, B11111111, B10001001, B11111111, B01110110, // 8
  B00001110, B10011111, B10010001, B11111111, B01111110  // 9 more squared tail
//B00001110, B10011111, B11010001, B01111111, B00111110  // 9 original
};
const char smallnumWidth = 3;
byte smallnum[30]={ //Last two digits - 3x5
  B11111000, B10001000, B11111000, // 0
  B00010000, B11111000, B00000000, // 1 serif
  B11101000, B10101000, B10111000, // 2
  B10001000, B10101000, B11111000, // 3
  B01110000, B01000000, B11111000, // 4
  B10111000, B10101000, B11101000, // 5
  B11111000, B10101000, B11101000, // 6
  B00001000, B00001000, B11111000, // 7
  B11111000, B10101000, B11111000, // 8
  B10111000, B10101000, B11111000  // 9
};

byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start.

void sendToMAX7219(byte posStart, byte posEnd){ //"private"
  //Called by editDisplay and blankDisplay. Needed in lieu of what cycleDisplay does for nixies.
  byte col = 0; //column to start updating, 0 at left
  byte val = 0; //byte to send to the LED column
  byte ci = 0;
  for(byte i=posStart; i<=posEnd; i++){
    if(i>3 && (NUM_MAX<=3 || DISPLAY_SIZE<6)) return; //if 3 or fewer matrices, don't render digits 4 and 5
    col = //h tens at far left
          (i>0? bignumWidth+1: 0)+ //h ones
          (i>1? bignumWidth+2: 0)+ //m tens
          (i>2? bignumWidth+1: 0)+ //m ones
          (i>3? bignumWidth+1: 0)+ //s tens
          (i>4? smallnumWidth+1: 0); //s ones
    for(int j=0; j<(i<4? bignumWidth: smallnumWidth); j++){ //For each column of this number
      ci = ((NUM_MAX*8)-1)-(col+j); //translate from our column count to MAX's column count
      lc.setColumn(
        (NUM_MAX-1)-(ci/8), //display index
        ci%8, //display column index
        (displayNext[i]==15?0:
          (i<4? bignum[displayNext[i]*bignumWidth+j]: smallnum[displayNext[i]*smallnumWidth+j])
        )
      );
    }
  }
}

unsigned long setStartLast = 0; //to control flashing during start
bool setBlinkState = 0;
void cycleDisplay(byte displayBrightness, bool useAmbient, word ambientLightLevel, byte fnSetPg){
  unsigned long now = millis();
  //MAX7219 handles its own cycling - just needs display data updates.
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
      for(int i=0; i<NUM_MAX; i++) {
        lc.setIntensity(i,((curBrightness==1?1:0)-setBlinkState)? BRIGHTNESS_FULL: BRIGHTNESS_DIM);
      }
    }
  }
  
  else { //not setting - defer to what other code has set displayBrightness to (and ambientLightLevel if applicable)
#ifdef LIGHTSENSOR
    //if using ambient lighting and: it has changed, or if returning from setting, or if brightness normality has changed
    if(useAmbient && (curAmbientLightLevel != ambientLightLevel || setStartLast>0 || curBrightness != displayBrightness)) {
      curAmbientLightLevel = ambientLightLevel;
      if(displayBrightness==2) { //If currently normal brightness (so we should display per ambient light level) - otherwise see below code
        //Convert the ambient light level (0-255, per LUX_DIM-LUX_FULL) to the corresponding brightness value for the actual display hardware, within the desired range (BRIGHTNESS_DIM-BRIGHTNESS_FULL)
        for(int i=0; i<NUM_MAX; i++) {
          lc.setIntensity(i,BRIGHTNESS_DIM + ((long)curAmbientLightLevel * (BRIGHTNESS_FULL - BRIGHTNESS_DIM) /255));
        }
      }
    }
#endif
    //if brightness normality has changed
    if(curBrightness != displayBrightness) {
      curBrightness = displayBrightness;
      if(curBrightness==0) for(int i=0; i<NUM_MAX; i++) { lc.clearDisplay(i); } //force dark
      if(curBrightness==1) for(int i=0; i<NUM_MAX; i++) { lc.setIntensity(i,BRIGHTNESS_DIM); }
      if(curBrightness==2) { //normal brightness - only set if no light sensor, or not using light sensor
#ifdef LIGHTSENSOR
        if(!useAmbient) for(int i=0; i<NUM_MAX; i++) { lc.setIntensity(i,BRIGHTNESS_FULL); }
#else
        for(int i=0; i<NUM_MAX; i++) { lc.setIntensity(i,BRIGHTNESS_FULL); }
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
  if(posEnd==255) posEnd=posStart; //single digit change
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
  sendToMAX7219(posStart,posEnd);
  //cycleDisplay(); //fixes brightness - can we skip this?
}
void blankDisplay(byte posStart, byte posEnd, byte fade){
  if(posEnd==255) posEnd=posStart; //single digit change
  for(byte i=posStart; i<=posEnd; i++) { displayNext[i]=15; }
  sendToMAX7219(posStart,posEnd);
  //cycleDisplay(); //fixes brightness - can we skip this?
}

//void startScroll() {}

void displayBlink(){
  for(int i=0; i<NUM_MAX; i++) { lc.clearDisplay(i); }
  displayBlinkStart = millis();
}

//void checkEffects(bool force){}

#endif //DISPLAY_MAX7219