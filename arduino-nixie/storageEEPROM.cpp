#ifdef STORAGE_EEPROM //include if requested. TODO change to platform stuff
#ifndef STORAGE_EEPROM_SRC //include once only
#define STORAGE_EEPROM_SRC

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch
#include <EEPROM.h> //Arduino - GNU LPGL

void storageInit(){
  
}

#endif
#endif