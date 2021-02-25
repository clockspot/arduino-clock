#ifdef STORAGE //only compile when requested (when included in main file)
#ifndef STORAGE_SRC //include once only
#define STORAGE_SRC

//This project was originally written to use the AVR Arduino's EEPROM locs for persistent storage, and would frequently read it directly.
//I wanted to abstract that away, partly to add support for SAMD flash, and partly to protect against runtime errors due to EEPROM/flash failure.
//So this code serves those values out of a volatile array of bytes.
//It reads them from EEPROM/flash at startup, and sets them into EEPROM/flash when changed, for recovery purposes.

#define STORAGE_SPACE 51 //number of bytes
byte storageBytes[STORAGE_SPACE]; //the volatile array of bytes

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#ifdef __AVR__
  #include <EEPROM.h> //Arduino - GNU LPGL
#endif

void initStorage(){
  //Read from real persistent storage into storageBytes
  for(byte i=0; i<STORAGE_SPACE; i++){
    #ifdef __AVR__
      storageBytes[i] = EEPROM.read(i);
    #else
      storageBytes[i] = 0;
    #endif
  }
}

int readEEPROM(int loc, bool isInt){
  //Read from the volatile array
  if(isInt) return (storageBytes[loc]<<8)+storageBytes[loc+1];
  else return storageBytes[loc];
}

void writeEEPROM(int loc, int val, bool isInt){
  //Update the volatile array and the real persistent storage for posterity
  if(isInt){
    storageBytes[loc] = highByte(val);
    storageBytes[loc+1] = lowByte(val);
    #ifdef __AVR__
      EEPROM.update(loc,highByte(val));
      EEPROM.update(loc+1,lowByte(val));
    #else
      //TODO
    #endif
  } else {
    storageBytes[loc] = val;
    #ifdef __AVR__
      EEPROM.update(loc,val);
    #else
      //TODO
    #endif
  }
}

#endif
#endif