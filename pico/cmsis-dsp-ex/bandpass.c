/* cmsis FIR */

#include "process.h"
#include "stdlib.h"
#include "arm_math.h"
#include "assert.h"
#include "bandpass.h"


struct fir_t {
    arm_fir_instance_f32 instance;
    uint16_t numTaps;
    float *rpCoeffs;
    float *pState;
};

/*
 *
 */
fir_handle_t fir_init(const float *coeffs, uint16_t coeffs_size, uint16_t block_size, uint16_t gain ) {

    // New fir handler
    fir_handle_t fir = (fir_handle_t) malloc(sizeof(fir_t));
    assert(fir_handle_t);
   
    // Create and assign state variable array
    fir->pState = (float*) malloc((coeffs_size * block_size - 1) * sizeof(float));
    assert(fir->pState);
    
    // 
    fir->numTaps = coeffs_size;

    // Reverse load fir coeffs
    fir->rpCoeffs = (float *) malloc((coeffs_size)*sizeof(float)); 
    assert(fir->rpCoeffs);
    for(uint16_t idx = 0; idx < fir->numTaps; idx++) {
        fir->rpCoeffs[idx] = gain * coeffs[coeffs_size - idx - 1];
    }

    // Init fir filter    
    arm_fir_init_f32(&(fir->instance), fir->numTaps, fir->rpCoeffs, fir->pState, block_size);

    return fir;
}

/*
 *
 */
void fir_update(fir_handle_t f, float *inp, float *outp, uint16_t block_size) {
    arm_fir_f32( &(f->instance), inp, outp, block_size);
}


const float bpf_coeffs[BPF_NUMTAPS] = {
  -0.020886993395915293,
  -2.8445638410116745e-16,
  -0.029585282768933477,
  2.7597409651642794e-16,
  0.15658016016472287,
  -2.990357144911784e-16,
  -0.31207791782673594,
  8.918519517668997e-17,
  0.3834371873875004,
  8.918519517668997e-17,
  -0.31207791782673594,
  -2.990357144911784e-16,
  0.15658016016472287,
  2.7597409651642794e-16,
  -0.029585282768933477,
  -2.8445638410116745e-16,
  -0.020886993395915293
};

const float lpf_coeffs[LPF_NUMTAPS] = {
  -0.007775715121256268,
  -0.007938974136595613,
  -0.009534265788246128,
  -0.008779578259641298,
  -0.004381884421750879,
  0.004666131585205163,
  0.0188044731228937,
  0.03764144706001848,
  0.05992101812383003,
  0.08357444021744635,
  0.10601855702701225,
  0.12454015906119098,
  0.13674393462068657,
  0.14100385434561774,
  0.13674393462068657,
  0.12454015906119098,
  0.10601855702701225,
  0.08357444021744635,
  0.05992101812383003,
  0.03764144706001848,
  0.0188044731228937,
  0.004666131585205163,
  -0.004381884421750879,
  -0.008779578259641298,
  -0.009534265788246128,
  -0.007938974136595613,
  -0.007775715121256268
};


