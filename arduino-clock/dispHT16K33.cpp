#include <arduino.h>
#include "arduino-clock.h"

#ifdef DISP_HT16K33 //see arduino-clock.ino Includes section

#include "dispHT16K33.h"
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

Adafruit_7segment matrix = Adafruit_7segment();

int curBrightness = BRIGHTNESS_FULL;

unsigned long displayBlinkStart = 0; //when nonzero, display should briefly blank

void initDisplay(){
  matrix.begin(DISP_ADDR);
  matrix.setBrightness(curBrightness);
}

byte displayNext[6] = {15,15,15,15,15,15}; //Internal representation of display. Blank to start.

void sendToHT16K33(byte posStart, byte posEnd){ //"private"
  //Called by editDisplay and blankDisplay. Needed in lieu of what cycleDisplay does for nixies.
  for(byte i=posStart; i<=posEnd; i++){
    if(i>3 && DISPLAY_SIZE<6) return; //if fewer than 6 digits, don't render digits 4 and 5 (0-index)
    if(displayNext[i]>9) matrix.writeDigitRaw((i>=2?i+1:i),0); //blank - skip pos 2 (colon)
    else matrix.writeDigitNum((i>=2?i+1:i),displayNext[i]);
  }
  matrix.writeDisplay();
}

unsigned long setStartLast = 0; //to control flashing during start
void cycleDisplay(byte displayDim, byte fnSetPg){
  unsigned long now = millis();
  //HT16K33 handles its own cycling - just needs display data updates.
  //But we do need to check if the blink should be over, and whether dim has changed.
  if(displayBlinkStart){
    if((unsigned long)(now-displayBlinkStart)>=500){ displayBlinkStart = 0; sendToHT16K33(0,5); }
  }
  //Other display code decides whether we should dim per function or time of day
  char dim = displayDim; //2=normal, 1=dim, 0=off
  //But if we're setting, decide here to dim for every other 500ms since we started setting
  if(fnSetPg>0) {
    if(setStartLast==0) setStartLast = now;
    dim = (1-(((unsigned long)(now-setStartLast)/500)%2))+1;
  } else {
    if(setStartLast>0) setStartLast=0;
  }
  if(curBrightness!=(dim==2? BRIGHTNESS_FULL: (dim==1? BRIGHTNESS_DIM: -1))){
    curBrightness = (dim==2? BRIGHTNESS_FULL: (dim==1? BRIGHTNESS_DIM: -1));
    if(curBrightness==-1) for(int i=0; i<DISPLAY_SIZE; i++) { matrix.writeDigitRaw((i>=2?i+1:i),0); } //force dark
    else matrix.setBrightness(curBrightness);
  }
}

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
  sendToHT16K33(posStart,posEnd);
  //cycleDisplay(); //fixes brightness - can we skip this?
  
  for(int i=0; i<DISPLAY_SIZE; i++) {
    if(displayNext[i]>9) Serial.print(F("-"));
    else Serial.print(displayNext[i],DEC);
  }
  Serial.println();  
}
void blankDisplay(byte posStart, byte posEnd, byte fade){
  for(byte i=posStart; i<=posEnd; i++) { displayNext[i]=15; }
  sendToHT16K33(posStart,posEnd);
  //cycleDisplay(); //fixes brightness - can we skip this?
}

//void startScroll() {}

void displayBlink(){
  for(int i=0; i<DISPLAY_SIZE; i++) { matrix.writeDigitRaw((i>=2?i+1:i),0); } //force dark
  displayBlinkStart = millis();
}

//void checkEffects(bool force){}

#endif //DISP_HT16K33