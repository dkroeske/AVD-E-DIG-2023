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

// Debug 
#define DEBUG_LED       16

// PWM
#define PWM_GPIO_OUT    19 
#define PWM_FREQ_KHZ    500
#define PWM_CLK_MHZ     125

// ADC
#define F_ADC           48000000
#define FS_ADC            100000
#define ADC_CLKDIV  F_ADC/FS_ADC
#define ADC_CAPTURE_DEPTH 64 

#define FS              220000 

#define f1              1000
#define f2              2000
#define SIN_TABLE_SIZE  512
uint16_t sin_table[ADC_CAPTURE_DEPTH];

void calc_sin_table() {
    for(uint16_t n = 0; n < ADC_CAPTURE_DEPTH; n++) {
        sin_table[n] = (uint8_t) (126.0 + 
            50.0 * sin(2.0 * 3.1415 * f1 * n / FS) + 
            0.0 * sin(2.0 * 3.1415 * f2 * n / FS));
    }
/*    for(uint16_t n = 0; n < 441; n++) {
        printf("%d, %d\n", n, sin_table[n]);
    }
*/
}

uint16_t adc_ping_samples[ADC_CAPTURE_DEPTH];
uint16_t adc_pong_samples[ADC_CAPTURE_DEPTH];

uint adc_dma_ping;
uint adc_dma_pong;
dma_channel_config dma_ping_cfg;
dma_channel_config dma_pong_cfg;

// audio out
uint16_t pwm_ping[ADC_CAPTURE_DEPTH];
uint16_t pwm_pong[ADC_CAPTURE_DEPTH];
uint pwm_dma_ping;
uint pwm_dma_pong;
dma_channel_config pwm_ping_cfg;
dma_channel_config pwm_pong_cfg;
int pwm_slice_num;


//static void __isr __time_critical_func(dma_handler)() {
void dma_handler() {
    

    if( dma_hw->ints0 & (1u << adc_dma_ping)) {
        dma_channel_configure(
            adc_dma_ping, 
            &dma_ping_cfg, 
            adc_ping_samples, 
            &adc_hw->fifo,
            ADC_CAPTURE_DEPTH,
            false);
    
        dma_channel_configure(
            pwm_dma_pong, &pwm_pong_cfg,
            &pwm_hw->slice[pwm_slice_num].cc,
            adc_ping_samples, 
            ADC_CAPTURE_DEPTH,
            true
        );


        dma_hw->ints0 = 1u << adc_dma_ping;
        
        gpio_put(DEBUG_LED, true);
        sleep_us(3);
        gpio_put(DEBUG_LED, false);
    }
    
    if( dma_hw->ints0 & (1u << adc_dma_pong)) {
        dma_channel_configure(
            adc_dma_pong, 
            &dma_pong_cfg, 
            adc_pong_samples, 
            &adc_hw->fifo,
            ADC_CAPTURE_DEPTH,
            false);
    
        dma_channel_configure(
            pwm_dma_ping, &pwm_ping_cfg,
            &pwm_hw->slice[pwm_slice_num].cc,
            adc_pong_samples, 
            ADC_CAPTURE_DEPTH,
            true
        );

        dma_hw->ints0 = 1u << adc_dma_pong;
    }
}

int main() {

    stdio_init_all();
    printf("Experiment ADC to DAC with rp2040\n");

    // Precalc signals 
    calc_sin_table();

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
    adc_set_clkdiv(480.0); //ADC_CLKDIV); // Fs = 40kHz
//    hw_clear_bits(&adc_hw->fcs, ADC_FCS_UNDER_BITS);
//    hw_clear_bits(&adc_hw->fcs, ADC_FCS_OVER_BITS);
    adc_fifo_setup(
        true,   // Write to fifo
        true,  
        1, 
        false, 
        false    //
    );
    adc_select_input(1);
//    adc_set_round_robin(3);

    
    // pwm
    gpio_set_function(PWM_GPIO_OUT, GPIO_FUNC_PWM);
    gpio_set_drive_strength(PWM_GPIO_OUT, GPIO_DRIVE_STRENGTH_12MA);
    pwm_slice_num = pwm_gpio_to_slice_num(PWM_GPIO_OUT);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f);
    pwm_config_set_wrap(&config, 255);
    pwm_init(pwm_slice_num, &config, true);
    

    // dma adc
    adc_dma_ping = dma_claim_unused_channel(true);
    adc_dma_pong = dma_claim_unused_channel(true);
    dma_ping_cfg = dma_channel_get_default_config(adc_dma_ping);
    dma_pong_cfg = dma_channel_get_default_config(adc_dma_pong);

    channel_config_set_transfer_data_size(&dma_ping_cfg, DMA_SIZE_16);
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

    channel_config_set_transfer_data_size(&dma_pong_cfg, DMA_SIZE_16);
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

    // dma pwm
    pwm_dma_ping = dma_claim_unused_channel(true);
    pwm_dma_pong = dma_claim_unused_channel(true);
    pwm_ping_cfg = dma_channel_get_default_config(pwm_dma_ping);
    pwm_pong_cfg = dma_channel_get_default_config(pwm_dma_pong);
    
    channel_config_set_transfer_data_size(&pwm_ping_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&pwm_ping_cfg, true);
    channel_config_set_write_increment(&pwm_ping_cfg, false);
    channel_config_set_dreq(&pwm_ping_cfg, DREQ_PWM_WRAP0 + pwm_slice_num);
    
    channel_config_set_transfer_data_size(&pwm_pong_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&pwm_pong_cfg, true);
    channel_config_set_write_increment(&pwm_pong_cfg, false);
    channel_config_set_dreq(&pwm_pong_cfg, DREQ_PWM_WRAP0 + pwm_slice_num);

    // adc isr
    dma_set_irq0_channel_mask_enabled((1u << adc_dma_ping) | (1u << adc_dma_pong), true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    dma_start_channel_mask(1u << adc_dma_ping);
    adc_run(true);


    uint16_t tmp = 0;    
    while (true) {
//        dma_channel_wait_for_finish_blocking(adc_dma_ping);
        // Print ping buffer

        sleep_ms(2000);

        while(1) {
            printf("(%.2d)\n", tmp++);
            sleep_ms(1000);            
        }
    }
}
