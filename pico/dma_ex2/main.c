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


#define PWM_GPIO_OUT    15 //PICO_DEFAULT_LED_PIN
#define DEBUG_PIN_PING  17
#define DEBUG_PIN_PONG  16
#define BLOCK_SIZE      1024 

#define DMA_DEBUG

uint8_t sample_cnt = 0;
uint16_t ping[BLOCK_SIZE];
uint16_t pong[BLOCK_SIZE];
uint16_t pwm[BLOCK_SIZE * 32];

//void process(uint16_t *p);
void process_pwm(float *p);

#define TAP_NUM 25

float fir_result[BLOCK_SIZE];
uint16_t fir_lastindex = 0;

static float filter_taps[TAP_NUM] = {
  0.008343267095683986,
  0.007149583224306668,
  0.0014492517668356205,
  -0.01331060569409418,
  -0.0345042249128654,
  -0.053675798671946655,
  -0.05866861647110748,
  -0.03857434732652085,
  0.01033141819616749,
  0.08087666209830395,
  0.15569629640166371,
  0.2128725965553248,
  0.23427451265344562,
  0.2128725965553248,
  0.15569629640166371,
  0.08087666209830395,
  0.01033141819616749,
  -0.03857434732652085,
  -0.05866861647110748,
  -0.053675798671946655,
  -0.0345042249128654,
  -0.01331060569409418,
  0.0014492517668356205,
  0.007149583224306668,
  0.008343267095683986
};


void fir(uint16_t *samples, float *fir_result) {
    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        //fir_result[idx] = samples[idx] * filter_taps[fir_lastindex];
        fir_result[idx] = samples[idx] * filter_taps[fir_lastindex];
        fir_lastindex ++;
        fir_lastindex %= TAP_NUM;
    }
}



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
        
        #ifdef DMA_DEBUG        
        gpio_put(DEBUG_PIN_PONG, false);
        #endif
    }

    //
    sample_cnt++;

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
//    for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
//        uint16_t u = (uint16_t) (127.0f + 100.0f * sin( (2.0f * M_PI * 1.0f  * idx) / BLOCK_SIZE)) ;
//        for(uint16_t idy = 0; idy < 32; idy++) {
//            pwm[32*idx+idy] = u;
//        }
//    }

    // PWM 
    gpio_set_function(PWM_GPIO_OUT, GPIO_FUNC_PWM);
    gpio_set_drive_strength(PWM_GPIO_OUT, GPIO_DRIVE_STRENGTH_12MA);
    uint slice_num = pwm_gpio_to_slice_num(PWM_GPIO_OUT);
    pwm_config pc = pwm_get_default_config();
    float div = (f_sys_clk * 1.0f) / (254.0 * 128); 
    printf("div %f\n", div);
    pwm_config_set_clkdiv(&pc, 4.5f); // 
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

        uint64_t start_time = time_us_64();
        dma_channel_wait_for_finish_blocking(dma_adc_ping_channel);
        fir(ping, fir_result);
        process_pwm(fir_result);
        uint64_t end_time = time_us_64();
        printf("PING et: %llu us, cnt: %d)\n", end_time - start_time, sample_cnt);
        
        //start_time = time_us_64();
        dma_channel_wait_for_finish_blocking(dma_adc_pong_channel);
        fir(pong, fir_result);
        process_pwm(fir_result);
        //end_time = time_us_64();
        //printf("\tPONG et: %llu us, buf_index: %d)\n", end_time - start_time, samples_index);
    }
}


void process_pwm(float *p) {
   for(uint16_t idx = 0; idx < BLOCK_SIZE; idx++) {
        //float delta = p[idx+1] - p[idx];
        for(uint16_t idy = 0; idy < 32; idy++) {
            //pwm[32*idx+idy] = (uint16_t)(p[idx] + delta*idx/32);
            pwm[32*idx+idy] = (uint16_t)p[idx];
        }
    }
}



