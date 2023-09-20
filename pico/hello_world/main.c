//
// Hello world PICO style
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"

#define LED 1
#define SW  22

int main() {

    stdio_init_all();
    printf("Hello world, PICO style\n");

    // Init all I/O
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    while (true) {
        
        //LED on and wait
        sleep_ms(250);
        gpio_put(LED, 0);
        
        //LED off and wait
        sleep_ms(250);
        gpio_put(LED, 1);
        
        //Just to inform user
        printf("+\n");
    }
}
