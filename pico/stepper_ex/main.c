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
uint8_t stepper_sequence[] = {3, 6, 12, 9};


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

    uint8_t step_index = 0;
    while (true) {

        for( uint16_t idx = 0; idx < 2048; idx++) {
            
            uint8_t seq = stepper_sequence[step_index];
            step_index++;
            step_index%=4;

            gpio_put(stepper_io[0], ((seq>>3)&0x01)); 
            gpio_put(stepper_io[1], ((seq>>2)&0x01)); 
            gpio_put(stepper_io[2], ((seq>>1)&0x01)); 
            gpio_put(stepper_io[3], ((seq>>0)&0x01)); 
            sleep_ms(DELAY);
        }

        gpio_put(M1, 0); gpio_put(M2, 0); gpio_put(M3, 0); gpio_put(M4, 0);
        while(1==1);
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

