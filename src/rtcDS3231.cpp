#include <Arduino.h>
#include "main.h"

#ifdef RTC_IS_DS3231 //see main.cpp Includes section
// Config flag is RTC_IS_DS3231 (not RTC_DS3231) to avoid colliding with the
// RTClib class of that exact name — referenced as `RTC_DS3231 rtc` below.

#include "rtcDS3231.h"
#include <Wire.h> //Arduino - GNU LPGL - for I2C access to DS3231
#include <RTClib.h>

//RTC objects
RTC_DS3231 rtc;
DateTime tod; //stores the rtc.now() snapshot for several functions to use

void rtcInit(){
  Wire.begin();
  rtc.begin();
}
void rtcSetTime(byte h, byte m, byte s){
  rtcTakeSnap();
  rtc.adjust(DateTime(tod.year(),tod.month(),tod.day(),h,m,s));
  millisReset();
}
void rtcSetDate(int y, byte m, byte d, byte w){
  //will cause the clock to fall slightly behind since it discards partial current second
  rtcTakeSnap();
  rtc.adjust(DateTime(y,m,d,tod.hour(),tod.minute(),tod.second()));
  millisReset();
}
void rtcSetHour(byte h){ //used for DST forward/backward
  //will cause the clock to fall slightly behind since it discards partial current second
  rtcTakeSnap();
  rtc.adjust(DateTime(tod.year(),tod.month(),tod.day(),h,tod.minute(),tod.second()));
  millisReset();
}

void rtcTakeSnap(){
  //rtcGet functions pull from this snapshot - to ensure that code works off the same timestamp
  tod = rtc.now();
}
int  rtcGetYear(){ return tod.year(); }
byte rtcGetMonth(){ return tod.month(); }
byte rtcGetDate(){ return tod.day(); }
byte rtcGetWeekday(){ return tod.dayOfTheWeek(); }
int  rtcGetTOD(){ return tod.hour()*60+tod.minute(); }
byte rtcGetHour(){ return tod.hour(); }
byte rtcGetMinute(){ return tod.minute(); }
byte rtcGetSecond(){ return tod.second(); }

byte rtcGetTemp(){ return rtc.getTemperature()*100; }

#endif //RTC_IS_DS3231