//
// PICO mixer
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
//#include "hardware/adc.h"

#include "nco.pio.h"

// ADC
//#define CAPTURE_CHANNEL  0
//#define CAPTURE_DEPTH 1000
//uint8_t capture_buf[CAPTURE_DEPTH];

int main() {

    stdio_init_all();
    printf("PICO mixer\n");

    // init nco
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &nco_program);

    uint sm = pio_claim_unused_sm(pio, true);
    nco_program_init(pio, sm, offset, 15, 16, 517e3);

    nco_set_frequency(pio, sm, 517e3);
    printf("LO = %f Hz\n", 517e3 );
    sleep_ms(1000);

    while (true) {
       
//        nco_set_frequency(pio, sm, 419e3);
//        printf("LO = %f Hz\n", 419e3 );
//        sleep_ms(3500);
        
//        nco_set_frequency(pio, sm, 590e3);
//        printf("LO = %f Hz\n", 590e3);
//        sleep_ms(3500);

        sleep_ms(3000);
        printf(".\n");

    }
}
