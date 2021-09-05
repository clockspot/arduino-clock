

#include <Arduino_LSM6DS3.h> //brings IMU object

//inputs for Sel/Alt/Up/Dn, use some bogus ones
#define CTRL_SEL 100
#define CTRL_ALT 101
#define CTRL_UP 102
#define CTRL_DN 103

//How is the Arduino oriented inside the clock? Include one each of USB_DIR and IC_DIR to indicate which way the USB port and IC (front side) are oriented, respectively. For UNDB clocks, it's USB_DIR_UP and IC_DIR_BACK.
// #define USB_DIR_UP
// #define USB_DIR_DOWN
// #define USB_DIR_LEFT
// #define USB_DIR_RIGHT
// #define USB_DIR_FRONT
#define USB_DIR_BACK
// #define IC_DIR_UP
#define IC_DIR_DOWN
// #define IC_DIR_LEFT
// #define IC_DIR_RIGHT
// #define IC_DIR_FRONT
// #define IC_DIR_BACK

#define IMU_DEBOUNCING 150 //ms

// int imuRoll = 0; //the state we're reporting (-1, 0, 1)
// unsigned long imuRollLast = 0; //when we saw it change
// int imuPitch = 0;
// unsigned long imuPitchLast = 0;
// int imuYaw = 0;
// unsigned long imuYawLast = 0;
//int imuLastRead = 0; //for debug
void readIMU(){
  
  //IMU.readAcceleration will give us three values, but we only care about two,
  //since we are only reading the clock being tilted left/right (roll) and back/front (pitch).
  float roll, pitch, nah;

  /*
  Now we decide how to read the IMU into those values, per the orientation of the Nano:
  USB   IC     Roll Pitch
  Up    Front    y   -z
        Back    -y    z
        Left     z    y
        Right   -z   -y
  Down  Front   -y   -z
        Back     y    z
        Left     z   -y
        Right   -z    y
  Left  Front    x   -z
        Back     x    z
        Up       x   -y
        Down     x    y
  Right Front   -x   -z
        Back    -x    z
        Up      -x   -y
        Down    -x    y
  Front Left     z   -x
        Right   -z   -x
        Up      -y   -x
        Down     y   -x
  Back  Left     z    x
        Right   -z    x
        Up       y    x
        Down    -y    x
  Or, more succinctly:
  USB Left:  Roll  = x
  USB Right: Roll  = -x
  IC  Left:  Roll  = z
  IC  Right: Roll  = -z
  USB Front: Pitch = -x
  USB Back:  Pitch = x
  IC  Front: Pitch = -z
  IC  Back:  Pitch = z
  ...and then a bunch of nonsense to capture y  >:(
  There might be a clever way to encode this in future,
  but for now let's hardcode it with preprocessor directives
  TODO may also want to apply an offset in case the clock is naturally slightly tilted?
  */

  #ifdef USB_DIR_UP
    #ifdef IC_DIR_FRONT //y, -z (roll, pitch)
      IMU.readAcceleration(nah, roll, pitch); pitch = -pitch;
    #endif
    #ifdef IC_DIR_BACK //-y, z
      IMU.readAcceleration(nah, roll, pitch); roll = -roll;
    #endif
    #ifdef IC_DIR_LEFT //z, y
      IMU.readAcceleration(nah, pitch, roll);
    #endif
    #ifdef IC_DIR_RIGHT //-z, -y
      IMU.readAcceleration(nah, pitch, roll); roll = -roll; pitch = -pitch;
    #endif
  #endif //USB_DIR_UP

  #ifdef USB_DIR_DOWN
    #ifdef IC_DIR_FRONT //-y, -z
      IMU.readAcceleration(nah, roll, pitch); roll = -roll; pitch = -pitch;
    #endif
    #ifdef IC_DIR_BACK //y, z
      IMU.readAcceleration(nah, roll, pitch);
    #endif
    #ifdef IC_DIR_LEFT //z, -y
      IMU.readAcceleration(nah, pitch, roll); pitch = -pitch;
    #endif
    #ifdef IC_DIR_RIGHT //-z, y
      IMU.readAcceleration(nah, pitch, roll); roll = -roll;
    #endif
  #endif //USB_DIR_DOWN

  #ifdef USB_DIR_LEFT
    #ifdef IC_DIR_FRONT //x, -z
      IMU.readAcceleration(roll, nah, pitch); pitch = -pitch;
    #endif
    #ifdef IC_DIR_BACK //x, z
      IMU.readAcceleration(roll, nah, pitch);
    #endif
    #ifdef IC_DIR_UP //x, -y
      IMU.readAcceleration(roll, pitch, nah); pitch = -pitch;
    #endif
    #ifdef IC_DIR_DOWN //x, y
      IMU.readAcceleration(roll, pitch, nah);
    #endif
  #endif //USB_DIR_LEFT

  #ifdef USB_DIR_RIGHT
    #ifdef IC_DIR_FRONT //-x, -z
      IMU.readAcceleration(roll, nah, pitch); roll = -roll; pitch = -pitch;
    #endif
    #ifdef IC_DIR_BACK //-x, z
      IMU.readAcceleration(roll, nah, pitch); roll = -roll;
    #endif
    #ifdef IC_DIR_UP //-x, -y
      IMU.readAcceleration(roll, pitch, nah); roll = -roll; pitch = -pitch;
    #endif
    #ifdef IC_DIR_DOWN //-x, y
      IMU.readAcceleration(roll, pitch, nah); roll = -roll;
    #endif
  #endif //USB_DIR_RIGHT

  #ifdef USB_DIR_FRONT
    #ifdef IC_DIR_LEFT //z, -x
      IMU.readAcceleration(pitch, nah, roll); pitch = -pitch;
    #endif
    #ifdef IC_DIR_RIGHT //-z, -x
      IMU.readAcceleration(pitch, nah, roll); roll = -roll; pitch = -pitch;
    #endif
    #ifdef IC_DIR_UP //-y, -x
      IMU.readAcceleration(pitch, roll, nah); roll = -roll; pitch = -pitch;
    #endif
    #ifdef IC_DIR_DOWN //y, -x
      IMU.readAcceleration(pitch, roll, nah); pitch = -pitch;
    #endif
  #endif //USB_DIR_FRONT

  #ifdef USB_DIR_BACK
    #ifdef IC_DIR_LEFT //z, x
      IMU.readAcceleration(pitch, nah, roll);
    #endif
    #ifdef IC_DIR_RIGHT //-z, x
      IMU.readAcceleration(pitch, nah, roll); roll = -roll;
    #endif
    #ifdef IC_DIR_UP //y, x
      IMU.readAcceleration(pitch, roll, nah);
    #endif
    #ifdef IC_DIR_DOWN //-y, x
      IMU.readAcceleration(pitch, roll, nah); roll = -roll;
    #endif
  #endif //USB_DIR_BACK

  Serial.print(F("roll ")); Serial.print((int)(roll*3),DEC);
  Serial.print(F("   pitch ")); Serial.print((int)(pitch*3),DEC);
  Serial.println();
  
  // int imuState;
  //
  // //Assumes Arduino is oriented with components facing back of clock, and USB port facing up. TODO add support for other orientations
  //
  // //Roll
  // if((unsigned long)(millis()-imuRollLast)>=IMU_DEBOUNCING){ //don't check within a period from the last change
  //        if(y<=-0.5) imuState = 1;
  //   else if(y>= 0.5) imuState = -1;
  //   else if(y>-0.3 && y<0.3) imuState = 0;
  //   else imuState = imuRoll; //if it's borderline, treat it as "same"
  //   if(imuRoll != imuState){ imuRoll = imuState; imuRollLast = millis(); } //TODO maybe add audible feedback
  // }
  //
  // //Pitch
  // if((unsigned long)(millis()-imuPitchLast)>=IMU_DEBOUNCING){ //don't check within a period from the last change
  //        if(z<=-0.5) imuState = 1;
  //   else if(z>= 0.5) imuState = -1;
  //   else if(z>-0.3 && z<0.3) imuState = 0;
  //   else imuState = imuPitch; //if it's borderline, treat it as "same"
  //   if(imuPitch != imuState){ imuPitch = imuState; imuPitchLast = millis(); }
  // }
  //
  // //Yaw
  //
  
}


////////// Main code control //////////

void setup(){
  IMU.begin();
  
  Serial.begin(9600);
  // #ifndef __AVR__ //SAMD only
  while(!Serial);
  // #endif
}

unsigned long lastRead = 0;
void loop(){
  if((unsigned long)(millis()-lastRead)>1000) {
    lastRead = millis();
    readIMU();
  }
}