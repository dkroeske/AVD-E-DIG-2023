/* *************************************************

Hardware test SPI interface to MAX7219 LED Driver
 
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

//
#define NR_MAX7219_DISPLAYS 4

// function prototyping
void spi_cs_select();
void spi_cs_deselect();
void max7219_init();
//void max7219_transfer(uint8_t addr, uint8_t data, uint8_t size);
void max7219_transfer(uint8_t *buf, uint8_t size);
void set_pixel(uint8_t x, uint8_t y);
void clr_pixel(uint8_t x, uint8_t y);
void disp_buf_debug(uint8_t *disp_buffer);

// Global variables
uint8_t display_buffer[4*8] = {0,};

// Main loop
int main() {

    //
    stdio_init_all();
        
    stdio_flush();
    printf("MAX7219 leddriver example\n");
    printf("...\n");
    
    max7219_init();
    sleep_ms(2000);

    set_pixel( 31, 0);
    set_pixel( 0, 1);
    disp_buf_debug(display_buffer);

    while (true) {
        set_pixel( 7, 0);
        clr_pixel( 8, 0);
        disp_buf_debug(display_buffer);
        sleep_ms(500);

        clr_pixel( 7, 0);
        set_pixel( 8, 0);
        disp_buf_debug(display_buffer);
        sleep_ms(500);
        
//        set_pixel( 0, 1);
//        clr_pixel( 31, 1);
//        disp_buf_debug(display_buffer);
//        sleep_ms(500);

//        set_pixel( 0, 1);
//        clr_pixel( 31, 1);
//        disp_buf_debug(display_buffer);
//        sleep_ms(500);
    }
}


/* ****************************************************** */
void disp_buf_debug(uint8_t *disp_buffer)
/*
 * 
 * notes   : Debug display buffer onscreen
 ******************************************************** */
{
    printf("Display buffer content: \n");
    for(uint8_t idy = 0; idy < 8; idy++) {
        for(uint8_t idx = 0; idx < 4; idx++) {
            uint8_t byte = display_buffer[idx+4*idy];
            for(uint8_t idz = 0; idz < 8; idz++) {
                byte & (1<<idz)? printf("*") : printf(".");
            }
        }
        printf("\n");
    }
}

/* ****************************************************** */
void clr_pixel(uint8_t x, uint8_t y)
/*
 * 
 * notes   : Config the spi0 interface
 ******************************************************** */
{
    display_buffer[x/8+4*y] &= ~(1<<(x%8));
}


/* ****************************************************** */
void set_pixel(uint8_t x, uint8_t y)
/*
 * 
 * notes   : Config the spi0 interface
 ******************************************************** */
{
    display_buffer[x/8+y] |= 1<<(x%8);
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
    spi_init(spi0, 10000);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    
    // default spi settings
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    
    // setup spio default chip select pin
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    // Display test (0x0F) see p7 datasheet. 4 matrix boards simultanious
    uint8_t data[8] = {0x0F, 0x01, 0x0F, 0x01, 0x0F, 0x01, 0x0F, 0x01};
    max7219_transfer( data, 8);
    sleep_ms(50);
    max7219_transfer( (uint8_t[]) {0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00}, 8);

    // Decode Mode off (
    max7219_transfer( (uint8_t[]) {0x09, 0x00, 0x09, 0x00, 0x09, 0x00, 0x09, 0x00}, 8);

    // Intensity
    max7219_transfer( (uint8_t[]) {0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00}, 8);

    // All rows on
    max7219_transfer( (uint8_t[]) {0x0B, 0x07, 0x0B, 0x07, 0x0B, 0x07, 0x0B, 0x07}, 8);

    // Turn on chip
    max7219_transfer( (uint8_t[]) {0x0C, 0x01, 0x0C, 0x01, 0x0C, 0x01, 0x0C, 0x01}, 8);
}


/* ****************************************************** */
void max7219_transfer(uint8_t *buf, uint8_t size)
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ****************************************************** */
{
    
    for(uint8_t idx = 0; idx < size; idx++) {
        printf("0x%.2X ", buf[idx]);
    }
    printf("\n");

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
