#ifndef INPUT_PROTON_H
#define INPUT_PROTON_H

bool initInputs();
bool readCtrl(byte ctrl);
void checkMomentary(byte ctrl, unsigned long now);
void checkSwitch(byte ctrl);
void inputStop();
void checkInputs();
void setInputLast(unsigned long increment=0);
unsigned long getInputLast();
int getInputLastTODMins();
void ctrlEvt(byte ctrl, byte evt, byte evtLast, bool velocity=0);

#endif //INPUT_PROTON_H