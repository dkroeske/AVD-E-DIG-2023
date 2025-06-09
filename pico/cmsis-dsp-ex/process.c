
//
#include "stdio.h"
#include "process.h"
//#include "fir.h"
#include "bandpass.h"
#include "math.h"

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

// used for fir
float fir_in[BLOCK_SIZE];
float fir_out[BLOCK_SIZE];

// Handle to the fir instances
fir_handle_t bpf, q_lpf, i_lpf;


float cos_t[BLOCK_SIZE];
float msin_t[BLOCK_SIZE];

float q[BLOCK_SIZE];
float i[BLOCK_SIZE];
float z[BLOCK_SIZE * 2];
float z_conj[BLOCK_SIZE * 2];
float y[BLOCK_SIZE*2];


void process_init(){
    // Precalc cos en -sin tables to convert to IQ
    for(uint16_t n = 0; n < BLOCK_SIZE; n++) {
        cos_t[n] = 1.0 * cos(2.0 * M_PI * n * 1000.0 / 4000.0);
        msin_t[n] = -1.0 * sin(2.0 * M_PI * n * 1000.0 / 4000.0);
    }
    
    bpf = fir_init(bpf_coeffs, BPF_NUMTAPS, BLOCK_SIZE, 1.0);
    q_lpf = fir_init(lpf_coeffs, LPF_NUMTAPS, BLOCK_SIZE, 1.0);
    i_lpf = fir_init(lpf_coeffs, LPF_NUMTAPS, BLOCK_SIZE, 1.0);
}

// Entry point of processing. 
void process(const uint16_t *inp, uint16_t *outp, uint16_t size) {

    // Convert samples to float
    samples_to_float(inp, fir_in, BLOCK_SIZE, 255.0f);

    // Remove DC -> Highpass fc = 400Hz
    fir_update(bpf, fir_in, fir_out, BLOCK_SIZE);

    // fsk demodulate:

    // Calculate IQ: (multiply with cos() and -sin()) and lowpass 200 Hz
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        q[idx] = fir_out[idx] * cos_t[idx];
        i[idx] = fir_out[idx] * msin_t[idx];
    }
    fir_update(q_lpf, q, fir_out, BLOCK_SIZE);
    fir_update(i_lpf, i, fir_out, BLOCK_SIZE);

    // Convert to complex z=a+bj and conj(z)
    for(uint16_t n = 0; n < BLOCK_SIZE; n++) {
        z[(2*n)] = q[(n)];
        z[(2*n)+1] = i[n];
        z_conj[(2*n)] = q[n];
        z_conj[(2*n)+1] = -1.0 * i[n];
    }
    
        
    // calculate phase from y[n]

    // If phase <= 0? bit = 0; bit = 1; 

    // decode bit-stream

    // Convert back to uint16_t for pwm
    for(uint16_t idx = 0; idx < size; idx++) {
        for(uint16_t idy = 0; idy < 32; idy++) {
            outp[32*idx+idy] = (uint16_t) (127.0f + fir_out[idx] * 255.0f);
        }
    }

/*
    printf("----------\nBLOCK_SIZE = %d\n", BLOCK_SIZE);
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        printf("%d %d, %f %f\n", idx, inp[idx], fir_in[idx], c.out[idx]);
    }
*/


}



