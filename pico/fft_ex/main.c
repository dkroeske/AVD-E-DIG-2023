//
// PICO rx
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "math.h"

#include "nco.pio.h"

//
#define DEBUG_IO 13 

// ADC
#define CAPTURE_CHANNEL  0
#define CAPTURE_DEPTH 1024

uint16_t capture_buffer_I[CAPTURE_DEPTH];
uint16_t capture_buffer_Q[CAPTURE_DEPTH];

//#define ADCCLK  48000000.0 // ADC clock 48 Mhz for PICO
//#define Fs          1000.0 // Sample frequency

uint adc_dma_chan_I, adc_dma_chan_Q;
dma_channel_config dma_chan_I_cfg, dma_chan_Q_cfg; 


void dma_handler(uint8_t *buffer, int id){
}

/*
void _dma_handler() {
    
    if(dma_hw->ints0 & (1u << adc_dma_chan_I)) {
        dma_channel_configure(
            adc_dma_chan_I, 
            &dma_chan_I_cfg, 
            capture_buffer_I,
            &adc_hw->fifo, 
            CAPTURE_DEPTH, 
            false
        );
        dma_hw->ints0 = 1u << adc_dma_chan_I;
    }
    
    if(dma_hw->ints0 & (1u << adc_dma_chan_Q)) {
        dma_channel_configure(
            adc_dma_chan_Q, 
            &dma_chan_Q_cfg, 
            capture_buffer_Q,
            &adc_hw->fifo, 
            CAPTURE_DEPTH, 
            false
        );
        dma_hw->ints0 = 1u << adc_dma_chan_Q;
    }
    
    printf("DMA IRQ \n");
}
*/

void dma_handler_I() {
    dma_handler((uint8_t*)&capture_buffer_I, 0);
    dma_hw->ints0 = 1u << adc_dma_chan_I;
    dma_channel_set_write_addr(adc_dma_chan_I, &capture_buffer_I, false);
}

void dma_handler_Q() {
    dma_handler((uint8_t*)&capture_buffer_Q, 1);
    dma_hw->ints1 = 1u << adc_dma_chan_Q;
    dma_channel_set_write_addr(adc_dma_chan_Q, &capture_buffer_Q, false);
}

int main() {

    stdio_init_all();
    printf("PICO iq_nco\n");

    // Debug GPIO
    gpio_init(DEBUG_IO);
    gpio_set_dir(DEBUG_IO, GPIO_OUT);

    // PIO - init nco
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &nco_program);

    uint sm = pio_claim_unused_sm(pio, true);
    nco_program_init(pio, sm, offset, 11, 12, 600e3);

    nco_set_frequency(pio, sm, 600e3);
    printf("IQ clock = %f Hz\n", 600e3 );
    sleep_ms(1000);

    // ADC    
    adc_gpio_init(26 + CAPTURE_CHANNEL);
    adc_gpio_init(27 + CAPTURE_CHANNEL);
    adc_init();
    adc_select_input(CAPTURE_CHANNEL);
    adc_set_round_robin(0b0011);
    adc_fifo_setup(
        true,   // Write to fifo
        true,   // enable DMA data request (DREQ)
        1,      // Trigger DREQ with at least one sample
        false,  // No ERR bits
        true    // Shift 8 bits
    );
    adc_set_clkdiv(64000);
    sleep_ms(100);
    printf("Status: ADC setup completed\n");


    // DMA
    adc_dma_chan_I = dma_claim_unused_channel(true);
    adc_dma_chan_Q = dma_claim_unused_channel(true);

    dma_chan_I_cfg = dma_channel_get_default_config(adc_dma_chan_I);
    dma_chan_Q_cfg = dma_channel_get_default_config(adc_dma_chan_Q);
    
    channel_config_set_transfer_data_size(&dma_chan_I_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_chan_I_cfg, false);
    channel_config_set_write_increment(&dma_chan_I_cfg, true);
    channel_config_set_dreq(&dma_chan_I_cfg, DREQ_ADC);
    channel_config_set_chain_to(&dma_chan_I_cfg, adc_dma_chan_Q);

    channel_config_set_transfer_data_size(&dma_chan_Q_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_chan_Q_cfg, false);
    channel_config_set_write_increment(&dma_chan_Q_cfg, true);
    channel_config_set_dreq(&dma_chan_Q_cfg, DREQ_ADC);
    channel_config_set_chain_to(&dma_chan_Q_cfg, adc_dma_chan_I);

    // Config dma_sample_channel
    dma_channel_configure(
        adc_dma_chan_I,
        &dma_chan_I_cfg,
        capture_buffer_I,
        &adc_hw->fifo,
        CAPTURE_DEPTH,
        false 
    );
    
    // Config dma_sample_channel
    dma_channel_configure(
        adc_dma_chan_Q,
        &dma_chan_Q_cfg,
        capture_buffer_Q,
        &adc_hw->fifo,
        CAPTURE_DEPTH,
        false
    );

   // dma_set_irq0_channel_mask_enabled((1u<<adc_dma_chan_I)|(1u<<adc_dma_chan_Q), true);
    
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler_I);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_handler_Q);
    
    dma_channel_set_irq0_enabled(adc_dma_chan_I, true);
    dma_channel_set_irq1_enabled(adc_dma_chan_Q, true);
    
    irq_set_enabled(DMA_IRQ_0, true);
    irq_set_enabled(DMA_IRQ_1, true);
    
    dma_start_channel_mask(1u << adc_dma_chan_I);
    printf("Status: DMA setup completed\n");
    sleep_ms(100);

    adc_run(true);
    
    printf("Status: ADC running. Ping pong sampling between channel I and Q\n");
    sleep_ms(100);

/*
    for(int idx = 0; idx < CAPTURE_DEPTH; ++idx) {
        printf("%-3d, ", capture_buf[idx]);
        if(idx % 10 == 9) {
            printf("\n");
        }
    }
*/
//    fix15 max_fr;
//    int max_fr_dex;    
    
    while (true) {
        
//        tight_loop_contents();
//        uint32_t start_time = time_us_32();
        dma_channel_wait_for_finish_blocking(adc_dma_chan_I);
        printf("I: ");
        for(uint8_t idx = 0; idx < 20; idx++) {
            printf("%.4X ", capture_buffer_I[idx]);
        }
        printf("\n");
        
        dma_channel_wait_for_finish_blocking(adc_dma_chan_Q);
        printf("Q: ");
        for(uint8_t idx = 0; idx < 20; idx++) {
            printf("%.4X ", capture_buffer_Q[CAPTURE_DEPTH-20+idx]);
        }
        printf("\n");
//        uint32_t busy_time = time_us_32() - start_time;
//        printf("Sampling IQ buffers (%d bytes): %ld [ms]\n", CAPTURE_DEPTH, busy_time/1000);
        
//        nco_set_frequency(pio, sm, 419e3);
//        printf("IQ clock = %f Hz\n", 419e3 );
//        printf(".");
//        sleep_ms(3500);
        
//        nco_set_frequency(pio, sm, 590e3);
//        printf("IQ clock = %f Hz\n", 590e3);
//        printf("+");
//        sleep_ms(3500);
    }
}
