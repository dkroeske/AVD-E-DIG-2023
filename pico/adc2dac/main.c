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
//#include "hardware/dma.h"
//#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "math.h"

// ADC
//#define CAPTURE_CHANNEL  0
//#define CAPTURE_DEPTH 1000
//uint8_t capture_buf[CAPTURE_DEPTH];

// PWM
#define PWM_GPIO_OUT    19 
#define PWM_FREQ_KHZ    200
#define PWM_CLK_MHZ     125


#define FS              10000
#define SIN_TABLE_SIZE  100
float sin_table[SIN_TABLE_SIZE];

void calc_sin_table() {
    for( uint16_t idx = 0; idx < SIN_TABLE_SIZE; idx++) {
        sin_table[idx] = sin(2.0 * 3.1415 * ((float)idx)/SIN_TABLE_SIZE);
    }
}


void pwm_set_dc(uint slice, float dc){
    uint16_t pwm_value = ((PWM_CLK_MHZ*1000*dc)/(PWM_FREQ_KHZ*100));
    pwm_set_chan_level(slice, PWM_GPIO_OUT, pwm_value);
//    printf("pwm value: %d (%.1f)\n", pwm_value, dc);
}

int main() {

    stdio_init_all();
    printf("Experiment ADC to DAC\n");

    // pwm
    gpio_set_function(PWM_GPIO_OUT, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PWM_GPIO_OUT);
    uint channel = pwm_gpio_to_channel(PWM_GPIO_OUT);
    pwm_set_wrap(slice, (PWM_CLK_MHZ*1000/PWM_FREQ_KHZ)); // No prescaler: fclk=125MHz, counter = fclk/fdc
    //pwm_set_chan_level(slice, PWM_OUT, 875); // 21% -> 4167/100 * 21 = 875.07
    pwm_set_dc(slice, 0.0); // 21% -> 4167/100 * 21 = 875.07
    pwm_set_enabled(slice, true);
    printf("PWM is running with slice: %d and channel: %d\n", slice, channel);

    //
    calc_sin_table();
    
    while (true) {
        for(int16_t idx = 0; idx < SIN_TABLE_SIZE; idx++) {
//            pwm_set_dc(slice, idx);
            pwm_set_dc(slice, (uint8_t) (50.0 + 45.0 * sin_table[idx]));
            sleep_us(4);
//          sleep_ms(20);
        }
//        for(int16_t idx = 95; idx > 5 /*SIN_TABLE_SIZE */; idx--) {
//            pwm_set_dc(slice, idx);
//            sleep_us(4);
//            sleep_ms(20);
//        }
    }
}
