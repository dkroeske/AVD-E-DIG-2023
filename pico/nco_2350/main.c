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

#include "nco.pio.h"

// ADC
#define CAPTURE_CHANNEL  0
#define CAPTURE_DEPTH 1000
uint8_t capture_buf[CAPTURE_DEPTH];

int main() {

    stdio_init_all();
    printf("PICO iq_nco\n");

    // init nco
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &nco_program);

    uint sm = pio_claim_unused_sm(pio, true);
    nco_program_init(pio, sm, offset, 11, 12, 600e3);

    nco_set_frequency(pio, sm, 600e3);
    printf("IQ clock = %f Hz\n", 600e3 );
    sleep_ms(1000);

    // ADC    
    adc_gpio_init(26+CAPTURE_CHANNEL);
    adc_init();
    adc_select_input(CAPTURE_CHANNEL);
    adc_fifo_setup(
        true,
        true,
        1,
        false,
        true
    );
    adc_set_clkdiv(0);
    sleep_ms(1000);

    // DMA
    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_cfg, false);
    channel_config_set_write_increment(&dma_cfg, true);

    channel_config_set_dreq(&dma_cfg, DREQ_ADC);

    dma_channel_configure(
        dma_chan,
        &dma_cfg,
        capture_buf,
        &adc_hw->fifo,
        CAPTURE_DEPTH,
        true
    );

    adc_run(true);

    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
    adc_fifo_drain();

    for(int idx = 0; idx < CAPTURE_DEPTH; ++idx) {
        printf("%-3d, ", capture_buf[idx]);
        if(idx % 10 == 9) {
            printf("\n");
        }
    }
    
    while (true) {
       
//        nco_set_frequency(pio, sm, 419e3);
//        printf("IQ clock = %f Hz\n", 419e3 );
        printf(".");
        sleep_ms(3500);
        
//        nco_set_frequency(pio, sm, 590e3);
//        printf("IQ clock = %f Hz\n", 590e3);
        printf("+");
        sleep_ms(3500);
    }
}
