//
// SITOR-B decoder.
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
#include "hardware/adc.h"
#include "hardware/pio.h"

#include "ccir476.h"

#define SITOR_PIN   17
#define DEBUG_PIN   18

typedef enum {EV_IDLE, EV_PHASING, EV_DATA} ENUM_EVENT;
typedef enum {ST_START, ST_IDLE, ST_PHASING, ST_DATA} ENUM_STATE;

typedef struct {
    void (*pre)(void);
    void (*heartbeat)(void);
    void (*post)(void);
    ENUM_STATE nextState;
} STATE_TRANSITION_STRUCT;

void start_pre() {
//    printf("start_pre()\n"); 
}

void start_heartbeat() {
//    printf("start_heartbeat()\n"); 
}

void start_post() {
//    printf("start_post()\n"); 
}

void idle_pre() {
//    printf("idle_pre()\n"); 
}

void idle_heartbeat() {
//    printf("idle_heartbeat()\n"); 
}

void idle_post() {
//    printf("idle_post()\n"); 
}


STATE_TRANSITION_STRUCT fsm[2][2] = {
    {
        {start_pre, start_heartbeat, start_post, ST_START}, 
        {start_pre, start_heartbeat, start_post, ST_IDLE}
    }, // State START
    {
        {idle_pre, idle_heartbeat, idle_post, ST_IDLE}, 
        {idle_pre, idle_heartbeat, idle_post, ST_IDLE}
    }, // Stae IDLE
};

ENUM_STATE state;
ENUM_EVENT event;

void initFSM(ENUM_STATE new_state, ENUM_EVENT new_event) {
    state = new_state;
    event = new_event;
    
    // call event.pre
    if( fsm[state][event].pre != NULL ) {
        fsm[state][event].pre();
    }
}

void raiseEvent(ENUM_EVENT new_event) {
    // call event.post
    if( fsm[state][event].post != NULL ) {
        fsm[state][event].post();
    }

    // Set new state
    ENUM_STATE new_state = fsm[state][new_event].nextState;

    // call newstate pre
    if( fsm[new_state][new_event].pre != NULL ) {
        fsm[new_state][new_event].pre();
    }

    // set new state
    state = new_state;

    // store event;
    event = new_event;
}

// callback from navtex decoder
void sitorb_msg_available(char *msg) {
    printf("%s", msg);
}

void sitorb_char_available(char c) {
    printf("%c", c);
}

void sitorb_err(char *msg) {
    printf("%s", msg);
}


#define HEARTBEAT_US      1000000
#define SBIT_L_US          9*1000 // 9ms <= validbit <= 11ms
#define SBIT_H_US         11*1000  
#define SIDLE_US          SBIT_L_US * 40
#define SGLITCH_US        4 * 2000

/*
 * Capture and handle FSK bits and glitched. Pass to ccir476 decoder
 *
 */
/*
void gpio_callback(uint gpio, uint32_t events) {

    static absolute_time_t last_irq_time;
    absolute_time_t now = get_absolute_time();
    absolute_time_t delta = absolute_time_diff_us(last_irq_time, now);

    if( delta <= SGLITCH_US ) {
        printf("[Glitch detected]\n");            
    } else {
        if( delta >= SIDLE_US ) {
            printf("[Idle detected]\n");            
            ccir476_rearm();
        } else {
        
            //
            // Bepaal aantal ontvangen 1 of 0 achterelkaar en level
            //
            uint8_t count = ((delta+5000)/10000);           
            uint8_t bit;
            
            if(events & GPIO_IRQ_EDGE_RISE) {
                bit = 0;
            } else {
                if(events & GPIO_IRQ_EDGE_FALL) {
                    bit = 1;
                } else {
                    printf("bitlevel not 0 or 1, huh??\n");
                }
            }
            
            // Handle all individual bits incl. synchronize
            for(uint8_t idx = 0; idx < count; idx++) {
                ccir476_process_bit(bit);            
            }
        }
    }

    last_irq_time = now;
}
*/

volatile int8_t cnt = 0;
volatile bool olev = false;
volatile bool nlev = false;
static absolute_time_t last_irq_time;
bool gpio_fsk_callback(struct repeating_timer *t) {
    
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
        
        // DEBUG
        gpio_put(DEBUG_PIN, nlev);
        
        // Calc delta
        absolute_time_t now = get_absolute_time();
        absolute_time_t delta = absolute_time_diff_us(last_irq_time, now);
        
        // If IDLE reset fsm else split delta in 10ms (SITOR-B, 100 baud) parts 
        // and pass to ccir476 processor. The ccir476 will sync and convert individual bits
        // to bytes and do the NAVTEX decoding
        if( delta >= SIDLE_US ) {
            printf("[Idle detected]\n");            
            ccir476_rearm();
        } else {
            uint8_t count = ((delta+5000)/10000);           
            for(uint8_t idx = 0; idx < count; idx++) {
                ccir476_process_bit(olev);            
            }
        }
//        printf("%d -> %.6lld\n", nlev, delta);
        olev = nlev;
        last_irq_time = now;
    }
    
    // return true for next periodic interrupt.
    return true;
}

int main() {

    stdio_init_all();
    printf("SITOR-B decoder\n");

    // Init decoder
    ccir476_init(&sitorb_msg_available, &sitorb_char_available, &sitorb_err); 

    // Init hardware
    gpio_init(DEBUG_PIN);
    gpio_set_dir(DEBUG_PIN, GPIO_OUT);

    gpio_init(SITOR_PIN);
    gpio_set_dir(SITOR_PIN, GPIO_IN);
    gpio_pull_up(SITOR_PIN);
//    gpio_set_irq_enabled_with_callback(
//        SITOR_PIN,
//        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
//        true,
//        &gpio_callback
//    );
    
    static struct repeating_timer timer;
    add_repeating_timer_us(-250, gpio_fsk_callback, NULL, &timer);

    // Inform
    uint32_t f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("System clock [khz] = %ld kHz\n", f_sys_clk);
    uint32_t f_adc_clk = clock_get_hz(clk_adc);
    printf("ADC clock [hz] = %ld Hz\n", f_adc_clk);

    initFSM(ST_START, EV_IDLE);

    absolute_time_t hb_now = get_absolute_time();
    absolute_time_t hb_lst = get_absolute_time();
   
     while (true) {
    
        // Handle fsm
        hb_now = get_absolute_time();
        uint64_t hb_delta =  absolute_time_diff_us(hb_lst, hb_now);
        if( hb_delta >= HEARTBEAT_US ) {
            hb_lst = hb_now;       

            // call to fsm heartbeat
            if( fsm[state][event].heartbeat != NULL) {
                fsm[state][event].heartbeat();
            }
        }
    }

}

