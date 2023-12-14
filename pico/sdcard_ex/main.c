/* *************************************************

Test SD card reader/writer with flex and bison 
config file parser

Based on github.com/no-OS-FatFS-SDIO-SPI-RPI-Pico
 
(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include "f_util.h"
#include "ff.h"
#include "rtc.h"
#include "hw_config.h"

#include "common.h"
#include "config.h"

// Main loop
int main() {

    //
    stdio_init_all();
    time_init();
//    stdio_flush();
    sleep_ms(2000);

    // Read config from card
    if( OK == config_init("config.txt") ) {
        config_show();
    } else {
        printf("default config\n");
    }

    uint8_t x = 0;
    while(true){        
        printf("0x%.2X\n", x++);
        sleep_ms(3000);
    }
}

