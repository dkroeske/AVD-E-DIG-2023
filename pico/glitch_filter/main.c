//
// Glitch filter.
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
#include "hardware/timer.h"

#define SITOR_PIN       16
#define FILTER_PIN      18
#define TRIGGER_PIN     19

#define DMA_DEBUG

#define SBIT_L_US          9*1000 // 9ms <= validbit <= 11ms
#define SBIT_H_US         11*1000  
#define SIDLE_US          SBIT_L_US * 40
#define SGLITCH_US        4 * 2000


/*
 *
 */
/*
void gpio_callback(uint gpio, uint32_t events) {
    
    static absolute_time_t last_irq_time;
    absolute_time_t now = get_absolute_time();
    absolute_time_t delta = absolute_time_diff_us(last_irq_time, now);

    // 0->1 transition
    if(events & GPIO_IRQ_EDGE_RISE) {
    } else { 
        if( events & GPIO_IRQ_EDGE_FALL) {
        } else {
        }
    }
    
    last_irq_time = now;

    //printf("%.6lld (%d,%d) %s\n", delta, curr, prev, glitch == true ? "*": "");
}
*/

volatile int8_t cnt = 0;
volatile bool olev = false;
volatile bool nlev = false;
static absolute_time_t last_irq_time;
bool gpio_sample_callback(struct repeating_timer *t) {
    
    bool level = gpio_get(SITOR_PIN);

    if(level) {
        if( cnt < 3 ) {
            cnt++; 
        } else {
            nlev = true;
        }
    }
    else {
        if( cnt > -3 ) {
            cnt--;
        } else {
            nlev = false;
        }
    }

    //
    if(olev != nlev) {
        gpio_put(FILTER_PIN, nlev);
        olev = nlev;
        absolute_time_t now = get_absolute_time();
        absolute_time_t delta = absolute_time_diff_us(last_irq_time, now);
        printf("%d -> %.6lld\n", nlev, delta);
        last_irq_time =  now;
    }
    
    // return true for next periodic interrupt.
    return true;
}


typedef struct {
    bool level;
    uint32_t delay_us;
} BSTREAM_STRUCT;


#define NR_SAMPLES 10
BSTREAM_STRUCT bstream[] = { 
    {1, 10000 },
    {0, 20000 },

    {1, 14000 },
    {0,   400 },
    {1, 15600 },

    {0, 40000 },
    {1, 20000 },
    {0, 200000 },
    {1,  400  },
    {0, 100000 }
};

int main() {

    stdio_init_all();
    printf("Glitch filter experiment\n");

    gpio_init(SITOR_PIN);
    gpio_set_dir(SITOR_PIN, GPIO_OUT);

    gpio_init(FILTER_PIN);
    gpio_set_dir(FILTER_PIN, GPIO_OUT);

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);

    // Inform
    uint32_t f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("System clock [khz] = %ld kHz\n", f_sys_clk);

    // SITOR decoder
//    gpio_init(SITOR_PIN);
//    gpio_set_dir(SITOR_PIN, GPIO_IN);
//    gpio_pull_up(SITOR_PIN);
//    gpio_set_irq_enabled_with_callback(
//        SITOR_PIN,
//        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
//        true,
//        &gpio_callback
//    );

    static struct repeating_timer timer;
    add_repeating_timer_us(-250, gpio_sample_callback, NULL, &timer);

    while (true) {
        for(uint8_t idx = 0; idx < NR_SAMPLES; idx++) {
            gpio_put( SITOR_PIN, bstream[idx].level );
            sleep_us( bstream[idx].delay_us );
        }
    }
}

