
//
#include "stdio.h"
#include "process.h"
#include "fir.h"

//
float fft_inp_4khz[BLOCK_SIZE];
float fft_outp_4khz[BLOCK_SIZE];
float fft_power_4khz[BLOCK_SIZE/2];

//
uint8_t     ifftFlag = 0;
arm_rfft_fast_instance_f32 rfft;
float32_t   maxValue;
uint32_t    maxIndex;


uint16_t buf_size = 0;

void do_fft() {

    // init
    arm_rfft_fast_init_f32(&rfft, BLOCK_SIZE);
    // process cfft
    arm_rfft_fast_f32(&rfft, fft_inp_4khz, fft_outp_4khz, ifftFlag);
    // calc mag
    arm_cmplx_mag_f32(fft_outp_4khz, fft_power_4khz, BLOCK_SIZE/2);
}

void samples_to_float(const uint16_t *src, float *dest, uint16_t size, float scaling) {
    for(uint16_t idx = 0; idx < size; idx++) {
        dest[idx] = (float) (src[idx] / scaling);
    }
}

extern const float ConvDirect_Kernel[CONVDIRECT_KERNEL_LENGHT];
float fir_in[BLOCK_SIZE];

ConvDirect_Container c; 

void process_init() {
    ConvDirect_Init(&c, ConvDirect_Kernel);
}

// Entry point of processing. 
void process(const uint16_t *inp, uint16_t *outp, uint16_t size) {

    // Convert samples to float
    samples_to_float(inp, fir_in, BLOCK_SIZE, 255.0f);

    // do filter
    ConvDirect_Update( &c, fir_in);

    // Convert back to uint16_t for pwm
    for(uint16_t idx = 0; idx < size; idx++) {
        for(uint16_t idy = 0; idy < 32; idy++) {
            outp[32*idx+idy] = (uint16_t) (127.0f + c.out[idx] * 255.0f);
    //        outp[32*idx+idy] = (uint16_t) (127.0f + fir_in[idx] * 255.0f);
        }
    }

/*
    printf("----------\nBLOCK_SIZE = %d\n", BLOCK_SIZE);
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        printf("%d %d, %f %f\n", idx, inp[idx], fir_in[idx], c.out[idx]);
    }
*/


}



