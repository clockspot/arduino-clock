// Persistent storage

// This project was originally written to use the AVR Arduino's EEPROM for persistent storage, and would frequently read it directly during runtime (not just at startup). I wanted to abstract that away, partly to add support for SAMD flash memory, and partly to protect against runtime errors due to EEPROM/flash failure.
// This code serves those values out of a volatile array of bytes, which are backed by EEPROM/flash for the sole purpose of recovery after a power failure. It reads them from EEPROM/flash at startup, and sets them when changed.
// Flash support is via cmaglie's FlashStorage library. It offers an EEPROM emulation mode, which I'm currently using, but I don't like that it writes the entire "EEPROM" data to flash (not just the value being updated) with every commit() – I think it will wear out unnecessarily. TODO address this.
// Note that flash data is necessarily wiped out when the sketch is (re)uploaded.

#include <arduino.h>
#include "arduino-clock.h"

#include "storage.h"

#ifdef __AVR__
  #include <EEPROM.h> //Arduino - GNU LPGL
#else //SAMD - is there a better way to detect EEPROM availability? TODO
  #define FLASH_AS_EEPROM
  //cmaglie's FlashStorage library - https://github.com/cmaglie/FlashStorage/
  #include <FlashAsEEPROM.h> //EEPROM mode
  //#include <FlashStorage.h> //regular mode
#endif

#define STORAGE_SPACE 154 //number of bytes
byte storageBytes[STORAGE_SPACE]; //the volatile array of bytes
#define COMMIT_TO_EEPROM 1 //1 for production

void initStorage(){
  //If this is SAMD, write starting values if unused
  #ifdef FLASH_AS_EEPROM
  //Serial.println(F("Hello world from storage.cpp"));
  //Serial.print(F("valid=")); Serial.println(EEPROM.isValid(),DEC);
  //Serial.print(F("16=")); Serial.println(EEPROM.read(16));
  if(!EEPROM.isValid() || EEPROM.read(16)==0 || EEPROM.read(16)==255){ //invalid eeprom, wipe it out
    for(byte i=0; i<STORAGE_SPACE; i++) EEPROM.update(i,0);
    if(COMMIT_TO_EEPROM){
      EEPROM.commit();
      //Serial.println(F("WARNING: FLASH EEPROM COMMIT per init"));
    }
  }
  #endif
  //Read from real persistent storage into storageBytes
  for(byte i=0; i<STORAGE_SPACE; i++) storageBytes[i] = EEPROM.read(i);
}

int readEEPROM(int loc, bool isInt){
  //Read from the volatile array, either a byte or a signed int
  //Must read int as 16-bit, since on SAMD int is 32-bit and negatives aren't read correctly
  if(isInt) return (int16_t)(storageBytes[loc]<<8)+storageBytes[loc+1];
  else return storageBytes[loc];
}

bool writeEEPROM(int loc, int val, bool isInt, bool commit){
  //Update the volatile array and the real persistent storage for posterity
  //Eiither a byte or a signed int
  //Serial.print(F("Set ")); Serial.print(loc); Serial.print(F("=")); Serial.print(val,DEC);
  if(readEEPROM(loc,isInt)==val){
    //Serial.println(F(": nothing doing"));
    return false;
  }
  //Serial.println(F(": ok (probably)"));
  if(isInt){
    storageBytes[loc] = highByte(val);
    storageBytes[loc+1] = lowByte(val);
    EEPROM.update(loc,highByte(val));
    EEPROM.update(loc+1,lowByte(val));
  } else {
    storageBytes[loc] = val;
    EEPROM.update(loc,val);
  }
  #ifdef FLASH_AS_EEPROM
    if(commit && COMMIT_TO_EEPROM){
      EEPROM.commit(); //bad!! See TODO in storage.h
      //Serial.println(F("WARNING: FLASH EEPROM COMMIT per write"));
    }
  #endif
  return true; //a value was changed
}
void commitEEPROM(){
  #ifdef FLASH_AS_EEPROM
  if(COMMIT_TO_EEPROM){
    EEPROM.commit(); //bad!! See TODO in storage.h
    //Serial.println(F("WARNING: FLASH EEPROM COMMIT by request"));
  }
  #endif
}