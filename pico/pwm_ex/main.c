//
// Hello world PICO style
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"


#define PWM_OUT     18
#define DEBUG_OUT   16 

#define TABLE_SIZE  32

uint8_t sin_table[TABLE_SIZE];
uint16_t i = 0;


// isr
void pwm_interrupt_handler() {
    pwm_clear_irq(pwm_gpio_to_slice_num(PWM_OUT));

    if(i <= 8) gpio_put(DEBUG_OUT, 1); else gpio_put(DEBUG_OUT, 0);

    pwm_set_gpio_level(PWM_OUT, sin_table[i>>3]);
    i++;
    i%=(TABLE_SIZE<<3);
}

int main() {

    stdio_init_all();
//    set_sys_clock_khz(176000, true);

    // precalc sinus one period in array of 100 bytes
    for(uint8_t idx = 0; idx < TABLE_SIZE; idx++) {
        sin_table[idx] = (uint8_t)(122+(120.0 * sin((2.0 * M_PI * 1 * idx)/TABLE_SIZE)));
    }

//    for(uint8_t idx = 0; idx < 100; idx++) {
//        printf("%.2d ", sin_table[idx]);
//        if((idx % 16) == 0) printf("\n");
//    }

    // Set Debug i/o
    gpio_init(DEBUG_OUT);
    gpio_set_dir(DEBUG_OUT, GPIO_OUT);


    // Set pwm
    gpio_set_function(PWM_OUT, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PWM_OUT);

    // wrap 255 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 250);

    // set clock divider
    pwm_config_set_clkdiv(&config, 1.0f);
    
    // Set irq
    pwm_clear_irq(slice);
    pwm_set_irq_enabled(slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);
 
    pwm_init(slice, &config, true);

    //
    pwm_set_gpio_level(PWM_OUT, 0);


    while (true) {
        
        // Do something useless
        sleep_ms(2500);
        
        //Just to inform user
        printf("+\n");
    }
}
