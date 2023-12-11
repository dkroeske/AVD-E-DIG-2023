/* *************************************************

Stepper example with 28BYJ-48 5V with UN2003
 
MAX7219         PICO            (pin)  
---------------------------------------------------------
Vcc    <-->     Vbus            (#40)
GND    <-->     GND             (#38 or other ground pin)
IN1    <-->     GP18            (#24)
IN2    <-->     GP19            (#25)
IN3    <-->     GP20            (#26)
IN4    <-->     GP21            (#27)

(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define M1      18
#define M2      19
#define M3      20
#define M4      21

#define DELAY   2

uint8_t stepper_io[4] = {M1, M2, M3, M4};
uint8_t stepper_ccw[] = {3, 6, 12, 9};
uint8_t stepper_cw[] = {9, 12, 6, 39};

typedef enum {
    cw,
    ccw,
} stepper_mode_t;

typedef uint8_t stepper_speed_t;
typedef uint16_t stepper_steps_t;

stepper_mode_t stepper_mode = cw;
stepper_speed_t stepper_speed = 2;
stepper_steps_t stepper_steps = 0;

void stepper_set_mode(stepper_mode_t mode) {
    stepper_mode = mode;
}

void stepper_set_speed(stepper_speed_t speed) {
    stepper_speed = speed;
}

void stepper_set_steps(stepper_steps_t steps) {
    stepper_steps = steps;
}

void do_stepper(void) {
    
    uint8_t step_index = 0;
    for( uint16_t idx = 0; idx < stepper_steps; idx++) {
        
            uint8_t seq = 0;
            switch(stepper_mode) {
                case cw:
                    seq = stepper_cw[step_index];
                    break;
                case ccw:
                    seq = stepper_ccw[step_index];
                    break;
            }
            step_index++;
            step_index%=4;

            gpio_put(stepper_io[0], ((seq>>3)&0x01)); 
            gpio_put(stepper_io[1], ((seq>>2)&0x01)); 
            gpio_put(stepper_io[2], ((seq>>1)&0x01)); 
            gpio_put(stepper_io[3], ((seq>>0)&0x01)); 
            sleep_ms(stepper_speed);
    }
}

// Main loop
int main() {

    //
    stdio_init_all();
   
    stdio_flush();

    // Init IO
    for(uint8_t idx = 0; idx < 4; idx++) {
        gpio_init(stepper_io[idx]);
        gpio_set_dir(stepper_io[idx], GPIO_OUT);
        gpio_put(stepper_io[idx], 0);
    }

    while (true) {
        
        stepper_steps = 1000;
        stepper_mode = cw;
        stepper_speed = 100;
        do_stepper();

        sleep_ms(1000); 

    }
}

/* ****************************************************** */
void stepper_init(void)
/*
 * 
 * notes   : init I/O for stepper
 * version : DMK 2309, initial code
 ******************************************************** */
{
//    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
//    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
//    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
}

