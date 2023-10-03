//
// Hello world PICO style
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED 1
#define SW  20

int main() {

    stdio_init_all();
    printf("Hello world, PICO style\n");

    // Init all I/O
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
    
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    int sleep = 0;
    while (true) {
        
        if(gpio_get(SW)) {
            sleep = 1000;
            printf("+\n");
        } else {
            sleep = 130;
            printf("-\n");
        }       

        //LED on and wait
        sleep_ms(sleep);
        gpio_put(LED, 0);
        
        //LED off and wait
        sleep_ms(sleep);
        gpio_put(LED, 1);
        
        //Just to inform user
//        printf("+\n");
    }
}
