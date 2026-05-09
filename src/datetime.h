#ifndef DATETIME_H
#define DATETIME_H

// byte/word come from Arduino.h when building under the Arduino framework;
// outside it (e.g. PlatformIO native for host-side unit tests) we typedef them
// from stdint.h so this header is self-contained either way.
#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <stdint.h>
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
