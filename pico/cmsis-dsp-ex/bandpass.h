#ifndef BANDPASS_INCLUDE
#define BANDPASS_INCLUDE

typedef struct fir_t fir_t;

typedef fir_t *fir_handle_t;


fir_handle_t fir_init(const float *coeffs, uint16_t coeffs_size, uint16_t block_size, uint16_t gain );
void fir_update(fir_handle_t f, float *inp, float *outp, uint16_t block_size);

/* Input  Bandpass filter */
#define BPF_NUMTAPS 17
extern const float bpf_coeffs[];
/*
 filter designed with
 http://t-filter.appspot.com

sampling frequency: 4000 Hz

* 0 Hz - 500 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -30.90222503104154 dB

* 600 Hz - 1400 Hz
  gain = 0.8
  desired ripple = 5 dB
  actual ripple = 14.155920435314133 dB

* 1500 Hz - 2000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -30.90222503104154 dB

*/

#endif
