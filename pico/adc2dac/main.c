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

typedef struct {
    uint8_t left[8192];
    uint8_t right[8192];
} audio_buf;

bool ping = false;
bool pong = false;

audio_buf ab;
uint16_t i = 0;

void on_adc_dma_ping(uint8_t *buf) {

//    i = 0;
//    for(uint16_t idx = 0; idx < ADC_CAPTURE_DEPTH; idx+=2 ) {
//        ab.left[i] = buf[idx];
//        ab.right[i] = buf[idx+1];
//        i++;
//    }
    ping = true;
}

void on_adc_dma_pong(uint8_t *buf) {

//    i = 0;    
//    for(uint16_t idx = 0; idx < ADC_CAPTURE_DEPTH; idx+=2 ) {
//        ab.left[i] = buf[idx];
//        ab.right[i] = buf[idx+1];
//        i++;
//    }
    pong = true;
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

    sleep_ms(2000); 
    while (true) {
    
        if(ping) {
//            pwm_audio_write(ab.left, 8192);
//            puts("ping");
//            ping = false;
        }

        if(pong) {
//            pwm_audio_write(ab.left, 8192);
//            puts("pong");
//            pong = false;
        }
    }
}
