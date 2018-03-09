// #include <EEPROM.h>

// #include <Wire.h>
// #include <DS3231.h>
// DS3231 ds3231;
// RTClib rtc;
// DateTime tod;
// byte toddow;

////////// Main code control //////////

void setup(){
  Serial.begin(9600);
  //Wire.begin();
  //ds3231.setDate(2);
  Serial.println();
  testDate(1900,1,1);
  testDate(1985,10,7);
  testDate(1996,2,15);
  testDate(1996,3,15);
  testDate(1997,2,15);
  testDate(1997,3,15);
  testDate(2000,2,15);
  testDate(2000,3,15);
  testDate(2018,3,8);
  testDate(2018,3,9);
  testDate(2018,3,10);
  testDate(2018,3,11);
  testDate(2018,3,12);
  testDate(2018,3,13);
  testDate(2018,3,14);
  testDate(2018,3,15);
  testDate(2040,3,15);
}

byte testDate(word y, byte m, byte d){
  Serial.print(y,DEC);
  Serial.print("-");
  Serial.print(m,DEC);
  Serial.print("-");
  Serial.print(d,DEC);
  Serial.print(" is ");
  Serial.println(dayOfWeek(y,m,d),DEC);
}


byte dayOfWeek(word y, byte m, byte d){
  //DS3231 doesn't really calculate the day of the week, it just keeps a counter.
  //We'll calculate per https://en.wikipedia.org/wiki/Zeller%27s_congruence
  byte yb = y%100; //2-digit year
  byte ya = y/100; //century
  //For this formula, Jan and Feb are considered months 11 and 12 of the previous year.
  //So if it's Jan or Feb, add 10 to the month, and set back the year and century if applicable
  if(m<3) { m+=10; if(yb==0) { yb=99; ya-=1; } else yb-=1; }
  else m -= 2; //otherwise subtract 2 from the month
  return (d + ((13*m-1)/5) + yb + (yb/4) + (ya/4) + 5*ya) %7;
}

void loop(){
  //Things done every "clock cycle"
  //checkRTC(); //if clock has ticked, decrement timer if running, and updateDisplay
}

////////// Clock ticking and timed event triggering //////////
unsigned long rtcPollLast = 0; //maybe don't poll the RTC every loop? would that be good?
byte rtcSecLast = 61;
bool h12=false;
bool PM=false;
bool century=false;
void checkRTC(){
  // //Checks for new time-of-day second; decrements timer; checks for timed events;
  // //updates display for running time or date.
  // if(rtcPollLast<millis()+50) { //check every 1/20th of a second
  //   rtcPollLast=millis();
  //   //Check for timeouts based on millis
  //   //Update things based on RTC
  //   tod = rtc.now();
  //   toddow = ds3231.getDoW();
  //
  //   if(rtcSecLast != tod.second()) {
  //     rtcSecLast = tod.second();
  //     Serial.print("It is now ");
  //     Serial.print(tod.year(),DEC);
  //     Serial.print("-");
  //     Serial.print(tod.month(),DEC);
  //     Serial.print("-");
  //     Serial.print(tod.day(),DEC);
  //     Serial.print(" ");
  //     Serial.print(tod.hour(),DEC);
  //     Serial.print(":");
  //     Serial.print(tod.minute(),DEC);
  //     Serial.print(":");
  //     Serial.print(tod.second(),DEC);
  //     Serial.print(" --or-- ");
  //     Serial.print(ds3231.getYear(),DEC);
  //     Serial.print("-");
  //     Serial.print(ds3231.getMonth(century),DEC);
  //     Serial.print("-");
  //     Serial.print(ds3231.getDate(),DEC);
  //     Serial.print(" (");
  //     Serial.print(ds3231.getDoW(),DEC);
  //     Serial.print(") ");
  //     Serial.print(ds3231.getHour(h12,PM),DEC);
  //     Serial.print(":");
  //     Serial.print(ds3231.getMinute(),DEC);
  //     Serial.print(":");
  //     Serial.print(ds3231.getSecond(),DEC);
  //     Serial.println();
  //   }
  // }
}