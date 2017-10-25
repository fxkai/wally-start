// Include Guard ------------------------------------

#ifndef LOW_LEVEL_H
#define LOW_LEVEL_H
//#ifdef LINUX
//#include <bcm2835.h>
//#else
#include "wallypixel.h"
//#endif
#include <stdint.h>
#include <signal.h>   // some kind of magic I guess.  also, keyboard interrupts
#include "config.h"


void writeByte(uint8_t byte);

void flushBuffer(int length = NUM_LEDS);



#endif  // LOW_LEVEL_H
