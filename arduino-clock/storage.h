#ifndef STORAGE_H
#define STORAGE_H

void initStorage(); //Read from real persistent storage into storageBytes
int readEEPROM(int loc, bool isInt); //Read from the volatile array
bool writeEEPROM(int loc, int val, bool isInt, bool commit=1); //Update the volatile array and the real persistent storage for posterity
void commitEEPROM();

#endif //STORAGE_H