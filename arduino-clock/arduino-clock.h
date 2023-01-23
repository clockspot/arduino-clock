#ifndef ARDUINO_CLOCK_H
#define ARDUINO_CLOCK_H

////////// Hardware configuration //////////
//Include the config file that matches your hardware setup. If needed, duplicate an existing one.

#include "configs/esp32-braun.h"

////////////////////////////////////////////


//Unique IDs for the functions (value is arbitrary) - see fnScroll and checkRTC for how these are manually or automatically scrolled through
#define FN_TOD 0 //time of day
#define FN_DATE 1
#define FN_DAY_COUNTER 8
#define FN_SUN_LAST 9
#define FN_SUN_NEXT 10
#define FN_WEATHER_LAST 11
#define FN_WEATHER_NEXT 12
#define FN_DATE_AUTO 13 //same as FN_DATE but not followed by sun/weather functions – for optionally triggering at :30 seconds (and scrolling)
#define FN_ALARM 2 //alarm time
#define FN_ALARM2 7 //second alarm time //TODO
#define FN_TIMER 3 //countdown timer and chronograph
#define FN_THERMOMETER 4 //temperature per rtc – will likely read high
#define FN_TUBETEST 5 //simply cycles all digits for nixie tube testing
#define FN_VERSION 6 //displays software version
#define FN_OPTS 201 //fn values from here to 255 correspond to settings in the settings menu

//NEW
byte getCurFn();
void setCurFn(byte val);
void goToFn(byte val);
bool getFnIsSetting();
byte getSignalSource();
word getSignalRemain();
byte getSnoozeRemain();
bool getTimerRun();
bool getTimerDir();
bool getTimerRunoutRepeat();
bool getTimerRunoutChrono();
bool getTimerLapDisplay();
unsigned long getTimerDuration();
byte getDisplayBrightness();

void setup();
void loop();
void fnScroll(byte dir);
void fnOptScroll(byte dir);
void switchAlarmState(byte dir, byte whichAlarm);
void setAlarmState(byte state, byte whichAlarm);
byte getAlarmState(byte whichAlarm);
void switchPower(byte dir);
void startSet(int n, int m, int x, byte p);
void doSet(int delta);
void clearSet();
void setByFn(); //NEW
void setDate(); //NEW
void setDayCounter(); //NEW
void setTime(); //NEW
void setAlarm(byte whichAlarm); //NEW
void setTimer(); //NEW
void setOpt(byte opt); //NEW
void startSnooze(); //NEW
void stopSnooze(); //NEW
bool initEEPROM(bool hard);
void checkRTC(bool force);
void fibonacci(byte h, byte m, byte s);
void autoDST();
bool isDST(int y, byte m, byte d);
bool isDSTByHour(int y, byte m, byte d, byte h, bool setFlag);
byte nthSunday(int y, byte m, byte nth);
byte daysInMonth(word y, byte m);
int daysInYear(word y);
int dateToDayCount(word y, byte m, byte d);
byte dayOfWeek(word y, byte m, byte d);
int dateComp(int y, byte m, byte d, byte mt, byte dt, bool countUp);
bool isTimeInRange(word tstart, word tend, word ttest);
bool isDayInRange(byte dstart, byte dend, byte dtest);
void millisCheckDrift();
void millisApplyDrift();
void millisReset();
unsigned long ms();
void timerStart();
void timerStop();
void convertTimerTime(bool mode); //NEW
void timerClear();
void timerLap();
void timerRunoutToggle();
void cycleTimer();
void timerSleepSwitch(bool on);
byte getTimerState();
void setTimerState(char pos, bool val);
void startUnoff(); //NEW
void tempDisplay(int i0, int i1=0, int i2=0, int i3=0);
void updateDisplay();
void calcSun();
void displaySun(byte which, int d, int tod);
void displayWeather(byte which);
void initOutputs();
void signalStart(byte sigFn, byte sigDur);
void signalStop();
void cycleSignal();
word getSignalPitch();
word getHz(byte note);
byte getSignalOutput();
byte getSignalPattern();
void quickBeep(int pitch);
void quickBeepPattern(int source, int pattern);
void updateBacklight();
void cycleTweening();
byte getVersionPart(byte part);

#endif //ARDUINO_CLOCK_H