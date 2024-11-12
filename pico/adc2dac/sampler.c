/* *********************************************************

Sampler.c 
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
#include "sampler.h"


// Globals

// ping/pong callback
void (*adc_dma_ping_fp)(uint8_t *) = NULL;
void (*adc_dma_pong_fp)(uint8_t *) = NULL;


// Sample buffers
uint8_t adc_ping_samples[ADC_CAPTURE_DEPTH];
uint8_t adc_pong_samples[ADC_CAPTURE_DEPTH];

uint adc_dma_ping;
uint adc_dma_pong;
dma_channel_config dma_ping_cfg;
dma_channel_config dma_pong_cfg;


/* ****************************************************** */
static void __isr __time_critical_func(dma_handler)() 
/*
short   : ADC / DMA isr
inputs  : 
outputs :
notes   :
version : DMK - initial version
***********************************************************/
{
    // Check for -ping-
    if( dma_hw->ints0 & (1u << adc_dma_ping)) {
#ifdef ADC_DMA_DEBUG        
        gpio_put(DEBUG_LED_PING, true);
#endif

        // Get ready for new dma cycle
        dma_channel_set_write_addr(adc_dma_ping, adc_ping_samples, false);
        dma_hw->ints0 = 1u << adc_dma_ping;

        // call user
        if( adc_dma_ping_fp != NULL ) {
            adc_dma_ping_fp(adc_ping_samples);
        }
        
#ifdef ADC_DMA_DEBUG        
        gpio_put(DEBUG_LED_PING, false);
#endif
    }
    
    // Check for -pong-
    if( dma_hw->ints0 & (1u << adc_dma_pong)) {
        
#ifdef ADC_DMA_DEBUG        
        gpio_put(DEBUG_LED_PONG, true);
#endif
       
        // Get ready for new dma cycle
        dma_channel_set_write_addr(adc_dma_pong, adc_pong_samples, false);
        dma_hw->ints0 = 1u << adc_dma_pong;
        
        // call user
        if( adc_dma_pong_fp != NULL ) {
            adc_dma_pong_fp(adc_pong_samples);
        }
        
#ifdef ADC_DMA_DEBUG        
        gpio_put(DEBUG_LED_PONG, false);
#endif
    }
}





/* ****************************************************** */
void adc_dma_init()
/*
short   : ADC / DMA init
inputs  : 
outputs :
notes   :
version : DMK - initial version
***********************************************************/
{

#ifdef ADC_DMA_DEBUG        
    gpio_init(DEBUG_LED_PING);
    gpio_set_dir(DEBUG_LED_PING, GPIO_OUT);
    gpio_init(DEBUG_LED_PONG);
    gpio_set_dir(DEBUG_LED_PONG, GPIO_OUT);

    printf("adc_dma_init() debug @GPIO:\n\tping %d\n\tpong %d\n", DEBUG_LED_PING, DEBUG_LED_PONG);
#endif
    
    // init adc
    adc_init();
    adc_gpio_init(26); 
    adc_gpio_init(27);
    adc_set_clkdiv(480.0); //ADC_CLKDIV); // Fs = 40kHz
    hw_clear_bits(&adc_hw->fcs, ADC_FCS_UNDER_BITS);
    hw_clear_bits(&adc_hw->fcs, ADC_FCS_OVER_BITS);
    adc_fifo_setup(
        true,   // Write to fifo
        true,   // DREQ 
        1,      // Trigger DREQ with at least 1 sample
        false,  // No error bit (ERR)
        true    // Shift 12bits -> 8bits
    );
    adc_select_input(0);
    adc_set_round_robin(3);

    // dma adc
    adc_dma_ping = dma_claim_unused_channel(true);
    adc_dma_pong = dma_claim_unused_channel(true);
    dma_ping_cfg = dma_channel_get_default_config(adc_dma_ping);
    dma_pong_cfg = dma_channel_get_default_config(adc_dma_pong);

    channel_config_set_transfer_data_size(&dma_ping_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_ping_cfg, false);
    channel_config_set_write_increment(&dma_ping_cfg, true);
    channel_config_set_dreq(&dma_ping_cfg, DREQ_ADC);
    channel_config_set_chain_to(&dma_ping_cfg, adc_dma_pong);
    dma_channel_configure(
        adc_dma_ping, 
        &dma_ping_cfg, 
        adc_ping_samples, 
        &adc_hw->fifo, 
        ADC_CAPTURE_DEPTH, 
        false);
    dma_channel_set_irq0_enabled(adc_dma_ping, true);

    channel_config_set_transfer_data_size(&dma_pong_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_pong_cfg, false);
    channel_config_set_write_increment(&dma_pong_cfg, true);
    channel_config_set_dreq(&dma_pong_cfg, DREQ_ADC);
    channel_config_set_chain_to(&dma_pong_cfg, adc_dma_ping);
    dma_channel_configure(
        adc_dma_pong, 
        &dma_pong_cfg, 
        adc_pong_samples, 
        &adc_hw->fifo, 
        ADC_CAPTURE_DEPTH, 
        false);
    dma_channel_set_irq0_enabled(adc_dma_pong, true);

    // adc isr
    dma_set_irq0_channel_mask_enabled((1u << adc_dma_ping) | (1u << adc_dma_pong), true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    dma_start_channel_mask(1u << adc_dma_ping);
    adc_run(true);
}

/* ****************************************************** */
void adc_dma_set_ping_cb( void (*f)(uint8_t *) )
/*
short   : adc/dma set callback function
inputs  : 
outputs :
notes   :
version : DMK - initial version
***********************************************************/
{
    if( f != NULL ) {
        adc_dma_ping_fp = f;
    }
}

/* ****************************************************** */
void adc_dma_set_pong_cb( void (*f)(uint8_t *) )
/*
short   : adc/dma set callback function
inputs  : 
outputs :
notes   :
version : DMK - initial version
***********************************************************/
{
    if( f != NULL ) {
        adc_dma_pong_fp = f;
    }
}
