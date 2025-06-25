
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

uint32_t    dump = 0;


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
float signal[BLOCK_SIZE];
float signal_bpf[BLOCK_SIZE];

// Handle to the fir instances
fir_handle_t bpf, lpf_a, lpf_b;


float cos_t[BLOCK_SIZE];
float msin_t[BLOCK_SIZE];

float q[BLOCK_SIZE];
float i[BLOCK_SIZE];
float q_lpf[BLOCK_SIZE];
float i_lpf[BLOCK_SIZE];
//uint8_t bits[BLOCK_SIZE];

struct comp_t {
    float re, im;
};

typedef struct comp_t comp_t;

comp_t z[BLOCK_SIZE];
comp_t z_conj[BLOCK_SIZE];
comp_t z_conj_p;
comp_t y;

void comp(comp_t *dest, float re, float im) {
    dest->re = re;
    dest->im = im;
}

void comp_conj(comp_t *dest, const comp_t src) {
    dest->re = src.re;
    dest->im = src.im * -1.0;
}

void comp_mult(comp_t* dest, const comp_t a, const comp_t b) {
    dest->re = (a.re * b.re) - (a.im * b.im);
    dest->im = (a.re * b.im) + (a.im * b.re);
}

void process_init(){
    // Precalc cos en -sin tables to convert to IQ
    for(uint16_t n = 0; n < BLOCK_SIZE; n++) {
        cos_t[n] = 1.0 * cos(2.0 * M_PI * n * 1000.0 / 4000.0);
        msin_t[n] = -1.0 * sin(2.0 * M_PI * n * 1000.0 / 4000.0);
    }
    
    bpf = fir_init(bpf_coeffs, BPF_NUMTAPS, BLOCK_SIZE, 1.0);
    lpf_a = fir_init(lpf_coeffs, LPF_NUMTAPS, BLOCK_SIZE, 1.0);
    lpf_b = fir_init(lpf_coeffs, LPF_NUMTAPS, BLOCK_SIZE, 1.0);
}

// 
// Entry point of processing. 
//
void process(const uint16_t *inp, uint8_t *outp, uint16_t size) {

    // Convert samples to float
    samples_to_float(inp, signal, BLOCK_SIZE, 255.0f);

    // Remove DC -> Highpass fc = 400Hz
    fir_update(bpf, signal, signal_bpf, BLOCK_SIZE);

    // fsk demodulate:


    // Calculate IQ: (multiply with cos() and -sin()) and lowpass 200 Hz
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        q[idx] = signal_bpf[idx] * cos_t[idx];
        i[idx] = signal_bpf[idx] * msin_t[idx];
    }
    fir_update(lpf_a, q, q_lpf, BLOCK_SIZE);
    fir_update(lpf_b, i, i_lpf, BLOCK_SIZE);


    // Convert to complex z=a+bj and conj(z)
    for(uint16_t n = 0; n < BLOCK_SIZE; n++) {
        comp(&z[n], q_lpf[n], i_lpf[n]);
        comp_conj(&z_conj[n], z[n]);    
        if(n == 0) {
            comp_mult(&y, z[n], z_conj_p);
            z_conj_p = z[BLOCK_SIZE-1];
        } else {
            comp_mult(&y, z[n], z_conj[n-1]);
        }
        
        // pack bits into outp. E.g.
        if( atan2(y.im, y.re) >= 0) {
            outp[n/8] |= (0x01 << (7-(n%8)));
        } else {
            outp[n/8] &= ~(0x01 << (7-(n%8)));
        }


//        if( atan2(y.im, y.re) >= 0.0) {
//            outp[n] = 1;
//        } else {
//            outp[n] = 0;
//        }
    }

//    dump++; 
    if( dump >= 20 ) {
    
        for(uint16_t n = 0; n < BLOCK_SIZE; n++) {
            printf("%d;%f;%f;%f;%f;%f;%f;%d\n",
                inp[n],
                signal[n],
                signal_bpf[n],
                q[n],
                i[n],
                q_lpf[n],
                i_lpf[n],
                outp[n]
            );

        }
    }

        
    // Convert back to uint16_t for pwm
/*    for(uint16_t idx = 0; idx < size; idx++) {
        for(uint16_t idy = 0; idy < 32; idy++) {
            outp[32*idx+idy] = (uint16_t) (127.0f + bits[idx] * 255.0f);
        }
    }
*/

}



