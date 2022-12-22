#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

Adafruit_7segment matrix = Adafruit_7segment();

#include <Adafruit_VEML7700.h>

Adafruit_VEML7700 veml = Adafruit_VEML7700();

void setup(){
  Serial.begin(9600);
  //while(!Serial); //SAMD only
  
  matrix.begin(0x70);
  matrix.setBrightness(7);
  
  veml.begin(); //uses 0x10
  //see: https://forums.adafruit.com/viewtopic.php?f=19&t=165473&p=840089#p829281
  veml.setGain(VEML7700_GAIN_1_4);
  veml.setIntegrationTime(VEML7700_IT_400MS);
  
  veml.setLowThreshold(0);
  veml.setHighThreshold(1);
  veml.interruptEnable(true);
  
  matrix.print(0,DEC);
  matrix.writeDisplay();
}

unsigned long lastSample = 0;
void loop(){
  // if((unsigned long)(millis()-lastSample)>=5000) {
  //   lastSample = millis();
  //
  //   Serial.print("Lux: "); Serial.print(veml.readLux());
  //   Serial.print("   Raw: "); Serial.print(veml.readALS());
  //   Serial.println();
  //
  //   matrix.print((int)veml.readLux());
  //   matrix.writeDisplay();
  // }
  
  uint16_t irq = veml.interruptStatus();
  if((irq & VEML7700_INTERRUPT_LOW) || (irq & VEML7700_INTERRUPT_HIGH)) {
    word als = veml.readALS();
    veml.setLowThreshold(als<100? 0: als-100);
    veml.setHighThreshold(als>65435? 0: als+100);
    matrix.print((int)veml.readLux(),DEC);
    matrix.writeDisplay();
    Serial.print("Lux: "); Serial.print(veml.readLux(),DEC);
    Serial.print("   Raw: "); Serial.print(veml.readALS(),DEC);
    Serial.print("   Next: "); Serial.print(als-100,DEC); Serial.print(F("<x<")); Serial.print(als+100,DEC);
    Serial.println();
  }
}