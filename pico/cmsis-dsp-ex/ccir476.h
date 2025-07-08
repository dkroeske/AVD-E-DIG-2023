#ifndef CCIR476_INC
#define CCIR476_INC

#include "ccir476.h"

void ccir476_rearm();

void ccir476_init( 
    void (*msg)(char *),     
    void (*c)(char ch), 
    void (err)(char *) );

void ccir476_process_bit(bool bit);

#endif
