//
// Hello world PICO style
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define PWM_OUT 28 
#define PWM_FREQ 30000                  // fdc = 30kHz

uint calc_dc(){
    return 10;
}

int main() {

    stdio_init_all();

    // Set pwm
    gpio_set_function(PWM_OUT, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PWM_OUT);
    uint channel = pwm_gpio_to_channel(PWM_OUT);

    // Timing
    // Zonder prescaler: fclk = 125MHz. counter = fclk/fdc = 4167
    pwm_set_wrap(slice, 4167);

    // Dutycycle 21% -> 4167/100*21 = 875.07
    pwm_set_chan_level(slice, PWM_OUT, 875);

    // Go
    pwm_set_enabled(slice, channel);

    printf("pwm is running with slice: %d and channel: %d\n", slice, channel);

    while (true) {
        
        // Do something useless
        sleep_ms(2500);
        
        //Just to inform user
        printf("+\n");
    }
}
