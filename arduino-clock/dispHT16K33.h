#ifndef DISP_HT16K33_H
#define DISP_HT16K33_H

//Mutually exclusive with other disp options

void initDisplay();
void sendToHT16K33(byte posStart, byte posEnd);
void cycleDisplay(byte displayBrightness, byte fnSetPg);
void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade);
void blankDisplay(byte posStart, byte posEnd, byte fade);
void displayBlink();

#endif //DISP_HT16K33