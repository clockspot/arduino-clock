//https://github.com/adafruit/Adafruit_VEML7700/blob/master/examples/veml7700_test/veml7700_test.ino

#include <arduino.h>
#include "arduino-clock.h"

#ifdef LIGHTSENSOR_VEML7700 //see arduino-clock.ino Includes section

#include "lightsensorVEML7700.h"
#include <Adafruit_VEML7700.h>

Adafruit_VEML7700 veml = Adafruit_VEML7700();

//these can be overridden in your config .h
#ifndef LUX_FULL
#define LUX_FULL 500 //lux at/above which display should be at its brightest (per config)
#endif
#ifndef LUX_DIM
#define LUX_DIM 30 //lux at/below which display should be at its dimmest (per config)
#endif

void initLightSensor() {
  Serial.println(F("Initiating light sensor"));
  veml.begin();
  //Gain and integration time put a limit on the max lux we can detect
  //see: https://forums.adafruit.com/viewtopic.php?f=19&t=165473&p=840089#p829281
  veml.setGain(VEML7700_GAIN_1_4);
  veml.setIntegrationTime(VEML7700_IT_200MS);

  // Serial.print(F("Gain: "));
  // switch (veml.getGain()) {
  //   case VEML7700_GAIN_1: Serial.println("1"); break;
  //   case VEML7700_GAIN_2: Serial.println("2"); break;
  //   case VEML7700_GAIN_1_4: Serial.println("1/4"); break;
  //   case VEML7700_GAIN_1_8: Serial.println("1/8"); break;
  // }
  //
  // Serial.print(F("Integration Time (ms): "));
  // switch (veml.getIntegrationTime()) {
  //   case VEML7700_IT_25MS: Serial.println("25"); break;
  //   case VEML7700_IT_50MS: Serial.println("50"); break;
  //   case VEML7700_IT_100MS: Serial.println("100"); break;
  //   case VEML7700_IT_200MS: Serial.println("200"); break;
  //   case VEML7700_IT_400MS: Serial.println("400"); break;
  //   case VEML7700_IT_800MS: Serial.println("800"); break;
  // }
  
  //veml.powerSaveEnable(true);
  //veml.setPowerSaveMode(VEML7700_POWERSAVE_MODE4);

  // veml.setLowThreshold(10000);
  // veml.setHighThreshold(20000);
  veml.interruptEnable(false);

} //end initLightSensor

byte lastRelAmb = 0; //we won't report a change unless relative ambient light changes sufficiently
#define REL_AMB_THRESHOLD 3 //sufficiently = 3/255 of max-min
//unsigned long lastPrint = 0;
byte getRelativeAmbientLightLevel() {
  int lux = (int)veml.readLux();
  //if((unsigned long)(millis()-lastPrint)>500) { lastPrint = millis(); Serial.println(lastPrint,DEC); }
  Serial.println(lux);
  if(lux <= LUX_DIM) lux = 0;
  else if(lux >= LUX_FULL) lux = 255;
  else lux = (long)(lux - LUX_DIM) * 255 / (LUX_FULL - LUX_DIM);
  //if lux is within range, don't actually change ambientLight unless it's within 3 of what it was before,
  //so the display won't flicker if it's right on a threshold (with steady light)
  //also reduces Serial writes
  if(((lux==0 || lux==255) && lastRelAmb!=0 && lastRelAmb!=255) //if we're minned/maxxed, and last wasn't that;
    || lux-lastRelAmb>=REL_AMB_THRESHOLD || lux-lastRelAmb<=-REL_AMB_THRESHOLD) { //TODO is this sufficient wrt rollovers?
    lastRelAmb = lux;
    Serial.print("New RelAmb: "); Serial.println(lastRelAmb,DEC);
  }
  return lastRelAmb;
}

#endif //LIGHTSENSOR_VEML7700