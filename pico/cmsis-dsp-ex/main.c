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
#define RAW_FSK_OUT_PIN 19
#define ADC_PIN         26
#define DEBUG_PIN_FSK   18

#define CCIR476_RX_LED  3
#define CCIR476_ERR_LED 2

#define DMA_DEBUG
#define FSK_DEBUG

#define SITOR_BIT_US        10000  // Nominal sitor-bit = 10ms (100 baud)
#define SITOR_HALF_BIT_US   SITOR_BIT_US / 2  
#define SITOR_IDLE_US       SITOR_BIT_US * 20

// ADC in and raw_fsk bits out
uint16_t ping[BLOCK_SIZE];
uint16_t pong[BLOCK_SIZE];
uint8_t raw_fsk_ping[BLOCK_SIZE/8];
uint8_t raw_fsk_pong[BLOCK_SIZE/8];

const uint32_t sample_freq = 4000;

volatile int8_t cnt = 0;
volatile bool olev = false;
volatile bool nlev = false;
static absolute_time_t last_irq_time;

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

// CCIR476 callback, also set status to LED
static struct repeating_timer timer_rx_led;
static struct repeating_timer timer_err_led;
bool rx_led_callback(struct repeating_timer *t) { 
    gpio_put(CCIR476_RX_LED, 0);
    return false;
}
bool err_led_callback(struct repeating_timer *t) { 
    gpio_put(CCIR476_ERR_LED, 0);
    return false;
}

void sitorb_char_available(char c) {
    printf("%c", c);
    gpio_put(CCIR476_RX_LED, 1);
    add_repeating_timer_ms(200, rx_led_callback, NULL, &timer_rx_led);
}

void sitorb_err(char *msg) {
    printf("%s", msg);
    gpio_put(CCIR476_ERR_LED, 1);
    add_repeating_timer_ms(1000, err_led_callback, NULL, &timer_err_led);
}


/*******************************************************************/
bool gpio_fsk_callback(struct repeating_timer *t) {
/* 
short:      Callback periodic time to sample raw fsk output. This
            output is used to decode CCIR476 (NAVTEX) messages         
inputs:        
outputs: 
notes:         
Version:    DMK, Initial code
********************************************************************/
    
    bool level = gpio_get(RAW_FSK_OUT_PIN);

    if(level) {
        if( cnt < 3 ) {
            cnt++; 
        } else {
            nlev = true;
        }
    }
    else {
        if( cnt > -3 ) {
            cnt--;
        } else {
            nlev = false;
        }
    }

    //
    if(olev != nlev) {
        
#ifdef FSK_DEBUG        
        gpio_put(DEBUG_PIN_FSK, nlev);
#endif
        // Calc delta
        absolute_time_t now = get_absolute_time();
        absolute_time_t delta = absolute_time_diff_us(last_irq_time, now);
        
        // If IDLE reset fsm else split delta in 10ms (SITOR-B, 100 baud) parts 
        // and pass to ccir476 processor. The ccir476 will sync and convert individual bits
        // to bytes and do the NAVTEX decoding
        if( delta >= SITOR_IDLE_US ) {
#ifdef FSK_DEBUG        
            printf("[Idle]\n");            
#endif
            ccir476_rearm();
        } else {
            uint8_t count = ((delta+SITOR_HALF_BIT_US)/SITOR_BIT_US);           
            for(uint8_t idx = 0; idx < count; idx++) {
                ccir476_process_bit(olev);            
            }
        }
#ifdef FSK_DEBUG
//        printf("%d -> %.6lld\n", nlev, delta);
#endif
        olev = nlev;
        last_irq_time = now;
    }
    
    // return true for next periodic interrupt.
    return true;
}


int main() {

    stdio_init_all();
    printf("DMA experiment with rp2040\n");

#ifdef DMA_DEBUG        
    gpio_init(DEBUG_PIN_PING);
    gpio_set_dir(DEBUG_PIN_PING, GPIO_OUT);
    gpio_init(DEBUG_PIN_PONG);
    gpio_set_dir(DEBUG_PIN_PONG, GPIO_OUT);
    gpio_init(DEBUG_PIN_FSK);
    gpio_set_dir(DEBUG_PIN_FSK, GPIO_OUT);
#endif

    // Inform
    uint32_t f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("System clock [khz] = %ld kHz\n", f_sys_clk);
    uint32_t f_adc_clk = clock_get_hz(clk_adc);
    printf("ADC clock [hz] = %ld Hz\n", f_adc_clk);

    // Init status leds and do a little led dance
    gpio_init(CCIR476_RX_LED);
    gpio_set_dir(CCIR476_RX_LED, GPIO_OUT);
    gpio_init(CCIR476_ERR_LED);
    gpio_set_dir(CCIR476_ERR_LED, GPIO_OUT);
    for(uint8_t idx = 0; idx < 3; idx++ ) {
        gpio_put(CCIR476_RX_LED, 1);
        gpio_put(CCIR476_ERR_LED, 0);
        sleep_ms(100);
        gpio_put(CCIR476_RX_LED, 0);
        gpio_put(CCIR476_ERR_LED, 1);
        sleep_ms(100);
    }
    gpio_put(CCIR476_RX_LED, 1);
    gpio_put(CCIR476_ERR_LED, 1);
    sleep_ms(400);
    gpio_put(CCIR476_RX_LED, 0);
    gpio_put(CCIR476_ERR_LED, 0);

    // fsk sampling repeating timer
    static struct repeating_timer timer_fsk;
    add_repeating_timer_us(-250, gpio_fsk_callback, NULL, &timer_fsk);
    
    // Init decoder
    ccir476_init(&sitorb_msg_available, &sitorb_char_available, &sitorb_err); 

    // PIO - pio0 will output raw fsk decoder data using dma
    // Output rate = 100 baud (bits/s) meaning 1 bit takes 10ms
    // DMA has 1/Fs*BLOCK_SIZE time to generate BLOCK_SIZE bitlevel
    // meaning pio clockdiv = BLOCK_SIZE/FS* 
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &bit_streamer_program);
    uint sm = pio_claim_unused_sm(pio, true);
    bit_streamer_program_init(pio, sm, offset, RAW_FSK_OUT_PIN, sample_freq);
    
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
 
        dma_channel_wait_for_finish_blocking(dma_adc_pong_channel);
        process(pong, raw_fsk_pong, BLOCK_SIZE);
    }
}

