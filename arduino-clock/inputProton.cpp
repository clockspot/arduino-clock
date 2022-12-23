//"Proton" inputs are a more diverse set of controls, namely the buttons and switches on a Proton 320 clock radio

#include <arduino.h>
#include "arduino-clock.h"

#ifdef INPUT_PROTON //see arduino-clock.ino Includes section

#include "input-proton.h"

//Needs access to RTC timestamps  TODO does it?
#include "rtcDS3231.h"
#include "rtcMillis.h"

#ifndef HOLDSET_SLOW_RATE
#define HOLDSET_SLOW_RATE 125
#endif
#ifndef HOLDSET_FAST_RATE
#define HOLDSET_FAST_RATE 20
#endif
#ifndef DEBOUNCE_DUR
#define DEBOUNCE_DUR 150 //ms
#endif

//TODO port stuff from inputSimple

#endif //INPUT_PROTON