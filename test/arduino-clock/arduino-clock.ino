#include <arduino.h>
#include "arduino-clock.h"

#include "rtcDS3231.h" //if RTC_DS3231 is defined in config – for an I2C DS3231 RTC module

void setup(){
  Serial.begin(57600);
  delay(3000);
  setupRTC();
}

void loop(){
  loopRTC();
}
