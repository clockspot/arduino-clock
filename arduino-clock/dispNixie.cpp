#include <arduino.h>
#include "arduino-clock.h"

#ifdef DISP_NIXIE //see arduino-clock.ino Includes section

#include "dispNixie.h"

// Display cycling code derived from http://arduinix.com/Main/Code/ANX-6Tube-Clock-Crossfade.txt

//This clock is 2x3 multiplexed: two tubes powered at a time.
//The anode channel determines which two tubes are powered,
//and the two SN74141 cathode driver chips determine which digits are lit.
//4 pins out to each SN74141, representing a binary number with values [1,2,4,8]
byte binOutA[4] = {OUT_A1,OUT_A2,OUT_A3,OUT_A4};
byte binOutB[4] = {OUT_B1,OUT_B2,OUT_B3,OUT_B4};
//3 pins out to anode channel switches
byte anodes[3] = {ANODE_1,ANODE_2,ANODE_3};

const int fadeDur = 5; //ms - each multiplexed pair of digits appears for this amount of time per cycle
const int dimDur = 4; //ms - portion of fadeDur that is left dark during dim times
int fadeNextDur = 0; //ms - during fade, incoming digit's portion of fadeDur
int fadeLastDur = 0; //ms - during fade, outgoing digit's portion of fadeDur
unsigned long fadeStartLast = 0; //millis - when the last digit fade was started
unsigned long setStartLast = 0; //to control flashing during start

unsigned long displayBlinkStart = 0; //when nonzero, display should briefly blank

byte cleanRemain = 0; //anti-cathode-poisoning clean timeout counter, increments at CLEAN_SPEED ms (see loop()). Start at 11 to run at clock startup //TODO nixie only

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

void cycleDisplay(byte displayDim, byte fnSetPg){
  unsigned long now = millis();
  
  if(displayBlinkStart){
    if((unsigned long)(now-displayBlinkStart)<250){ delay(fadeDur*3); return; }
    // The delay is to make cycleDisplay take up the same amount of loop time it usually does. Not sure if necessary.
    else displayBlinkStart = 0;
  }
  
  //Other display code decides whether we should dim per function or time of day
  bool dim = (displayDim==1?1:0);
  //But if we're setting, decide here to dim for every other 500ms since we started setting
  if(fnSetPg>0) {
    if(setStartLast==0) setStartLast = now;
    dim = 1-(((unsigned long)(now-setStartLast)/500)%2);
  } else {
    if(setStartLast>0) setStartLast=0;
  }
  //TODO if we want to flash certain elements, we might do it similarly here
  
  fadeLastDur = fadeDur-(dim?dimDur:0); //by default, last digit displays for entire fadeDur minus dim time
  
  if(/*readEEPROM(20,false)==0 ||*/ dim) { //fading disabled or dim
    //TODO needs to become a global var
    if(fadeStartLast) fadeStartLast = 0; //cancel any fade currently going - dim state doesn't have enough steps to fade well
    for(byte i=0; i<6; i++) if(displayNext[i] != displayLast[i]) displayLast[i] = displayNext[i];
  }
  else { //fading enabled
    if(fadeStartLast==0) { //not fading - time to fade?
      for(byte i=0; i<6; i++) if(displayNext[i] != displayLast[i]) { fadeStartLast = now; break; }
    }
    if(fadeStartLast!=0) { //currently fading
      //let the next digit steal some display time from the last digit
      //ex: if fade time (from EEPROM) is 20ms, and fadeDur (next+last) is 6ms:
      // at  0ms, next = (( 0*(6-1))/20)+1 = 1; last = (6-nextDur) = 5;
      // at 10ms, next = ((10*(6-1))/20)+1 = 3; last = (6-nextDur) = 3; ...
      // at 20ms, next = ((20*(6-1))/20)+1 = 6; next = total, so fade is over!
      //TODO facilitate longer fades by writing a tweening function that smooths the frames, i.e. 111121222 - or use delayMicroseconds as below
      //TODO does this have more problems with the millis rollover issue?
      fadeNextDur = (((unsigned long)(now-fadeStartLast)*(fadeDur-1))/(5*10))+1; //readEEPROM(20,false) TODO needs to become a global var
      if(fadeNextDur >= fadeLastDur) { //fade is over
        fadeStartLast = 0;
        fadeNextDur = 0;
        fadeLastDur = fadeDur;
        for(byte j=0; j<6; j++) displayLast[j] = displayNext[j];
      } //end fade is over
      else { //shorten last digit display duration by subtracting next display duration from it
        fadeLastDur = fadeLastDur - fadeNextDur;
      }
    } //end curently fading
  } //end fading enabled
  
  //TODO consider using delayMicroseconds() which, with its tighter resolution, may give better control over fades and dim levels
  if(displayDim>0) { //if other display code says to shut off entirely, skip this part
    //Anode channel 0: tubes #2 (min x10) and #5 (sec x1)
    setCathodes(displayLast[2],displayLast[5]); //Via d2b decoder chip, set cathodes to old digits
    digitalWrite(anodes[0], HIGH); //Turn on tubes
    delay(fadeLastDur);//-(dim?dimDur:0)); //Display for fade-out cycles
    setCathodes(displayNext[2],displayNext[5]); //Switch cathodes to new digits
    delay(fadeNextDur);//-(dim?dimDur:0)); //Display for fade-in cycles
    digitalWrite(anodes[0], LOW); //Turn off tubes
  
    if(dim) delay(dimDur);
  
    //Anode channel 1: tubes #4 (sec x10) and #1 (hour x1)
    setCathodes(displayLast[4],displayLast[1]);
    digitalWrite(anodes[1], HIGH);
    delay(fadeLastDur);
    setCathodes(displayNext[4],displayNext[1]);
    delay(fadeNextDur);
    digitalWrite(anodes[1], LOW);
  
    if(dim) delay(dimDur);
  
    //Anode channel 2: tubes #0 (hour x10) and #3 (min x1)
    setCathodes(displayLast[0],displayLast[3]);
    digitalWrite(anodes[2], HIGH);
    delay(fadeLastDur);
    setCathodes(displayNext[0],displayNext[3]);
    delay(fadeNextDur);
    digitalWrite(anodes[2], LOW);
  
    if(dim) delay(dimDur);
  } //end if displayDim>0
  //TODO why does it sometimes flicker while in the setting mode
}

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