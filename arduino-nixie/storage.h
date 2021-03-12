#ifndef STORAGE_H
#define STORAGE_H

// This project was originally written to use the AVR Arduino's EEPROM for persistent storage, and would frequently read it directly during runtime (not just at startup). I wanted to abstract that away, partly to add support for SAMD flash memory, and partly to protect against runtime errors due to EEPROM/flash failure.
// This code serves those values out of a volatile array of bytes, which are backed by EEPROM/flash for the sole purpose of recovery after a power failure. It reads them from EEPROM/flash at startup, and sets them when changed.
// Flash support is via cmaglie's FlashStorage library. It offers an EEPROM emulation mode, which I'm currently using, but I don't like that it writes the entire "EEPROM" data to flash (not just the value being updated) with every commit() – I think it will wear out unnecessarily. TODO address this.
// Note that flash data is necessarily wiped out when the sketch is (re)uploaded.

void initStorage(); //Read from real persistent storage into storageBytes
int readEEPROM(int loc, bool isInt); //Read from the volatile array
bool writeEEPROM(int loc, int val, bool isInt, bool commit=1); //Update the volatile array and the real persistent storage for posterity
void commitEEPROM();

#endif //STORAGE_H