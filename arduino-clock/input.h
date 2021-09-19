#ifndef INPUT_H
#define INPUT_H

#ifdef INPUT_IMU
  //If we don't already have inputs defined for Sel/Alt/Up/Dn, use some bogus ones
  #ifndef CTRL_SEL
    #define CTRL_SEL 100
  #endif
  #ifndef CTRL_ALT
    #define CTRL_ALT 101
  #endif
  #ifndef CTRL_UP
    #define CTRL_UP 102
  #endif
  #ifndef CTRL_DN
    #define CTRL_DN 103
  #endif
#endif

#ifdef INPUT_IMU
void readIMU(unsigned long now);
#endif
bool initInputs();
bool readBtn(byte btn);
void checkBtn(byte btn, unsigned long now);
void inputStop();
#ifdef INPUT_UPDN_ROTARY
void checkRot(unsigned long now);
#endif
void checkInputs();
void setInputLast(unsigned long increment=0);
unsigned long getInputLast();
int getInputLastTODMins();

#endif //INPUT_H