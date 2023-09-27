//
// Hardware SPI interface to MCP2515 CAN interface
//

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

// function prototyping
void spi_cs_select();
void spi_cs_deselect();
void mcp2515_reset();
void mcp2515_init();
uint8_t mcp2515_read_register(int8_t addr);


// Main loop
int main() {

    stdio_init_all();

    while (true) {
        
        // Do something useless
        sleep_ms(2500);
        
        //Just to inform user
        printf("+\n");
    }
}

/* ****************************************************** */
void mcp_2515_init(void)
/*
 * 
 * notes   : 
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
}

/* ****************************************************** */
void mcp2515_reset()
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ******************************************************** */
{
    // reset mcp2515 chip, page xx datasheet
    uint8_t buf[] = { 0xC0 };
    spi_cs_select();
    spi_write_blocking(spi_default, buf, 1);
    spi_cs_deselect();
}

/* ****************************************************** */
uint8_t mcp2515_read_register(int8_t addr)
/*
 * 
 * notes   : 
 * version : DMK 2309, initial code
 ******************************************************** */
{
    uint8_t buf[2];
    buf[0] = 0x03; // CAN READ
    buf[1] = addr;
    uint8_t data;

    spi_cs_select();
    spi_write_blocking(spi_default, buf, 2);
    spi_read_blocking(spi_default, 0, &data, 1);
    spi_cs_deselect();

    return data;
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
