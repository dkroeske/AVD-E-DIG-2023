//
// ADC to DAC experiment (digital fir filtering) 
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
//#include "hardware/clocks.h"
//#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include "math.h"

// ADC
//#define CAPTURE_CHANNEL  0
//#define CAPTURE_DEPTH 1000
//uint8_t capture_buf[CAPTURE_DEPTH];

// Debug 
#define DEBUG_LED       16

// PWM
#define PWM_GPIO_OUT    19 
#define PWM_FREQ_KHZ    500
#define PWM_CLK_MHZ     125

// ADC
#define F_ADC           48000000
#define FS_ADC            100000
#define ADC_CLKDIV      F_ADC/FS_ADC



#define FS              220000 

#define f1              1000
#define f2              2000
#define SIN_TABLE_SIZE  512
uint8_t sin_table[SIN_TABLE_SIZE];

void calc_sin_table() {
    for(uint16_t n = 0; n < 441; n++) {
        sin_table[n] = (uint8_t) (126.0 + 
            50.0 * sin(2.0 * 3.1415 * f1 * n / FS) + 
            50.0 * sin(2.0 * 3.1415 * f2 * n / FS));
    }
/*    for(uint16_t n = 0; n < 441; n++) {
        printf("%d, %d\n", n, sin_table[n]);
    }
*/
}

#define ADC_CAPTURE_DEPTH 10000
uint8_t adc_ping_samples[ADC_CAPTURE_DEPTH];
uint8_t adc_pong_samples[ADC_CAPTURE_DEPTH];

uint8_t adc_debug_ping[ADC_CAPTURE_DEPTH];
uint8_t adc_debug_pong[ADC_CAPTURE_DEPTH];
bool first_ping = true;
bool first_pong = true;


uint adc_dma_ping;
uint adc_dma_pong;
dma_channel_config dma_ping_cfg;
dma_channel_config dma_pong_cfg;

//static void __isr __time_critical_func(dma_handler)() {
void dma_handler() {
    
    gpio_put(DEBUG_LED, true);

    if( dma_hw->ints0 & (1u << adc_dma_ping)) {
        dma_channel_configure(
            adc_dma_ping, 
            &dma_ping_cfg, 
            adc_ping_samples, 
            &adc_hw->fifo,
            ADC_CAPTURE_DEPTH,
            false);
    
        if( first_ping ) {
            memcpy(adc_debug_ping, adc_ping_samples, ADC_CAPTURE_DEPTH);
            first_ping = false;
        }

        dma_hw->ints0 = 1u << adc_dma_ping;
    }
    
    if( dma_hw->ints0 & (1u << adc_dma_pong)) {
        dma_channel_configure(
            adc_dma_pong, 
            &dma_pong_cfg, 
            adc_pong_samples, 
            &adc_hw->fifo,
            ADC_CAPTURE_DEPTH,
            false);
    
        if( first_pong ) {
            memcpy(adc_debug_pong, adc_pong_samples, ADC_CAPTURE_DEPTH);
            first_pong = false;
        }
    
        dma_hw->ints0 = 1u << adc_dma_pong;
    }

    sleep_us(2);
    gpio_put(DEBUG_LED, false);
}

int main() {

    stdio_init_all();
    printf("Experiment ADC to DAC with rp2040\n");

    // Precalc signals 
    // calc_sin_table();

    //
    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("frequency_count_khz = %d\n", f_sys_clk);

    // debug
    gpio_init(DEBUG_LED);
    gpio_set_dir(DEBUG_LED, GPIO_OUT);
    
    // adc
    adc_init();
    adc_gpio_init(26); 
    adc_gpio_init(27);
    adc_set_clkdiv(ADC_CLKDIV); // Fs = 40kHz
//    hw_clear_bits(&adc_hw->fcs, ADC_FCS_UNDER_BITS);
//    hw_clear_bits(&adc_hw->fcs, ADC_FCS_OVER_BITS);
    adc_fifo_setup(
        true,   // Write to fifo
        true,  
        1, 
        false, 
        true    //
    );
    adc_select_input(0);
    adc_set_round_robin(3);

    /*
    // pwm
    gpio_set_function(PWM_GPIO_OUT, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_GPIO_OUT);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clk_div);
    pwm_config_set_wrap(&config, 254);
    */

    // dma
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

    dma_set_irq0_channel_mask_enabled((1u << adc_dma_ping) | (1u << adc_dma_pong), true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    dma_start_channel_mask(1u << adc_dma_ping);
    adc_run(true);


    /*
    // DMA
    pwm_channel = dma_claim_unused_channel(true);
    dma_channel_config dma_config  = dma_channel_get_default_config(pwm_channel);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
    channel_config_set_dreq(&dma_config, DREQ_PWM_WRAP0);
    dma_channel_configure(
        pwm_channel,
        &dma_config,
        &pwm_hw->slice[slice_num].cc,
        sin_table,
        440,
        false 
    );

    dma_channel_set_irq0_enabled(pwm_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    // Start DMA
    dma_start_channel_mask(1u << pwm_channel);
    */

    uint16_t tmp = 0;    
    while (true) {
//        dma_channel_wait_for_finish_blocking(adc_dma_ping);
        // Print ping buffer

        sleep_ms(2000);

        printf("\nping:\n");
        for(uint16_t idx = 0; idx < ADC_CAPTURE_DEPTH; idx++) {
            printf("%d,", adc_debug_ping[idx]);
            if(idx % 16 == 15) {
                printf("\n");
            }
        }
        
        printf("\npong:\n");
        for(uint16_t idx = 0; idx < ADC_CAPTURE_DEPTH; idx++) {
            printf("%d,", adc_debug_pong[idx]);
            if(idx % 16 == 15) {
                printf("\n");
            }
        }
    
    
        while(1) {

            printf("(%.2d)\n", tmp++);
            sleep_ms(1000);            
        }
    }
}
