//
// PICO rx
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "nco.pio.h"


int main() {

    stdio_init_all();
    printf("PICO iq_nco\n");

    // init nco
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &nco_program);

    uint sm = pio_claim_unused_sm(pio, true);
    nco_program_init(pio, sm, offset, 11, 12, 419e3);

    while (true) {
       
        nco_set_frequency(pio, sm, 419e3);
        printf("IQ clock = %f Hz\n", 419e3 );
        sleep_ms(3500);
        
        nco_set_frequency(pio, sm, 590e3);
        printf("IQ clock = %f Hz\n", 590e3);
        sleep_ms(3500);
    }
}
