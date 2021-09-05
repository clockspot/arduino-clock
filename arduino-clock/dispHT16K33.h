#ifndef DISP_HT16K33
#define DISP_HT16K33

//Mutually exclusive with other disp options

void initDisplay();
void sendToHT16K33(byte posStart, byte posEnd);
void cycleDisplay(byte displayDim, byte fnSetPg);
void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade);
void blankDisplay(byte posStart, byte posEnd, byte fade);
void displayBlink();

#endif //DISP_HT16K33