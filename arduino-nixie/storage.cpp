#include <arduino.h>
#include "arduino-nixie.h"

#include "storage.h" //see notes herein

#ifdef __AVR__ //TODO is there a similar flag for SAMD
  #include <EEPROM.h> //Arduino - GNU LPGL
#else
  #define FLASH_AS_EEPROM
  #include <FlashAsEEPROM.h> //cmaglie's FlashStorage library - EEPROM mode - https://github.com/cmaglie/FlashStorage/
  //#include <FlashStorage.h> //cmaglie's FlashStorage library - regular mode
#endif

#define STORAGE_SPACE 152 //number of bytes
byte storageBytes[STORAGE_SPACE]; //the volatile array of bytes

void initStorage(){
  //If this is SAMD, write starting values if unused
  #ifdef FLASH_AS_EEPROM
  //Serial.print(F("valid=")); Serial.println(EEPROM.isValid(),DEC);
  //Serial.print(F("16=")); Serial.println(EEPROM.read(16));
  if(!EEPROM.isValid() || EEPROM.read(16)==0 || EEPROM.read(16)==255){ //invalid eeprom, wipe it out
    for(byte i=0; i<STORAGE_SPACE; i++) EEPROM.update(i,0);
    EEPROM.commit();
    //Serial.println(F("WARNING: FLASH EEPROM COMMIT per init"));
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
    if(commit){
      EEPROM.commit(); //bad!! See TODO in storage.h
      //Serial.println(F("WARNING: FLASH EEPROM COMMIT per write"));
    }
  #endif
  return true; //a value was changed
}
void commitEEPROM(){
  #ifdef FLASH_AS_EEPROM
    EEPROM.commit(); //bad!! See TODO in storage.h
    //Serial.println(F("WARNING: FLASH EEPROM COMMIT by request"));
  #endif
}