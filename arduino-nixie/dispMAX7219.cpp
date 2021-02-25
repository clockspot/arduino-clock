#ifdef DISP_MAX7219 //only compile when requested (when included in main file)
#ifndef DISP_MAX7219_SRC //include once only
#define DISP_MAX7219_SRC

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#include <SPI.h> //Arduino - for SPI access to MAX7219
#include <LedControl.h> //Eberhard Farle's LedControl library - http://wayoda.github.io/LedControl

LedControl lc=LedControl(DIN_PIN,CLK_PIN,CS_PIN,NUM_MAX);

int curBrightness = BRIGHTNESS_FULL;

unsigned long displayBlinkStart = 0; //when nonzero, display should briefly blank

void initDisplay(){
  for(int i=0; i<NUM_MAX; i++) { lc.shutdown(i,false); lc.setIntensity(i,curBrightness); }
}

//TODO can we move this into flash?
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

void sendToMAX7219(){ //"private"
  //Called by editDisplay and blankDisplay. Needed in lieu of what cycleDisplay does for nixies.
  int ci = (NUM_MAX*8)-1; //total column index - we will start at the last one and move backward
  //display index = (NUM_MAX-1)-(ci/8)
  //display column index = ci%8
  for(int i=0; i<bignumWidth; i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, (displayNext[0]==15?0:bignum[displayNext[0]*bignumWidth+i])); ci--; } //h tens
  for(int i=0; i<1;           i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, 0); ci--; } //1col gap
  for(int i=0; i<bignumWidth; i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, (displayNext[1]==15?0:bignum[displayNext[1]*bignumWidth+i])); ci--; } //h ones
  for(int i=0; i<2;           i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, 0); ci--; } //2col gap
  for(int i=0; i<bignumWidth; i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, (displayNext[2]==15?0:bignum[displayNext[2]*bignumWidth+i])); ci--; } //m tens
  for(int i=0; i<1;           i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, 0); ci--; } //1col gap
  for(int i=0; i<bignumWidth; i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, (displayNext[3]==15?0:bignum[displayNext[3]*bignumWidth+i])); ci--; } //m ones
  if(NUM_MAX>3){ //TODO test
    for(int i=0; i<1;             i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, 0); ci--; } //1col gap
    for(int i=0; i<smallnumWidth; i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, (displayNext[4]==15?0:smallnum[displayNext[4]*smallnumWidth+i])); ci--; } //s tens
    for(int i=0; i<1;             i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, 0); ci--; } //1col gap
    for(int i=0; i<smallnumWidth; i++){ lc.setColumn((NUM_MAX-1)-(ci/8),ci%8, (displayNext[5]==15?0:smallnum[displayNext[5]*smallnumWidth+i])); ci--; } //m ones
  }
}

unsigned long setStartLast = 0; //to control flashing during start
void cycleDisplay(){
  unsigned long now = millis();
  //MAX7219 handles its own cycling - just needs display data updates.
  //But we do need to check if the blink should be over, and whether dim has changed.
  if(displayBlinkStart){
    if((unsigned long)(now-displayBlinkStart)>=250){ displayBlinkStart = 0; sendToMAX7219(); }
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
  if(curBrightness!=(dim? BRIGHTNESS_DIM: BRIGHTNESS_FULL)){
    curBrightness = (dim? BRIGHTNESS_DIM: BRIGHTNESS_FULL);
    for(int i=0; i<NUM_MAX; i++) { lc.setIntensity(i,curBrightness); }
  }
}

void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade){
  //Splits n into digits, sets them into displayNext in places posSt-posEnd (inclusive), with or without leading zeros
  //If there are blank places (on the left of a non-leading-zero number), uses value 15 to blank tube
  //If number has more places than posEnd-posStart, the higher places are truncated off (e.g. 10015 on 4 tubes --> 0015)
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
  sendToMAX7219();
}
void blankDisplay(byte posStart, byte posEnd, byte fade){
  for(byte i=posStart; i<=posEnd; i++) { displayNext[i]=15; }
  sendToMAX7219();
}

//void startScroll() {}

void displayBlink(){
  for(int i=0; i<NUM_MAX; i++) { lc.clearDisplay(i); }
  displayBlinkStart = millis();
}

//void checkEffects(bool force){}

#endif
#endif