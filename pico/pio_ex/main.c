#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "pwm.pio.h"

#define PWM_PIN 28

int main() {

    stdio_init_all();
    sleep_ms(500);
    stdio_flush();
    sleep_ms(500);
    printf("*\n");

    PIO pio = pio0; // PIO 0 of 1
    uint offset = pio_add_program(pio, &pwm_program);
    
    uint sm = pio_claim_unused_sm(pio, true);
    pwm_program_init(pio, sm, offset, PWM_PIN, 100000);

    uint8_t idx = 0;
    while (true) {
       
        printf("\\");
        sleep_ms(100);
        //pio_sm_put_blocking(pio, sm, (uint8_t)0xAA);
        pio_sm_put(pio, sm, (uint8_t)0xAA);
    
        
        printf("+");
        sleep_ms(100);
//        pio_sm_put_blocking(pio, sm, 0xAA);
        pio_sm_put(pio, sm, (uint8_t)0x81);
        
        if(++idx % 16 == 0 ) printf("\n");
    }
}
