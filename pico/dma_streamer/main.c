//
// PICO bit streamer
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/rand.h"

#include "streamer.pio.h"

#define BLOCK_SIZE 1024

uint8_t data[BLOCK_SIZE];

int dma_chan;

// DMA handler
void dma_handler() {

//    dma_hw->ints0 = 1u << dma_chan; // clear interrupt
    
//    dma_channel_set_read_addr(dma_chan, &, true);
}

int main() {

    stdio_init_all();
    printf("PICO bit streamer\n");

    float f_system_clk = clock_get_hz(clk_sys);
    printf("system_clk = %f Hz\n", f_system_clk);

    // fill data with dummy
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        data[idx] = (uint8_t)get_rand_32();
    }
    data[0] = 0x5A;
    data[BLOCK_SIZE-1] = 0x02;

    // init pio_streamer
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &bit_streamer_program);

    uint sm = pio_claim_unused_sm(pio, true);

    printf("sm: %d\n", sm);
    bit_streamer_program_init(pio, sm, offset, 16, 517e3);


    // Set DMA to tranfer 1024 bits to pio
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan,
        &c,
        &pio0_hw->txf[0],
        NULL,
        BLOCK_SIZE,
        false
    );

    // start DMA tranfer
    dma_channel_set_read_addr(dma_chan, data, true);

    while (true) {
       
//        nco_set_frequency(pio, sm, 419e3);
//        printf("LO = %f Hz\n", 419e3 );
//        sleep_ms(3500);
        
//        nco_set_frequency(pio, sm, 590e3);
//        printf("LO = %f Hz\n", 590e3);
//        sleep_ms(3500);

//        for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
//            uint32_t pio_tx = data[3]<<24 | 
//                data[2]<<16 | 
//                data[1]<<8 | 
//                data[0];
//            uint32_t pio_tx = 0x5A << 24;  
//            pio_sm_put_blocking(pio0, sm, pio_tx);
//            printf("%lX\n", pio_tx);
//        }

        dma_channel_wait_for_finish_blocking(dma_chan);
        dma_channel_set_read_addr(dma_chan, data, true);
        printf(".\n");
        sleep_ms(2000);
//        printf(".\n");

    }
}
