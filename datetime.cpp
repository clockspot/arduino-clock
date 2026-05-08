#include "datetime.h"

byte daysInMonth(word y, byte m){
  if(m==2) return (y%4==0 && (y%100!=0 || y%400==0) ? 29 : 28);
  // https://cmcenroe.me/2014/12/05/days-in-month-formula.html
  else return (28 + ((m + (m/8)) % 2) + (2 % m) + (2 * (1/m)));
}

int daysInYear(word y){
  return 337 + daysInMonth(y,2);
}

int dateToDayCount(word y, byte m, byte d){
  int dc = 0;
  for(byte i=1; i<m; i++) dc += daysInMonth(y,i);
  dc += d-1;
  return dc;
}

byte dayOfWeek(word y, byte m, byte d){
  // 0=Sun..6=Sat. Per https://en.wikipedia.org/wiki/Zeller%27s_congruence
  byte yb = y%100;
  byte ya = y/100;
  if(m<3) { m+=10; if(yb==0) { yb=99; ya-=1; } else yb-=1; }
  else m -= 2;
  return (d + ((13*m-1)/5) + yb + (yb/4) + (ya/4) + 5*ya) %7;
}

byte nthSunday(int y, byte m, int8_t nth){
  // nth must be signed: callers pass negative values (e.g. -1) for "last
  // Sunday of the month". Declaring nth as byte (uint8_t) silently broke all
  // non-US DST rules.
  if(nth>0) return (((7-dayOfWeek(y,m,1))%7)+1+((nth-1)*7));
  if(nth<0) return (dayOfWeek(y,m,1)==0 && daysInMonth(y,m)>28? 29: nthSunday(y,m,1)+21+((nth+1)*7));
  return 0;
}

int dateComp(int y, byte m, byte d, byte mt, byte dt, bool countUp){
  // If m+d is later   { if count up from, use last year, else use this year }
  // If m+d is earlier { if count down to, use next year, else use this year }
  bool targetDir = mt*100+dt>=m*100+d+(countUp&&!(mt==12&&dt==31)?1:0);
  int targetYear = (countUp && targetDir? y-1: (!countUp && !targetDir? y+1: y));
  int targetDayCount = dateToDayCount(targetYear, mt, dt);
  if(targetYear<y) targetDayCount -= daysInYear(targetYear);
  if(targetYear>y) targetDayCount += daysInYear(y);
  long currentDayCount = dateToDayCount(y,m,d);
  long diff = targetDir ? currentDayCount-targetDayCount : targetDayCount-currentDayCount;
  return diff < 0 ? -diff : diff;
}

bool isTimeInRange(word tstart, word tend, word ttest) {
  // Times are minutes since midnight, 0-1439.
  // tstart<tend: in range if >=tstart AND <tend
  // tstart>tend (range crosses midnight): in range if >=tstart OR <tend
  // tstart==tend: nothing in range
  return ( (tstart<tend && ttest>=tstart && ttest<tend) || (tstart>tend && (ttest>=tstart || ttest<tend)) );
}

bool isDayInRange(byte dstart, byte dend, byte dtest) {
  // Like isTimeInRange but inclusive on both ends (always at least one match).
  return ( (dstart<=dend && dtest>=dstart && dtest<=dend) || (dstart>dend && (dtest>=dstart || dtest<=dend)) );
}
