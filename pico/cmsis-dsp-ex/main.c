//
// CMSIS on pico 2 study.
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/adc.h"
#include "hardware/pio.h"

#include "streamer.pio.h"
#include "ccir476.h"
#include "process.h"

int dma_adc_ping_channel;
int dma_adc_pong_channel;
int dma_fsk_ping_channel;
int dma_fsk_pong_channel;
dma_channel_config adc_ping_config;
dma_channel_config adc_pong_config;
dma_channel_config dma_fsk_ping_config;
dma_channel_config dma_fsk_pong_config;

#define DEBUG_PIN_PING  16
#define DEBUG_PIN_PONG  17
#define RAW_FSK_OUT     19
#define ADC_PIN         26
#define SITOR_PIN       18

#define DMA_DEBUG

// ADC in and raw_fsk bits out
uint16_t ping[BLOCK_SIZE];
uint16_t pong[BLOCK_SIZE];
uint8_t raw_fsk_ping[BLOCK_SIZE/8];
uint8_t raw_fsk_pong[BLOCK_SIZE/8];

const uint32_t sample_freq = 4000;


// ISR of ADC DMA
static void __isr __time_critical_func(dma_handler)() {

    if( dma_hw->ints0 & (1u << dma_adc_ping_channel) ) {

        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PING, true);
        #endif
        
        // adc
        dma_channel_set_write_addr(dma_adc_ping_channel, ping, false);
        dma_channel_set_read_addr(dma_fsk_pong_channel, raw_fsk_pong, true);
        // restart isr_adc
        dma_hw->ints0 = 1u << dma_adc_ping_channel;
        
        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PING, false);
        #endif
    }
    
    if( dma_hw->ints0 & (1u << dma_adc_pong_channel) ) {

        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PONG, true);
        #endif
        
        // adc
        dma_channel_set_write_addr(dma_adc_pong_channel, pong, false);
        dma_channel_set_read_addr(dma_fsk_ping_channel, raw_fsk_ping, true);
        // restart isr_adc
        dma_hw->ints0 = 1u << dma_adc_pong_channel;
        
        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PONG, false);
        #endif
    }
}


// callback from navtex decoder
void sitorb_msg_available(char *msg) {
    printf("%s", msg);
}

void sitorb_char_available(char c) {
    printf("%c", c);
}

void sitorb_err(char *msg) {
    printf("%s", msg);
}


#define HEARTBEAT_US      1000000
#define SBIT_L_US          9*1000 // 9ms <= validbit <= 11ms
#define SBIT_H_US         11*1000  
#define SIDLE_US          SBIT_L_US * 40
#define SGLITCH_US        4 * 2000


/*
 * Capture and handle FSK bits and glitched. Pass to ccir476 decoder
 *
 */
void gpio_callback(uint gpio, uint32_t events) {

    static absolute_time_t last_irq_time;
    absolute_time_t now = get_absolute_time();
    absolute_time_t delta =  absolute_time_diff_us(last_irq_time, now);
    uint8_t count, bit;

    if( absolute_time_diff_us(last_irq_time, now) <= SGLITCH_US ) {
        printf("[*]");            
    } else {
        if( delta >= SIDLE_US ) {
            printf("[Idle detected]\n");            
            // ccir476_rearm();
        } else {
        
            //
            // Bepaal aantal ontvangen 1 of 0 achterelkaar en level
            //
            count = ((delta+5000)/10000);           
            //bit;
            
            if(events & GPIO_IRQ_EDGE_RISE) {
                bit = 0;
            } else {
                if(events & GPIO_IRQ_EDGE_FALL) {
                    bit = 1;
                } else {
                    printf("bitlevel not 0 or 1, huh??\n");
                }
            }
            
            // Handle all individual bits incl. synchronize
            //for(uint8_t idx = 0; idx < count; idx++) {
            //    ccir476_process_bit(bit);            
            //}
        }
    }

    printf("%lld:%d:%d\n", delta, count, bit);

    last_irq_time = now;
}


int main() {

    stdio_init_all();
    printf("DMA experiment with rp2040\n");

#ifdef DMA_DEBUG        
    gpio_init(DEBUG_PIN_PING);
    gpio_set_dir(DEBUG_PIN_PING, GPIO_OUT);
    gpio_init(DEBUG_PIN_PONG);
    gpio_set_dir(DEBUG_PIN_PONG, GPIO_OUT);
#endif

    // Inform
    uint32_t f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("System clock [khz] = %ld kHz\n", f_sys_clk);
    uint32_t f_adc_clk = clock_get_hz(clk_adc);
    printf("ADC clock [hz] = %ld Hz\n", f_adc_clk);

    // SITOR decoder
//    gpio_init(SITOR_PIN);
//    gpio_set_dir(SITOR_PIN, GPIO_IN);
//    gpio_pull_up(SITOR_PIN);
    gpio_set_irq_enabled_with_callback(
        RAW_FSK_OUT, //SITOR_PIN,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &gpio_callback
    );
    // Init decoder
    ccir476_init(&sitorb_msg_available, &sitorb_char_available, &sitorb_err); 

    // PIO - pio0 will output raw fsk decoder data using dma
    // Output rate = 100 baud (bits/s) meaning 1 bit takes 10ms
    // DMA has 1/Fs*BLOCK_SIZE time to generate BLOCK_SIZE bitlevel
    // meaning pio clockdiv = BLOCK_SIZE/FS* 
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &bit_streamer_program);
    uint sm = pio_claim_unused_sm(pio, true);
    bit_streamer_program_init(pio, sm, offset, RAW_FSK_OUT, sample_freq);
    
    // DMA output raw fsk out ping
    dma_fsk_ping_channel = dma_claim_unused_channel(true);
    dma_fsk_ping_config = dma_channel_get_default_config(dma_fsk_ping_channel);
    channel_config_set_transfer_data_size(&dma_fsk_ping_config, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_fsk_ping_config, true);
    channel_config_set_write_increment(&dma_fsk_ping_config, false);
    channel_config_set_dreq(&dma_fsk_ping_config, DREQ_PIO0_TX0);
    dma_channel_configure(
        dma_fsk_ping_channel,
        &dma_fsk_ping_config,
        &pio0_hw->txf[0],
        NULL,
        BLOCK_SIZE/8,
        false
    );
    
    // DMA output raw fsk out pong
    dma_fsk_pong_channel = dma_claim_unused_channel(true);
    dma_fsk_pong_config = dma_channel_get_default_config(dma_fsk_pong_channel);
    channel_config_set_transfer_data_size(&dma_fsk_pong_config, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_fsk_pong_config, true);
    channel_config_set_write_increment(&dma_fsk_pong_config, false);
    channel_config_set_dreq(&dma_fsk_pong_config, DREQ_PIO0_TX0);
    dma_channel_configure(
        dma_fsk_pong_channel,
        &dma_fsk_pong_config,
        &pio0_hw->txf[0],
        NULL,
        BLOCK_SIZE/8,
        false
    );

    // ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_clkdiv(f_adc_clk / sample_freq); 
    hw_clear_bits(&adc_hw->fcs, ADC_FCS_UNDER_BITS);
    hw_clear_bits(&adc_hw->fcs, ADC_FCS_OVER_BITS);
    adc_fifo_setup(
        true,
        true,
        1,
        false,
        true
    );
    adc_select_input(0);

    dma_adc_ping_channel = dma_claim_unused_channel(true);
    dma_adc_pong_channel = dma_claim_unused_channel(true);
    adc_ping_config = dma_channel_get_default_config(dma_adc_ping_channel);
    adc_pong_config = dma_channel_get_default_config(dma_adc_pong_channel);

    channel_config_set_transfer_data_size(&adc_ping_config, DMA_SIZE_16);
    channel_config_set_read_increment(&adc_ping_config, false);
    channel_config_set_write_increment(&adc_ping_config, true);
    channel_config_set_dreq(&adc_ping_config, DREQ_ADC);
    channel_config_set_chain_to(&adc_ping_config, dma_adc_pong_channel);
    dma_channel_configure(
        dma_adc_ping_channel,
        &adc_ping_config,
        ping,
        &adc_hw->fifo,
        BLOCK_SIZE,
        false 
    );
    dma_channel_set_irq0_enabled(dma_adc_ping_channel, true);
    
    channel_config_set_transfer_data_size(&adc_pong_config, DMA_SIZE_16);
    channel_config_set_read_increment(&adc_pong_config, false);
    channel_config_set_write_increment(&adc_pong_config, true);
    channel_config_set_dreq(&adc_pong_config, DREQ_ADC);
    channel_config_set_chain_to(&adc_pong_config, dma_adc_ping_channel);
    dma_channel_configure(
        dma_adc_pong_channel,
        &adc_pong_config,
        pong,
        &adc_hw->fifo,
        BLOCK_SIZE,
        false 
    );
    dma_channel_set_irq0_enabled(dma_adc_pong_channel, true);
    
    // IRQ
    dma_set_irq0_channel_mask_enabled((1u<<dma_adc_ping_channel) | (1u << dma_adc_pong_channel), true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Let's rock & roll
    dma_start_channel_mask(1u << dma_adc_ping_channel);
    adc_run(true);

    // Init fir filter
    process_init(); 

    while (true) {

        dma_channel_wait_for_finish_blocking(dma_adc_ping_channel);
        process(ping, raw_fsk_ping, BLOCK_SIZE);
/*        for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
            if( raw_bits[idx] == 1 ) {
                gpio_put(RAW_FSK_OUT, true);
            } else {
                gpio_put(RAW_FSK_OUT, false);
            }
            sleep_us(200);
        }
*/

 
        dma_channel_wait_for_finish_blocking(dma_adc_pong_channel);
        process(pong, raw_fsk_pong, BLOCK_SIZE);
/*        for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
            if( raw_bits[idx] == 1 ) {
                gpio_put(RAW_FSK_OUT, true);
            } else {
                gpio_put(RAW_FSK_OUT, false);
            }
            sleep_us(200);
        }
*/
    }
}

