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
void set_pixel(uint8_t *dpb, uint8_t x, uint8_t y);
void clr_pixel(uint8_t *dpb, uint8_t x, uint8_t y);
void draw_char(uint8_t *dpb, uint8_t x, uint8_t y, char ch, bool inverse);
void draw_text(uint8_t *dpb, uint8_t x, uint8_t y, char *str, bool inverse);
void clr_screen(uint8_t *dpb, uint8_t x, uint8_t y);
void display(uint8_t *pdb);
void disp_buf_debug(uint8_t *dpb);

// Global variables
//
uint8_t display_buffer[NR_MAX7219_DISPLAYS*8] = {0,};
// Reference to external font lookup
extern uint8_t funny[][8];

// Main loop
int main() {

    //
    stdio_init_all();
        
    stdio_flush();
    printf("MAX7219 leddriver example\n");
    printf("...\n");
    
    max7219_init();
    sleep_ms(250);

    // Test grenzen van het display, in 
    // alle hoekjes een pixel aan
    set_pixel(display_buffer, 0, 0); 
    set_pixel(display_buffer, 1, 0); 

    //disp_buf_debug(display_buffer);


    // Funny demo loop, not usefull
    char *str_a = " uC ";
    char *str_b = "NICE";
    while (true) {
	clr_screen(display_buffer, 0, 0);
	draw_text(display_buffer, 0, 0, str_a, false);
    	set_pixel(display_buffer, 0, 0); 
    	display(display_buffer);
	sleep_ms(500);

	clr_screen(display_buffer, 0, 0);
	draw_text(display_buffer, 0, 0, str_b, false);
    	clr_pixel(display_buffer, 0, 0); 
    	display(display_buffer);
	sleep_ms(500);
    }
}

extern uint8_t funny[][8];


/* ****************************************************** */
void draw_text(uint8_t *dpb, uint8_t x, uint8_t y, char *str, bool inverse) {
/*
 * 
 * notes   :
 ******************************************************** */
    uint8_t idx = 0;
    while(*str!=0) {
	    draw_char(dpb, x+8*idx, y, *str, inverse);
	    str++;
	    idx++;
    }
}

/* ****************************************************** */
void draw_char(uint8_t *dpb, uint8_t x, uint8_t y, char ch, bool inverse) {
/*
 * 
 * notes   :
 ******************************************************** */
   
    for(uint8_t idx = 0; idx < 8; idx++) {
        uint8_t b = funny[(uint8_t)ch][idx];
        for( uint8_t idy = 0; idy < 8; idy++ ) {
            if(b & (1<<idy) ) {
                inverse == true? clr_pixel(dpb, idx+x, idy+y): set_pixel(dpb, idx+x, idy+y);
            } else {
                inverse == true? set_pixel(dpb, idx+x, idy+y): clr_pixel(dpb, idx+x, idy+y);
            }
        }
    }
}

/* ****************************************************** */
void clr_screen(uint8_t *dpb, uint8_t x, uint8_t y) {
/*
 * 
 * notes   :
 ******************************************************** */

    for(uint8_t idy = 0; idy < 8; idy++) {
        for(uint8_t idx = 0; idx < 32; idx++) {
            clr_pixel(dpb, idx, idy);
        }
    }
    display(dpb);
    
}

/* ****************************************************** */
uint8_t reverse(uint8_t b) {
/*
 * 
 * notes   : Magic code from stackoverflow to reverse byte
 ******************************************************** */
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

/* ****************************************************** */
void display(uint8_t *pdb)
/*
 * 
 * notes   : Display buffer on actual device
 ******************************************************** */
{
    uint8_t buf[8];
    for(uint8_t idy = 0; idy < 8; idy++) {
        buf[0] = buf[2] = buf[4] = buf[6] = idy + 1;
        buf[1] = reverse(pdb[0 + 4*idy]);
        buf[3] = reverse(pdb[1 + 4*idy]);
        buf[5] = reverse(pdb[2 + 4*idy]);
        buf[7] = reverse(pdb[3 + 4*idy]);
        max7219_transfer(buf, 8);          
        sleep_ms(10);  
    }
}

/* ****************************************************** */
void disp_buf_debug(uint8_t *dpb)
/*
 * 
 * notes   : Debug display buffer onscreen
 ******************************************************** */
{
    printf("Display buffer content: \n");
    for(uint8_t idy = 0; idy < 8; idy++) {
        for(uint8_t idx = 0; idx < 4; idx++) {
            uint8_t byte = dpb[idx+4*idy];
            for(uint8_t idz = 0; idz < 8; idz++) {
                byte & (1<<idz)? printf("*") : printf(".");
            }
        }
        printf("\n");
    }
}

/* ****************************************************** */
void clr_pixel(uint8_t *pdb, uint8_t x, uint8_t y)
/*
 * in      : pdb is pointer to display buffer
 * notes   : Config the spi0 interface
 ******************************************************** */
{
    if( (x < 32) && (y < 8) ) {
        pdb[x/8+4*y] &= ~(1<<(x%8));
    }
}


/* ****************************************************** */
void set_pixel(uint8_t *pdb, uint8_t x, uint8_t y)
/*
 * 
 * notes   : Config the spi0 interface
 ******************************************************** */
{
    if( (x < 32) && (y < 8) ) {
        pdb[x/8+4*y] |= 1<<(x%8);
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
