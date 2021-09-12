#include <arduino.h>
#include "arduino-clock.h"

#ifdef DISP_NIXIE //see arduino-clock.ino Includes section

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

//Most timing is in hundredths of a millisecond – which I'm calling hundos
//I was doing microseconds, but it caused the display to flicker too much while recalculating

//these can be overridden in your config .h
#ifndef BRIGHTNESS_FULL
#define BRIGHTNESS_FULL 500 //hundos - This determines the overall refresh rate of the display. This is the period of time reserved for each digit (or incoming/outgoing pair of digits, when fading). At full brightness, it appears for this full period. At less than full brightness, it appears for a portion of this time plus a blanking period – this duty cycling achieves the dimming effect.
#endif
#ifndef BRIGHTNESS_DIM
#define BRIGHTNESS_DIM 50 //hundos - When the display is at is dimmest, this is the period of time the digit(s) should appear. (In this case the blanking period will be BRIGHTNESS_FULL-BRIGHTNESS_DIM.)
#endif

// const int fadeDur = 500;
// const int dimDur = 450; //hundos - portion of fadeDur that is left dark during dim times

byte curBrightness = 255; //represents current display normal/dim/off state – compare to displayBrightness as passed to cycleDisplay. Start at 255 to force an adjustment at startup
int curBrightnessDur = 0; //The actual current brightness between BRIGHTNESS_FULL and BRIGHTNESS_DIM, expressed as the duration for which the digit should appear in its cycle. Stored here so we don't have to calculate it every cycle - since tubes don't hang onto it like LED controllers do.
#ifdef LIGHTSENSOR
word curAmbientLightLevel = 0;
#endif
int fadeNextDur = 0; //hundos - during fade, incoming digit's portion of curBrightnessDur
int fadeLastDur = 0; //hundos - during fade, outgoing digit's portion of curBrightnessDur
unsigned long fadeStartLast = 0; //hundos - when the last digit fade was started

unsigned long displayBlinkStart = 0; //hundos - when nonzero, display should briefly blank

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

unsigned long setStartLast = 0; //**millis** - to control flashing during start
bool setBlinkState = 0;
void cycleDisplay(byte displayBrightness, bool useAmbient, word ambientLightLevel, byte fnSetPg){
  unsigned long now = micros()/10; //hundos
  
  //See if it's time to stop the blink
  if(displayBlinkStart){
    if((unsigned long)(now-displayBlinkStart)<250000){ delayMicroseconds(BRIGHTNESS_FULL*3*10); return; }
    // The delay is to make cycleDisplay take up the same amount of loop time it usually does, for each of the 3 pairs. Not sure if necessary.
    else displayBlinkStart = 0;
  }
  
  //Check if it's time to change brightness - either due to setting flashing or change in displayBrightness/ambientLightLevel inputs
  if(fnSetPg>0) { //setting - dim for every other 500ms - using the brightest and dimmest states of the display (TODO too crude?)
    if(setStartLast==0) {
      setStartLast = millis();
      setBlinkState = 1;
      //When starting, if we're using ambient at less than 2/3 brightness, set curBrightness to dim, so we'll start bright
      if(useAmbient && ambientLightLevel<170) curBrightness = 1;
    }
    bool blinkModulus = ((unsigned long)(millis()-setStartLast)/500)%2;
    if(setBlinkState!=blinkModulus) { //will occur every 500ms
      setBlinkState = blinkModulus;
      //If we were dim at start (curBrightness), invert setBlinkState to "start" at 1
      curBrightnessDur = ((curBrightness==1?1:0)-setBlinkState)? BRIGHTNESS_FULL: BRIGHTNESS_DIM;
      //Serial.print(F("New curBrightnessDur set: ")); Serial.println(curBrightnessDur,DEC);
    }
  }
   
  else { //not setting - defer to what other code has set displayBrightness to (and ambientLightLevel if applicable)  
#ifdef LIGHTSENSOR
    //if using ambient lighting and: it has changed, or if returning from setting, or if brightness normality has changed
    if(useAmbient && (curAmbientLightLevel != ambientLightLevel || setStartLast>0 || curBrightness != displayBrightness)) {
      curAmbientLightLevel = ambientLightLevel;
      if(displayBrightness==2) { //If currently normal brightness (so we should display per ambient light level) - otherwise see below code
        //Convert the ambient light level (0-255, per LUX_DIM-LUX_FULL) to the corresponding brightness value for the actual display hardware, within the desired range (BRIGHTNESS_DIM-BRIGHTNESS_FULL)
        curBrightnessDur = BRIGHTNESS_DIM + ((long)curAmbientLightLevel * (BRIGHTNESS_FULL - BRIGHTNESS_DIM) /255);
        //Serial.print(F("New curBrightnessDur amb: ")); Serial.println(curBrightnessDur,DEC);
      }
    }
#endif
    //if brightness normality has changed
    if(curBrightness != displayBrightness) {
      curBrightness = displayBrightness;
      if(curBrightness==0) { curBrightnessDur = 0; //Serial.print(F("New curBrightnessDur nor: ")); Serial.println(curBrightnessDur,DEC); 
      }
      if(curBrightness==1) { curBrightnessDur = BRIGHTNESS_DIM; //Serial.print(F("New curBrightnessDur nor: ")); Serial.println(curBrightnessDur,DEC); 
      }
      if(curBrightness==2) { //normal brightness - only set if no light sensor, or not using light sensor
#ifdef LIGHTSENSOR
        if(!useAmbient) curBrightnessDur = BRIGHTNESS_FULL;
#else
        { curBrightnessDur = BRIGHTNESS_FULL; //Serial.print(F("New curBrightnessDur nor: ")); Serial.println(curBrightnessDur,DEC); 
        }
#endif
      }
    }
    if(setStartLast>0) setStartLast=0; //remove setting flag if needed
  } //end if not setting
  
  //TODO why is there a pause right at the top?
  
  //Controlling the duration of the fading digit pairs
  if(fadeStartLast==0) { //not fading - time to fade?
    for(byte i=0; i<6; i++) if(displayNext[i] != displayLast[i]) { fadeStartLast = now; break; }
  }
  if(fadeStartLast!=0) { //currently fading
    //let the next digit steal some display time from the last digit
    //ex: if fade time (from EEPROM) is 20/100sec (20000 hundos), and curBrightnessDur (next+last) is 500 hundos:
    /*
    at    0 hds,  (  0*500)/20000 = 0
    at  500 hds,  (500*500)/20000 = 12
    at 1000 hds, (1000*500)/20000 = 25
    at 5000 hds, (5000*500)/20000 = 125
    at 10k  hds, (10k *500)/20000 = 250
    at 20k  hds, (20k *500)/20000 = 500
    */
    //TODO does this have more problems with the millis/micros rollover issue?
    //we add 1 to make sure it doesn't floor itself into stalling
    fadeNextDur = (((unsigned long)(now-fadeStartLast)*(curBrightnessDur))/(readEEPROM(20,false)*1000))+1;
    if(fadeNextDur >= fadeLastDur) { //fade is over
      fadeStartLast = 0;
      fadeNextDur = 0;
      fadeLastDur = curBrightnessDur;
      for(byte j=0; j<6; j++) displayLast[j] = displayNext[j];
    } //end fade is over
    else { //shorten last digit display duration by subtracting next display duration from it
      fadeLastDur = fadeLastDur - fadeNextDur;
    }
  } //end curently fading
  
  if(displayBrightness>0) { //if other display code says to shut off entirely, skip this part
    //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
    setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
    digitalWrite(anodes[0], HIGH); //Turn on tubes
    delayMicroseconds(fadeLastDur*10);//-(dim?dimDur:0)); //Display for fade-out cycles
    setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
    delayMicroseconds(fadeNextDur*10);//-(dim?dimDur:0)); //Display for fade-in cycles
    digitalWrite(anodes[0], LOW); //Turn off tubes
    
    delayMicroseconds((BRIGHTNESS_FULL-curBrightnessDur)*10);
    
    //Anode channel 1: tubes #4 (sec x10) and #1 (hour x1)
    setCathodes(displayLast[4],displayLast[1]);
    digitalWrite(anodes[1], HIGH);
    delayMicroseconds(fadeLastDur*10);
    setCathodes(displayNext[4],displayNext[1]);
    delayMicroseconds(fadeNextDur*10);
    digitalWrite(anodes[1], LOW);
    
    delayMicroseconds((BRIGHTNESS_FULL-curBrightnessDur)*10);
    
    //Anode channel 2: tubes #0 (hour x10) and #3 (min x1)
    setCathodes(displayLast[0],displayLast[3]);
    digitalWrite(anodes[2], HIGH);
    delayMicroseconds(fadeLastDur*10);
    setCathodes(displayNext[0],displayNext[3]);
    delayMicroseconds(fadeNextDur*10);
    digitalWrite(anodes[2], LOW);
    
    delayMicroseconds((BRIGHTNESS_FULL-curBrightnessDur)*10);
  } //end if displayBrightness>0
  //TODO why does it sometimes flicker while in the setting mode
  
} //end cycleDisplay


void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade){
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank digit
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4-tube display --> 0015)
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

#endif //DISP_NIXIE