#ifdef DISP_MAX7219 //include if requested
#ifndef DISP_MAX7219_SRC //include once only
#define DISP_MAX7219_SRC

#include <SPI.h> //Arduino - for SPI access to MAX7219

///// "Public" functions /////

void initDisplay(){}

void cycleDisplay(){}

void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade){}
void blankDisplay(byte posStart, byte posEnd, byte fade){}
void startScroll() {}

void updateDisplay(){} //end updateDisplay()

void displayBlink(){}

void checkEffects(bool force){}

#endif
#endif