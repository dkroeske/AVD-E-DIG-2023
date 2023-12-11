/* *************************************************

Test SD card reader/writer

Based on github.com/no-OS-FatFS-SDIO-SPI-RPI-Pico
 
(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include "f_util.h"
#include "ff.h"
#include "rtc.h"
#include "hw_config.h"

// Main loop
int main() {

    //
    stdio_init_all();
    time_init();
    stdio_flush();
    sleep_ms(2000);

    //
    sd_card_t *pSD = sd_get_by_num(0);  
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if( FR_OK != fr) {
        panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    } 
    
/*
    FIL fil;
    const char* const filename = "test.txt";
    fr = f_open(&fil, filename, FA_READ);
    if( FR_OK != fr && FR_EXIST != fr) {
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }

    char linebuf[40] = "";
    while(NULL != f_gets(linebuf, 40, &fil)) {
        printf("%s", linebuf);
    }
    f_close(&fil);
*/

    while(true){        
    
        FIL fil;
        const char* const filename = "test.txt";
        fr = f_open(&fil, filename, FA_READ);
        if( FR_OK != fr && FR_EXIST != fr) {
            panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        }

        char linebuf[40] = "";
        while(NULL != f_gets(linebuf, 40, &fil)) {
            printf("%s", linebuf);
        }
        f_close(&fil);


        printf("\n");
        sleep_ms(3000);
    }

    f_unmount(pSD->pcName);
}

