#ifndef DISPLAY_NIXIE_H
#define DISPLAY_NIXIE_H

//Mutually exclusive with other disp options

void decToBin(bool binVal[], byte i);
void setCathodes(byte decValA, byte decValB);
void initDisplay();
void cycleDisplay(byte displayBrightness, bool useAmbient, word ambientLightLevel, byte fnSetPg);
void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade);
void blankDisplay(byte posStart, byte posEnd, byte fade);
// void startScroll();
void displayBlink();
// void checkEffects(bool force);

#endif //DISPLAY_NIXIE_H