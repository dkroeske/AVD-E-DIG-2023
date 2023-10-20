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

// Main loop
int main() {

    //
    stdio_init_all();
       
    // Init pixels
    ws2812_init();

    while(true){        
    
        float div = clock_get_hz(clk_sys)/(800000*10.0);
        printf("div: %.6f\n", div);

        // Red
        pixel(0x55, 0x00, 0x00);
        sleep_ms(500);
        
        // Green
        pixel(0x00, 0x55, 0x00);
        sleep_ms(200);
        
        // Blue
        pixel(0x00, 0x00, 0x55);
        sleep_ms(200);
        
    }
}

