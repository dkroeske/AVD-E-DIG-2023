/* *********************************************************

Sampler.h 
ADC ping/pong sampler using dma with callback

***********************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"



#define ADC_DMA_DEBUG        

#ifdef ADC_DMA_DEBUG
#define DEBUG_LED_PING  16
#define DEBUG_LED_PONG  17
#endif

// ADC
#define F_ADC           48000000
#define FS_ADC              2000
#define ADC_CLKDIV          (float)(F_ADC/FS_ADC)
//#define ADC_CAPTURE_DEPTH   8192*2 
#define ADC_CAPTURE_DEPTH   64 


/* ****************************************************** */
void adc_dma_set_ping_cb( void (*f)(uint8_t *) );
/*
short   : adc/dma set callback function
inputs  : 
outputs :
notes   :
version : DMK - initial version
***********************************************************/

/* ****************************************************** */
void adc_dma_set_pong_cb( void (*f)(uint8_t *) );
/*
short   : adc/dma set callback function
inputs  : 
outputs :
notes   :
version : DMK - initial version
***********************************************************/

/* ****************************************************** */
void adc_dma_init();
/*
short   : ADC / DMA init
inputs  : 
outputs :
notes   :
***********************************************************/
