#ifndef PROCESS_INC
#define PROCESS_INC

#define BLOCK_SIZE 1024

#include "arm_math.h"
#include "arm_const_structs.h"

void do_fft();
void process_init();
void process(const uint16_t *inp, uint8_t *outp, uint16_t size);
void samples_to_float(const uint16_t *src, float *dest, uint16_t size, float scaling);

#endif
