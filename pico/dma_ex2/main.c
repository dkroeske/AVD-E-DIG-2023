//
// DMA exploration
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
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "math.h"

//int dma_pwm_channel;
int dma_adc_ping_channel;
int dma_adc_pong_channel;
int dma_pwm_channel;
dma_channel_config adc_ping_config;
dma_channel_config adc_pong_config;
dma_channel_config dma_pwm_config;


#define PWM_GPIO_OUT    19 //PICO_DEFAULT_LED_PIN
#define DEBUG_PIN_PING  17
#define DEBUG_PIN_PONG  16
#define BLOCK_SIZE      254 

#define DMA_DEBUG

uint8_t samples_index;
uint16_t ping[BLOCK_SIZE];
uint16_t pong[BLOCK_SIZE];
uint16_t pwm[BLOCK_SIZE * 32];

void process(uint16_t *p);

static void __isr __time_critical_func(dma_handler)() {

    if( dma_hw->ints0 & (1u << dma_adc_ping_channel) ) {

        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PING, true);
        #endif
        
        // adc
        dma_channel_set_write_addr(dma_adc_ping_channel, ping, false);
        dma_channel_set_read_addr(dma_pwm_channel, pwm, true);
        // restart isr_adc
        dma_hw->ints0 = 1u << dma_adc_ping_channel;
        
        samples_index++;
        samples_index%=4;

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
        dma_channel_set_read_addr(dma_pwm_channel, pwm, true);
        // restart isr_adc
        dma_hw->ints0 = 1u << dma_adc_pong_channel;
        
        samples_index++;
        samples_index%=4;

        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PONG, false);
        #endif
    }

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

    // 
    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("frequency_count_khz = %d\n", f_sys_clk);

    //
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        uint16_t u = (uint16_t) (127.0f + 100.0f * sin( (2.0f * M_PI * 1.0f  * idx) / BLOCK_SIZE)) ;
        for(uint16_t idy = 0; idy < 32; idy++) {
            pwm[32*idx+idy] = u;
        }
    }

    // PWM 
    gpio_set_function(PWM_GPIO_OUT, GPIO_FUNC_PWM);
    gpio_set_drive_strength(PWM_GPIO_OUT, GPIO_DRIVE_STRENGTH_12MA);
    uint slice_num = pwm_gpio_to_slice_num(PWM_GPIO_OUT);
    pwm_config pc = pwm_get_default_config();
    float div = (f_sys_clk * 1.0f) / (254.0 * 260); 
    pwm_config_set_clkdiv(&pc, div); // 
    pwm_config_set_wrap(&pc,  254);
    pwm_init(slice_num, &pc, true);

    dma_pwm_channel = dma_claim_unused_channel(true);
    dma_pwm_config = dma_channel_get_default_config(dma_pwm_channel);
    channel_config_set_transfer_data_size(&dma_pwm_config, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_pwm_config, true);
    channel_config_set_write_increment(&dma_pwm_config, false);
    channel_config_set_dreq(&dma_pwm_config, DREQ_PWM_WRAP0 + slice_num);
//    channel_config_set_chain_to(&pwm_config, ...);
    dma_channel_configure(
        dma_pwm_channel,
        &dma_pwm_config,
        &pwm_hw->slice[slice_num].cc,
        pwm,
        BLOCK_SIZE * 32,
        true
    );
//    dma_channel_set_irq0_enabled(dma_pwm_channel, false);

    // ADC
    adc_init();
    adc_gpio_init(26);
    adc_set_clkdiv(12000.0); // 4kHz
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

    dma_start_channel_mask(1u << dma_adc_ping_channel);
    adc_run(true);

    while (true) {

        //uint64_t start_time = time_us_64();
        dma_channel_wait_for_finish_blocking(dma_adc_ping_channel);
        process(ping);
        //uint64_t end_time = time_us_64();
        //printf("PING et: %llu us, buf_index: %d)\n", end_time - start_time, samples_index);
        
        //start_time = time_us_64();
        dma_channel_wait_for_finish_blocking(dma_adc_pong_channel);
        process(pong);
        //end_time = time_us_64();
        //printf("\tPONG et: %llu us, buf_index: %d)\n", end_time - start_time, samples_index);
    }
}


void process(uint16_t *p) {
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
//        if(idx < 8) printf("%.2d ", p[idx]); 
        for(uint16_t idy = 0; idy < 32; idy++) {
//            pwm[32*idx+idy] = p[idx];
        }
    }
}

