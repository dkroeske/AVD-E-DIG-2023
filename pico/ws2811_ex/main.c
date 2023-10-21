/* *************************************************

Test WS2811 Pixel Statemachine, based on RPI example
code
 
(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "ws2812.h"
#include "ws2812.pio.h"

typedef struct {
    uint8_t r,g,b;
} RGB_STRUCT;

#define NR_PIXELS 60

RGB_STRUCT pixels[NR_PIXELS];

// Function to write array to ws2811 strand
void play_pixels(RGB_STRUCT *rgb, uint8_t length) {
    for(uint8_t idx = 0; idx < length; idx++) {
        pixel(rgb[idx].r, rgb[idx].g, rgb[idx].b);
    }
}

// Main loop
int main() {

    //
    stdio_init_all();
       
    // Init pixels
    ws2812_init();

    while(true){        
    
        // Soort van knipper effect voor pixel string
        for (int idx = 0; idx < NR_PIXELS; idx++) {
            pixels[idx].r = 0;
            pixels[idx].g = 0;
            pixels[idx].b = 0;
        }
        uint8_t index = rand() % NR_PIXELS;
        pixels[index].r = 0x55;
        pixels[index].g = 0x55;
        pixels[index].b = 0x55;

        //
        play_pixels(pixels, NR_PIXELS);

        sleep_ms(200);
    
        // Rood knipperen
//        pixel(0x55, 0x00, 0x00);
//        sleep_ms(200);
//        pixel(0x00, 0x00, 0x00);
//        sleep_ms(200);
        
    }
}

