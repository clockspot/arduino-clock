#ifndef DISP_NIXIE_H
#define DISP_NIXIE_H

//Mutually exclusive with other disp options

void decToBin(bool binVal[], byte i);
void setCathodes(byte decValA, byte decValB);
void initDisplay();
void cycleDisplay(byte displayDim, byte fnSetPg);
void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade);
void blankDisplay(byte posStart, byte posEnd, byte fade);
// void startScroll();
void displayBlink();
// void checkEffects(bool force);

#endif //DISP_NIXIE_H