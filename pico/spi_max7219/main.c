/* *************************************************

Hardware test SPI interface to MAX7219 LED Driver
 
MAX7219         PICO            (pin)  
---------------------------------------------------------
Vcc    <-->     Vbus            (#40)
GND    <-->     GND             (#38 or other ground pin)
CS     <-->     SPI0_CSn        (#22)
SO     <-->     SPIO_RX         (#21)
SI     <-->     SPIO_TX         (#25)
SCK    <-->     SPIO_SCK        (#24)
INT             Not connected

(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

// function prototyping
void spi_cs_select();
void spi_cs_deselect();
void max7219_init();
void max7219_transfer(uint8_t addr, uint8_t data);

// Main loop
int main() {

    //
    stdio_init_all();
        
    max7219_init();
    sleep_ms(2000);

    stdio_flush();
    printf("MAX7219 leddriver example\n");

    while (true) {

        // Do something useless
        sleep_ms(2500);
        
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
    spi_init(spi0, 10000);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    
    // default spi settings
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    
    // setup spio default chip select pin
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    // Display test (0x0F) see p7 datasheet
    max7219_transfer(0x0F, 0x01);
    sleep_ms(100);
    max7219_transfer(0x0F, 0x00);

}


/* ****************************************************** */
void max7219_transfer(uint8_t addr, uint8_t data)
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ****************************************************** */
{
    uint8_t buf[2*4];
    buf[0] = addr;
    buf[1] = data;
    buf[2] = addr;
    buf[3] = data;
    buf[4] = addr;
    buf[5] = data;
    buf[6] = addr;
    buf[7] = data;

    spi_cs_select();

    spi_write_blocking(spi_default, buf, 8);
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
