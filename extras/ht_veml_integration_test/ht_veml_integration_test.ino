#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

Adafruit_7segment matrix = Adafruit_7segment();

#include <Adafruit_VEML7700.h>

Adafruit_VEML7700 veml = Adafruit_VEML7700();

void setup(){
  // Serial.begin(9600);
  // while(!Serial); //SAMD only
  
  matrix.begin(0x70);
  matrix.setBrightness(7);
  
  veml.begin(); //uses 0x10
  //see: https://forums.adafruit.com/viewtopic.php?f=19&t=165473&p=840089#p829281
  veml.setGain(VEML7700_GAIN_1_4);
  veml.setIntegrationTime(VEML7700_IT_400MS);
  veml.interruptEnable(false);
}

unsigned long lastSample = 0;
void loop(){
  if((unsigned long)(millis()-lastSample)>=500) {
    lastSample = millis();
    
    // Serial.print("Lux: "); Serial.print(veml.readLux());
    // Serial.print("   Raw: "); Serial.print(veml.readALS());
    // Serial.println();
    
    matrix.print((int)veml.readLux());
    matrix.writeDisplay();
  }
}