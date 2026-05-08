#ifndef DISPLAY_MAX7219_H
#define DISPLAY_MAX7219_H

//Mutually exclusive with other disp options

void initDisplay();
void sendToMAX7219(byte posStart, byte posEnd);
void cycleDisplay(byte displayBrightness, bool useAmbient, word ambientLightLevel, byte fnSetPg);
void editDisplay(word n, byte posStart, byte posEnd=255, bool leadingZeros=false, bool fade=false);
void blankDisplay(byte posStart, byte posEnd=255, byte fade=false);
void displayBlink();

#endif //DISPLAY_MAX7219_H