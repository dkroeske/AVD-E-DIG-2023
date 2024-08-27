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

RETVAL mount_sd(void);

// Main loop
int main() {

    //
    stdio_init_all();
    sleep_ms(2000);

    stdio_flush();
    printf("PICO SD card example\n");

    //
    RETVAL retval = mount_sd();

    // Read config from card
    if(OK == retval) {
        if( OK == config_init("config.txt") ) {
            config_show();
        } else {
            printf("default config\n");
        }
    }

    uint8_t x = 0;
    while(true){        
        printf("\n0x%.2X\n", x++);
        sleep_ms(3000);
    }
}


RETVAL mount_sd(void){
    RETVAL retval = OK;
    
    if( OK == retval) {
        sd_card_t *pSD = sd_get_by_num(0);
        FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
        if( FR_OK != fr ) {
            retval = NOK;
        } else {
            sleep_ms(250);
        }
    }
    return retval;
}
