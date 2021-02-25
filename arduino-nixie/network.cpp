#ifdef NETWORK //only compile when requested (when included in main file)
#ifndef NETWORK_SRC //include once only
#define NETWORK_SRC

#ifndef __AVR__ //TODO better sensor
  //do stuff for wifinina
#else
  //any placeholder functions?
#endif

#endif
#endif