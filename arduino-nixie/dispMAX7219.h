#ifndef DISP_MAX7219_H
#define DISP_MAX7219_H

//Mutually exclusive with other disp options

void initDisplay();
void sendToMAX7219(byte posStart, byte posEnd);
void cycleDisplay(byte displayDim, byte fnSetPg);
void editDisplay(word n, byte posStart, byte posEnd, bool leadingZeros, bool fade);
void blankDisplay(byte posStart, byte posEnd, byte fade);
void displayBlink();

#endif //DISP_MAX7219_H