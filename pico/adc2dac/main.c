//
// ADC to DAC experiment (digital fir filtering) 
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include "sampler.h"
#include "pwm_audio.h"


void on_adc_dma_ping(uint8_t *buf) {
    pwm_audio_write(buf, 8192);
    puts("ping");
}

void on_adc_dma_pong(uint8_t *buf) {
    pwm_audio_write(buf, 8192);
    puts("pong");
}


int main() {

    stdio_init_all();
    printf("Experiment ADC to DAC with rp2040\n");

    //
    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("frequency_count_khz = %d\n", f_sys_clk);

    // Init pwm audio
    pwm_audio_init();
    pwm_audio_start();

    // Init adc in dma mode
    adc_dma_set_ping_cb(&on_adc_dma_ping);
    adc_dma_set_pong_cb(&on_adc_dma_pong);
    adc_dma_init();

    uint16_t tmp = 0;   
    sleep_ms(2000); 
    while (true) {
    
//            dma_channel_wait_for_finish_blocking(adc_dma_ping);
//            dma_channel_wait_for_finish_blocking(adc_dma_pong);
        printf("%.4X\n", tmp++);
        sleep_ms(1000);
    }
}
