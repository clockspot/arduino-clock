#ifndef DATETIME_H
#define DATETIME_H

#include <stdint.h>

// Allow compilation outside the Arduino framework (e.g. PlatformIO native
// for unit tests) where byte/word are not predefined.
#ifndef ARDUINO
  typedef uint8_t byte;
  typedef uint16_t word;
#endif

byte daysInMonth(word y, byte m);
int daysInYear(word y);
byte dayOfWeek(word y, byte m, byte d);
byte nthSunday(int y, byte m, int8_t nth);
int dateToDayCount(word y, byte m, byte d);
int dateComp(int y, byte m, byte d, byte mt, byte dt, bool countUp);
bool isTimeInRange(word tstart, word tend, word ttest);
bool isDayInRange(byte dstart, byte dend, byte dtest);

#endif
