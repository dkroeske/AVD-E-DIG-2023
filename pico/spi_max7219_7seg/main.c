/* *************************************************

Hardware test SPI interface to MAX7219 7 segment Driver
 
MAX7219         PICO            (pin)  
---------------------------------------------------------
Vcc    <-->     Vbus            (#40)
GND    <-->     GND             (#38 or other ground pin)
CS     <-->     SPI0_CSn        (#22)
SI     <-->     SPIO_TX         (#25)
SCK    <-->     SPIO_SCK        (#24)

(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

// Het aantal displays
#define NR_7SEG_DISPLAYS 8

// function prototyping
void spi_cs_select();
void spi_cs_deselect();
void max7219_init();
//void max7219_transfer(uint8_t addr, uint8_t data, uint8_t size);
void max7219_transfer(uint8_t *buf, uint8_t size);

// Global variables
//

// Eenvoudige animatie op de losse segmenten. P8 datasheet
uint8_t seq_len = 9;
uint8_t seq[] = { 0x40, 0x20, 0x01, 0x04, 0x08, 0x10, 0x01, 0x02, 0x00 };

// Main loop
int main() {

    //
    stdio_init_all();
        
    stdio_flush();
    printf("MAX7219 7 segment leddriver example\n");
    
    max7219_init();
    sleep_ms(250);

	while(1==1) {
        
        // Decode Mode off - Losse segmenten aansturen
        max7219_transfer( (uint8_t[]) {0x09, 0x00}, 2);
        
        // Animatie 1
        uint8_t tmp[2];
        for(uint8_t idx = 0; idx < NR_7SEG_DISPLAYS; idx++) {
            for(uint8_t idy = 0; idy < seq_len; idy++) {
                tmp[0] = idx+1;
                tmp[1] = seq[idy];
                max7219_transfer(tmp, 2);
                sleep_ms(100);                   
            }
        }
        
        
        // Decode Mode ON - chip stuurt segmenten aan via lookup
        max7219_transfer( (uint8_t[]) {0x09, 0xFF}, 2);
        
        // Breek cnt op in eenheden, 10 tallen enz.
        uint16_t cnt = 1000;
        while( cnt-- != 0 ) {
            tmp[0] = 1;
            tmp[1] = (cnt/1) % 10;
            max7219_transfer(tmp, 2);
        
            tmp[0] = 2;
            tmp[1] = (cnt/10) % 10 ;
            max7219_transfer(tmp, 2);
        
            tmp[0] = 3;
            tmp[1] = (cnt/100) % 10 ;
            max7219_transfer(tmp, 2);
        
            tmp[0] = 4;
            tmp[1] = (cnt/1000) % 10 ;
            max7219_transfer(tmp, 2);

            sleep_ms(75);                   
        }
    }
}


/* ****************************************************** */
void max7219_init(void)
/*
 * 
 * notes   : Config the spi0 interface
 * version : DMK 2309, initial code
 ******************************************************** */
{

    // pico speed
    spi_init(spi0, 100000);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    
    // default spi settings
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    
    // setup spio default chip select pin
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    // Display test (0x0F) see p7 datasheet. 4 matrix boards simultanious
    max7219_transfer( (uint8_t[]) {0x0F, 0x01}, 2);
    sleep_ms(50);
    max7219_transfer( (uint8_t[]) {0x0F, 0x00}, 2);
    sleep_ms(50);

    // Decode Mode off (
    max7219_transfer( (uint8_t[]) {0x09, 0x00}, 2);

    // Intensity
    max7219_transfer( (uint8_t[]) {0x0A, 0x00}, 2);

    // All rows on
    max7219_transfer( (uint8_t[]) {0x0B, 0x07}, 2);

    // Turn on chip
    max7219_transfer( (uint8_t[]) {0x0C, 0x01}, 2);
}


/* ****************************************************** */
void max7219_transfer(uint8_t *buf, uint8_t size)
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ****************************************************** */
{
    
//    for(uint8_t idx = 0; idx < size; idx++) {
//        printf("0x%.2X ", buf[idx]);
//    }
//    printf("\n");

    spi_cs_select();
    spi_write_blocking(spi_default, buf, size);
    spi_cs_deselect();
}


/* ****************************************************** */
void spi_cs_select()
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ******************************************************** */
{
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
}


/* ****************************************************** */
void spi_cs_deselect()
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ******************************************************** */
{
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
}
