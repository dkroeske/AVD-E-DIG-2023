/* *************************************************

Basic MCP2515 CAN application. Rx and Tx of CAN frames
 
(c) dkroeske@gmail.com

v1.0    2023-10-10: Initial code

***************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "mcp2515.h"
#include "mcp2515_regs.h"

// Function prototypes
//
// CAN callbacks
void on_can_rx(CAN_DATA_FRAME_STRUCT *frame);
void on_can_tx(CAN_DATA_FRAME_STRUCT *frame);
void on_can_err(CAN_ERR_FRAME_STRUCT *err);
//
// CAN frame/err debug printf's
void debug_dataframe(CAN_DATA_FRAME_STRUCT *frame);
void debug_errframe(CAN_ERR_FRAME_STRUCT *frame); 

// Global variables
CAN_DATA_FRAME_STRUCT frame;

// Main loop
int main() {

    //
    stdio_init_all();
        
    // set mode { bijvoorbeeld: REQOP_LOOPBACK, REQOP_NORMAL }
    can_init(REQOP_LOOPBACK);
    
    // Init CAN and set callback handlers
    can_set_rx_handler(&on_can_rx);
    can_set_tx_handler(&on_can_tx);
    can_set_err_handler(&on_can_err);

    //
    while (true) {

        // 
        uint8_t data =  mcp2515_read_register(CANSTAT);
        switch (data >> 5) {
            case 0x00: 
                printf("Normal Operating mode\n");
                break;
            case 0x01: 
                printf("Sleep mode\n");
                break;
            case 0x02: 
                printf("Loopback mode\n");
                break;
            case 0x03: 
                printf("Listen-Only mode\n");
                break;
            case 0x04: 
                printf("Configuration mode\n");
                break;
            default: 
                printf("Invalid mode!\n");
                break;
        } 
        
        // Do something useless
        sleep_ms(2500);
        
    }
}


/* CAN CALLBACK FUNCTIONS */

/* ************************************************************* */
void on_can_rx(CAN_DATA_FRAME_STRUCT *frame) 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    puts(">> on_can_rx()");
    debug_dataframe(frame);
    puts("<< on_can_rx()");
}

/* ***************************************************************************************** */
void on_can_tx(CAN_DATA_FRAME_STRUCT *frame) 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    puts(">> can_tx()");
    puts("<< can_tx()");
}

/* ***************************************************************************************** */
void on_can_err(CAN_ERR_FRAME_STRUCT *err) 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    puts(">> on_can_err()");
    debug_errframe(err);
    puts("<< on_can_err()");
}

/* UTIL FUNCTIONS */

/* ***************************************************************************************** */
void debug_dataframe(CAN_DATA_FRAME_STRUCT *frame) 
/* 
short	: Print CAN dataframe to terminal
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    char buf[1048] = "";
    char line[80];
    // sprintf(line, "*********** CAN DATAFRAME *************\n");
    // strcat(buf, line);

    sprintf(line, "ID: 0x%.8X ", (unsigned int)frame->id);
    strcat(buf, line);

    // sprintf(line, "Datalen: %.2d\n", frame->datalen);
    // strcat(buf,line);
    for(uint8_t idx = 0; idx < frame->datalen; idx++) {
        sprintf(line, "0x%.2X ", frame->data[idx]);
        strcat(buf, line);
    }
    //strcat(buf, "\n");
    
    puts(buf);
}

/* ***************************************************************************************** */
void debug_errframe(CAN_ERR_FRAME_STRUCT *frame) 
/* 
short	: Print CAN errors to terminal
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    printf("*********** MCP 2515 ERROR *************\n");
    printf("REC    : 0x%.2X\n", frame->rREC);
    printf("TEC    : 0x%.2X\n", frame->rTEC);
    printf("EFLG   : 0x%.2X\n", frame->rEFLG);
    printf("CANINTF: 0x%.2X\n", frame->rCANINTF);
}
