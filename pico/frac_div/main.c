//
// Fraction Divider
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

#define OUT 10
#define SW  20

uint8_t N = 2;  // integer part
uint8_t K = 3;  // fractional part

uint8_t down_cnt = 2;
bool overflow = false;

bool out = false;
struct repeating_timer timer0;
uint8_t accumulator = 0;

//
bool timer_irq(struct repeating_timer *t) {


    //
    down_cnt--;
    printf("%.2d | %.2d | %.2d %.2d\n", accumulator, K, overflow, down_cnt);


    //
    if( down_cnt == 0 )
    {
        // 3 bits accumulator
        if((accumulator + K) / 8) {
            overflow = true;
        } else {
            overflow = false;
        }
        accumulator += K;
        accumulator %= 8;

        // reload with overflow
        if(overflow) {
            down_cnt = N + 1;
        } else {
            down_cnt = N;
        }
    
        //
        // printf("%.2d | %.2d | %.2d %.2d\n", accumulator, K, overflow, down_cnt);

        // output change
        out ^= true;
        gpio_put(OUT, out);
    }

//    if(down_cnt >= N) printf("\n");


    // output change
//    out ^= true;
//    gpio_put(OUT, out);
    return true;
}

int main(void) {

    stdio_init_all();
    printf("Fractional Divider Example\n");

    // Init all I/O
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
    
    gpio_init(OUT);
    gpio_set_dir(OUT, GPIO_OUT);

    add_repeating_timer_us(500000, &timer_irq, NULL, &timer0);

    while (true) {

        if( down_cnt == 0 ) {
//            printf("%.2d | %.2d | %.2d\n", accumulator, K, overflow);
        }
        
//        if(gpio_get(SW)) {
//            printf("+\n");
//        } else {
//            printf("-\n");
//        }
//
        sleep_ms(1000); 
    }
}
