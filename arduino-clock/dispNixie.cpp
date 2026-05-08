#include <arduino.h>
#include "arduino-clock.h"

#ifdef DISPLAY_NIXIE //see arduino-clock.ino Includes section

#include "dispNixie.h"

#include "storage.h" //to read refresh rate

// Display cycling code derived from http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {OUT_A1,OUT_A2,OUT_A3,OUT_A4};
byte binOutB[4] = {OUT_B1,OUT_B2,OUT_B3,OUT_B4};
//3 pins out to anode channel switches
byte anodes[3] = {ANODE_1,ANODE_2,ANODE_3};

//these can be overridden in your config .h
#ifndef DIGIT_DUR_FULL
#define DIGIT_DUR_FULL 5000 //micros - The period of time each digit (or fading pair of digits) will appear at full brightness. This determines the overall refresh rate of the display: when it is at less than full brightness, the digit(s) will appear for a portion of this time, with the rest taken up by a blanking period; this duty cycling creates the dim effect.
//Max is ~16380 (16.38ms) since delayMicroseconds max value is 16384.
#endif
#ifndef DIGIT_DUR_DIM
#define DIGIT_DUR_DIM 50 //micros - The period of time each digit (or fading pair of digits) will appear when the display is at its dimmest.
#endif

byte curBrightness = 255; //represents current display normal/dim/off state – compare to displayBrightness as passed to cycleDisplay. Start at 255 to force an adjustment at startup
int digitDur = 0; //The calculated duration each digit (or fading pair of digits) should appear, per intended display brightness – somewhere between DIGIT_DUR_FULL and DIGIT_DUR_DIM. Stored here so we don't have to calculate it every cycle - since tubes don't hang onto it like LED controllers do. We will also set digitDurLast to match, in case fading is not in effect.
#ifdef LIGHTSENSOR
word curAmbientLightLevel = 0;
#endif
int digitDurNext = 0; //micros - during fade, incoming digit's portion of digitDur
int digitDurLast = 0; //micros - during fade, outgoing digit's portion of digitDur
unsigned long fadeStart = 0; //micros - when the last digit fade was started

unsigned long displayBlinkStart = 0; //micros - when nonzero, display should briefly blank

byte cleanRemain = 0; //anti-cathode-poisoning clean timeout counter, increments at CLEAN_SPEED ms (see loop()). Start at 11 to run at clock startup

int8_t scrollRemain = 0; //"frames" of scroll – signed byte - 0=not scrolling, >0=coming in, <0=going out, -128=scroll out at next change.

byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start. Change this to change display.
byte displayLast[6] = {11,11,11,11,11,11}; //for noticing changes to displayNext and fading the display to it
byte scrollDisplay[6] = {15,15,15,15,15,15}; //For animating a value into displayNext from right, and out to left

unsigned long pollCleanLast = 0; //every CLEAN_SPEED ms
unsigned long pollScrollLast = 0; //every SCROLL_SPEED ms

void decToBin(bool binVal[], byte i){ //"private"
  //binVal is a reference (modify in place) of a binary number bool[4] with values [1,2,4,8]
  if(i<0 || i>15) i=15; //default value, turns tubes off
  binVal[3] = int(i/8)%2;
  binVal[2] = int(i/4)%2;
  binVal[1] = int(i/2)%2;
  binVal[0] = i%2;
} //end decToBin()

void setCathodes(byte decValA, byte decValB){ //"private"
  bool binVal[4]; //4-bit binary number with values [1,2,4,8]
  decToBin(binVal,decValA); //have binary value of decVal set into binVal
  for(byte i=0; i<4; i++) digitalWrite(binOutA[i],binVal[i]); //set bin inputs of SN74141
  decToBin(binVal,decValB);
  for(byte i=0; i<4; i++) digitalWrite(binOutB[i],binVal[i]); //set bin inputs of SN74141
} //end setCathodes()

void initDisplay(){
  for(byte i=0; i<4; i++) { pinMode(binOutA[i],OUTPUT); pinMode(binOutB[i],OUTPUT); }
  for(byte i=0; i<3; i++) { pinMode(anodes[i],OUTPUT); }
}

char getCommand() {
  char c = '\0';
  if(Serial.available()) c = Serial.read();
  return c;
}

bool useOurOwn = false;
unsigned long setStart = 0; //**millis** - to control flashing during start
bool setBlinkState = 0;
void cycleDisplay(byte displayBrightness, bool useAmbient, word ambientLightLevel, byte fnSetPg){
  
  if(useOurOwn) {
    useAmbient = true;
    ambientLightLevel = curAmbientLightLevel;
  }
  char command = getCommand();
  switch(command) {
    case 'q': ambientLightLevel = 255; Serial.println(F("bright")); break;
    case 'a': ambientLightLevel = 128; Serial.println(F("medium")); break;
    case 'z': ambientLightLevel = 0; Serial.println(F("medium")); break;
    case '9': ambientLightLevel = 9; Serial.println(F("dark")); break;
    case '8': ambientLightLevel = 8; Serial.println(F("dark")); break;
    case '7': ambientLightLevel = 7; Serial.println(F("dark")); break;
    case '6': ambientLightLevel = 6; Serial.println(F("dark")); break;
    case '5': ambientLightLevel = 5; Serial.println(F("dark")); break;
    case '4': ambientLightLevel = 4; Serial.println(F("dark")); break;
    case '3': ambientLightLevel = 3; Serial.println(F("dark")); break;
    case '2': ambientLightLevel = 2; Serial.println(F("dark")); break;
    case '1': ambientLightLevel = 1; Serial.println(F("dark")); break;
    case 'o': useOurOwn = !useOurOwn; if(useOurOwn) Serial.println(F("use our own: yes")); else Serial.println(F("use our own: no"));  break;
    // case 'w': waitSpeed = 0; Serial.println(F("normal rate")); break;
    // case 's': waitSpeed = 1; Serial.println(F("slowish rate")); break;
    // case 'x': waitSpeed = 2; Serial.println(F("slowww rate")); break;
    // case 'c': fadeTime = 0; Serial.println(F("no fading")); break;
    // case 'd': fadeTime = 20; Serial.println(F("20ms fading")); break;
    // case 'e': fadeTime = 50; Serial.println(F("50ms fading")); break;
  }
  
  unsigned long now = micros(); if(now==0) now++;
  
  //See if it's time to stop the blink
  if(displayBlinkStart){
    if((unsigned long)(now-displayBlinkStart)<250000){ delayMicroseconds(DIGIT_DUR_FULL*3); return; }
    // The delay is to make cycleDisplay take up the same amount of loop time it usually does, for each of the 3 pairs. Not sure if necessary.
    else displayBlinkStart = 0;
    //TODO this blanking is broken
    //TODO bring in the sampling from the chip
    //TODO bring back the scrolling and cleaning
  }
  
  //Check if it's time to change brightness - either due to setting flashing or change in displayBrightness/ambientLightLevel inputs
  if(fnSetPg>0) { //setting - dim for every other 500ms - using the brightest and dimmest states of the display (TODO too crude?)
    if(setStart==0) {
      setStart = millis();
      setBlinkState = 1; //forces a brightness shift as soon as it starts, since modulus starts at 0
      //When starting, if we're using ambient at less than 2/3 brightness, set curBrightness to dim, so we'll start bright
      if(useAmbient && ambientLightLevel<170) curBrightness = 1;
    }
    bool blinkModulus = ((unsigned long)(millis()-setStart)/500)%2;
    if(setBlinkState!=blinkModulus) { //will occur every 500ms
      setBlinkState = blinkModulus;
      //If we were dim at start (curBrightness), invert setBlinkState to "start" at 1
      digitDur = digitDurLast = ((curBrightness==1?1:0)-setBlinkState)? DIGIT_DUR_FULL: DIGIT_DUR_DIM;
      //Serial.print(F("New digitDur set: ")); Serial.println(digitDur,DEC);
    }
  }
   
  else { //not setting - defer to what other code has set displayBrightness to (and ambientLightLevel if applicable)  
#ifdef LIGHTSENSOR
    //if using ambient lighting and: it has changed, or if returning from setting, or if brightness normality has changed
    if(useAmbient && (curAmbientLightLevel != ambientLightLevel || setStart>0 || curBrightness != displayBrightness)) {
      curAmbientLightLevel = ambientLightLevel;
      if(displayBrightness==2) { //If currently normal brightness (so we should display per ambient light level) - otherwise see below code
        //Convert the ambient light level (0-255, per LUX_DIM-LUX_FULL) to the corresponding brightness value for the actual display hardware, within the desired range (DIGIT_DUR_DIM-DIGIT_DUR_FULL)
        digitDur = digitDurLast = DIGIT_DUR_DIM + ((long)curAmbientLightLevel * (DIGIT_DUR_FULL - DIGIT_DUR_DIM) /255);
        //Serial.print(F("New digitDur amb: ")); Serial.println(digitDur,DEC);
      }
    }
#endif
    //if brightness normality has changed
    if(curBrightness != displayBrightness) {
      curBrightness = displayBrightness;
      if(curBrightness==0) {
        digitDur = digitDurLast = 0;
        //Serial.print(F("New digitDur nor0: ")); Serial.println(digitDur,DEC); 
      }
      if(curBrightness==1) {
        digitDur = digitDurLast = DIGIT_DUR_DIM;
        //Serial.print(F("New digitDur nor1: ")); Serial.println(digitDur,DEC); 
      }
      if(curBrightness==2) { //normal brightness - only set if no light sensor, or not using light sensor
#ifdef LIGHTSENSOR
        if(!useAmbient) digitDur = digitDurLast = DIGIT_DUR_FULL;
#else
        digitDur = digitDurLast = DIGIT_DUR_FULL;
        //Serial.print(F("New digitDur nor2: ")); Serial.println(digitDur,DEC); 
#endif
      }
    }
    if(setStart>0) setStart=0; //remove setting flag if needed
  } //end if not setting
  
  //Controlling the duration of the fading digit pairs
  if(fadeStart==0) { //not fading - time to fade or change?
    if(readEEPROM(20,false)) { //fading enabled - if value has changed, set a fade start
      for(byte i=0; i<6; i++) if(displayNext[i] != displayLast[i]) {
        //Serial.print(F("starting fade with digitDur=")); Serial.println(digitDur,DEC);
        fadeStart = now; break;
      }
    } else { //fading disabled - simply set new value into last one
      for(byte i=0; i<6; i++) displayLast[i] = displayNext[i];
    }
  }
  if(fadeStart!=0) { //currently fading
    // Serial.print(F("yes fadeStart is ")); Serial.println(fadeStart,DEC);
    //let the next digit steal some display time from the last digit
    //ex: if fade time (from EEPROM) is 20/100sec (200000 micros), and digitDur (next+last) is 5000 micros:
    /*
    at    000 micros, (     0*5000)/200000 = 0
    at   5000 micros, (  5000*5000)/200000 = 125
    at  10000 micros, ( 10000*5000)/200000 = 250
    at  50000 micros, ( 50000*5000)/200000 = 1250
    at 100000 micros, (100000*5000)/200000 = 2500
    at 200000 micros, (200000*5000)/200000 = 5000
    */
    //We can only run this when fading is enabled, otherwise divide by zero here
    unsigned long base = (unsigned long)readEEPROM(20,false)*10000;
    if((unsigned long)(now-fadeStart) > base) { //fade is over - next becomes last
      fadeStart = 0;
      digitDurNext = 0;
      digitDurLast = digitDur;
      for(byte j=0; j<6; j++) displayLast[j] = displayNext[j];
    } else {
      digitDurNext = (((unsigned long)(now-fadeStart)*(digitDur)) / base);
      digitDurLast = digitDur - digitDurNext;
    }
    //Serial.print(F("last=")); Serial.print(digitDurLast,DEC);
    //Serial.print(F(" next=")); Serial.println(digitDurNext,DEC);
  } //end curently fading
  
  if(digitDur==0) { //display nothing - just wait
    delayMicroseconds(DIGIT_DUR_FULL*3); // The delay is to make cycleDisplay take up the same amount of loop time it usually does, for each of the 3 pairs. Not sure if necessary.
  } else {
    //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
    setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
    digitalWrite(anodes[0], HIGH); //Turn on tubes
    delayMicroseconds(digitDurLast);
    setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
    delayMicroseconds(digitDurNext); //we need these two lines even if no DurNext, because otherwise display flickers
    digitalWrite(anodes[0], LOW); //Turn off tubes
    
    delayMicroseconds(DIGIT_DUR_FULL-digitDur);
    
    //Anode channel 1: tubes #4 (sec x10) and #1 (hour x1)
    setCathodes(displayLast[4],displayLast[1]);
    digitalWrite(anodes[1], HIGH);
    delayMicroseconds(digitDurLast);
    setCathodes(displayNext[4],displayNext[1]);
    delayMicroseconds(digitDurNext);
    digitalWrite(anodes[1], LOW);
    
    delayMicroseconds(DIGIT_DUR_FULL-digitDur);
    
    //Anode channel 2: tubes #0 (hour x10) and #3 (min x1)
    setCathodes(displayLast[0],displayLast[3]);
    digitalWrite(anodes[2], HIGH);
    delayMicroseconds(digitDurLast);
    setCathodes(displayNext[0],displayNext[3]);
    delayMicroseconds(digitDurNext);
    digitalWrite(anodes[2], LOW);
    
    delayMicroseconds(DIGIT_DUR_FULL-digitDur);
  } //end if displayBrightness>0
  //TODO why does it sometimes flicker while in the setting mode
  
} //end cycleDisplay


void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade){
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank digit
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4-tube display --> 0015)
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
    if(!fade) displayLast[posEnd-i] = displayNext[posEnd-i]; //cycleDisplay will be none the wiser
  }
} //end editDisplay()

void blankDisplay(byte posStart, byte posEnd, byte fade){
  if(posEnd==255) posEnd=posStart; //single digit change
  for(byte i=posStart; i<=posEnd; i++) { displayNext[i]=15; if(!fade) displayLast[i]=15; }
} //end blankDisplay();

// void startScroll() { //To scroll a value in, call this after calling editDisplay as normal
//   for(byte i=0; i<6; i++) scrollDisplay[i] = displayNext[i]; //cache the incoming value in scrollDisplay[]
//   blankDisplay(0,5,true);
//   scrollRemain = DISPLAY_SIZE+1; //this will trigger updateDisplay() to start scrolling. DISPLAY_SIZE+1 adds blank frame at front
// } //end startScroll()

void displayBlink(){
  displayBlinkStart = millis();
}

// void checkEffects(bool force){
//   //control the cleaning/scrolling effects - similar to checkRTC but it has its own timings
//   unsigned long now = millis();
//   //If we're running a tube cleaning, advance it every CLEAN_SPEED ms.
//   if(cleanRemain && (unsigned long)(now-pollCleanLast)>=CLEAN_SPEED) { //account for rollover
//     pollCleanLast=now;
//     cleanRemain--;
//     if(cleanRemain<1) calcSun(rtcGetYear(),rtcGetMonth(),rtcGetDate()); //take this opportunity to perform a calculation that blanks the display for a bit
//     updateDisplay();
//   }
//   //If we're scrolling an animation, advance it every SCROLL_SPEED ms.
//   else if(scrollRemain!=0 && scrollRemain!=-128 && ((unsigned long)(now-pollScrollLast)>=SCROLL_SPEED || force)) {
//     pollScrollLast=now;
//     if(scrollRemain<0) {
//       scrollRemain++; updateDisplay();
//     } else {
//       scrollRemain--; updateDisplay();
//       if(scrollRemain==0) scrollRemain = -128;
//     }
//   }
// }

#endif //DISPLAY_NIXIE