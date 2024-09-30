//
// Hello world PICO style
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define SW  20

int main() {

    stdio_init_all();
    printf("Hello world, PICO-2 style\n");

    // Init all I/O
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);

    
    // default led op het pico board
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    int sleep = 0;
    while (true) {
        
        if(gpio_get(SW)) {
            sleep = 1000;
            printf("Welkom E-jedi's!\n");
        } else {
            sleep = 250;
            printf("...............!\n");
        }       

        //LED on and wait
        sleep_ms(sleep);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        
        //LED off and wait
        sleep_ms(sleep);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }
}

