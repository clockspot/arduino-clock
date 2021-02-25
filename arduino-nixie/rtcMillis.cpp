#ifdef RTC_MILLIS //only compile when requested (when included in main file)
#ifndef RTC_MILLIS_SRC //include once only
#define RTC_MILLIS_SRC

////////// FAKE RTC using millis //////////

//#include "Arduino.h" //not necessary, since these get compiled as part of the main sketch

//snapshot of time of day
unsigned long todMils = 0; //time of day in milliseconds
int todY = 2020;
byte todM = 1;
byte todD = 1;
byte todW = 0;
unsigned long millisAtTOD = 0; //reflects millis at snapshot

void rtcInit(){}
void rtcSetTime(byte h, byte m, byte s){
  todMils = (h*3600000)+(m*60000)+(s*1000);
  millisAtTOD = millis();
}
void rtcSetDate(int y, byte m, byte d, byte w){
  todY = y; todM = m; todD = d; todW = w;
}
void rtcSetHour(byte h){
  todMils = (h*3600000)+(todMils%60000);
  millisAtTOD = millis();
}

void rtcTakeSnap(){
  unsigned long millisNow = millis();
  //Increment todMils per the change in millis
  todMils += millisNow-millisAtTOD;
  
  //Apply anti-drift
  unsigned long drift = (abs(ANTI_DRIFT) * (millisNow-millisAtTOD))/1000;
  if(ANTI_DRIFT>0) todMils += drift;
  if(ANTI_DRIFT<0) todMils -= drift;
  //any issues with data types/truncation here?
  
  //Update the millis snap
  millisAtTOD = millisNow;
  //Handle midnight rollover
  //This may behave erratically if rtcTakeSnap() is not called for long enough that todMils rolls over,
  //but this should not happen, because except for short hangs (eg wifi connect) we should be calling it at least 1/sec
  if(todMils >= 86400000){
    while(todMils >= 86400000) todMils = todMils - 86400000; //while is just to ensure it's below 86400000
    if(todD==daysInMonth(todY,todM)){ todD = 1; todM++; if(todM==13){ todM==1; todY++; } }
    else todD++;
    todW++; if(todW>6) todW=0;
  }
}

int  rtcGetYear(){ return todY; }
byte rtcGetMonth(){ return todM; }
byte rtcGetDate(){ return todD; }
byte rtcGetWeekday(){ return todW; }
int  rtcGetTOD(){ return (todMils/1000)/60; }
byte rtcGetHour(){ return (todMils/1000)/3600; }
byte rtcGetMinute(){ return ((todMils/1000)/60)%60; }
byte rtcGetSecond(){ return (todMils/1000)%60; }

byte rtcGetTemp(){ return 1000; } //a fake response - ten degrees (1000 hundredths) forever

#endif
#endif