//
// Knikkerbaan.
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

#include "servo.h"

#define SERVO_PIN   6


int main() {

    stdio_init_all();
    printf("DMA experiment with rp2040\n");

    // Inform
    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("frequency_count_khz = %d\n", f_sys_clk);

    //
    servo_enable(SERVO_PIN);
//    uint8_t servo_min_degree = 30;
//    uint8_t servo_max_degree = 150;
    
    while (true) {
        
        servo_set_position(SERVO_PIN, 0);
        sleep_ms(1000);
        servo_set_position(SERVO_PIN, 180);
        sleep_ms(1000);

    }

    return 0;
}

