/* *************************************************

Basic MCP2515 CAN library. Rx and Tx of CAN frames
 
Just to test the PICO to MCP2515 interface. Connections:
 
MCP2515         PICO            (pin)  
---------------------------------------------------------
Vcc    <-->     Vbus            (#40)
GND    <-->     GND             (#38 or other ground pin)
CS     <-->     SPI0_CSn        (#22)
SO     <-->     SPIO_RX         (#21)
SI     <-->     SPIO_TX         (#25)
SCK    <-->     SPIO_SCK        (#24)
INT    <-->     GPIO            (#21)         

(c) dkroeske@gmail.com

v1.0    2023-10-10: Initial code

***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/util/queue.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "mcp2515_regs.h"
#include "mcp2515.h"

// 
static inline void spi_cs_select();
static inline void spi_cs_deselect();

// prototyping CAN cmd's

// CAN function pointers
void (*can_rx_fp)(CAN_DATA_FRAME_STRUCT *) = NULL;
void (*can_tx_fp)(CAN_DATA_FRAME_STRUCT *) = NULL;
void (*can_err_fp)(CAN_ERR_FRAME_STRUCT *) = NULL;

// CANbus message Queue
queue_t rx_message_queue;
queue_t tx_message_queue;


/* ************************************************************** */
void can_init(uint8_t mode)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    // Init SPI
    spi_init(spi0, 100000);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "spi CS"));

    // Init rx_message queue - raw CAN message is stored
    queue_init(&rx_message_queue, 14*sizeof(uint8_t), 100);

    // Init tx_message queue - raw CAN message is stored
    queue_init(&tx_message_queue, 14*sizeof(uint8_t), 100);

    // Setup interrupt handler for mcp2515 interrupts
    gpio_set_irq_enabled_with_callback(21, GPIO_IRQ_EDGE_FALL, true, &mcp2515_callback);

    // Init mcp2515
    mcp2515_init(mode);
	
}


/**************************************************************** */
uint8_t can_read_msg(CAN_DATA_FRAME_STRUCT *frame) 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
*******************************************************************/
{
    uint8_t buf[14];
   
    // Read queue
    bool done = queue_try_remove(&rx_message_queue, buf);
    
    if(done) {
        // Check if standard or extended id 
        if( buf[1] & 0x08 ) {
            // Extended ID
            uint8_t b3 = (buf[0] >> 3);  
            uint8_t b2 = (buf[0] << 5) | ((buf[1]&0xE0) >>3) | (buf[1]&0x03);
            uint8_t b1 = buf[2];
            uint8_t b0 = buf[3];
            frame->id = b3 << 24 | b2 << 16 | b1 << 8 | b0;
        } else {
            // Standard ID
            frame->id = (buf[1] << 3) | (buf[1] >> 5);
        }
        
        // Get datalenght (clip to max. 8 bytes) 
        frame->datalen = (buf[4]&0x0F) <= 8 ? buf[4]&0x0F : 8;
        
        // Get actual data
        for(uint8_t idx = 0; idx < frame->datalen; idx++) {
           frame->data[idx] = buf[5+idx];
        }
    }
    return done;
}

/**************************************************************** */
void can_write_msg(CAN_DATA_FRAME_STRUCT frames[], uint8_t size)
/* 
short	: write can_messages. Store msg in queue and tx interrupt based
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
*******************************************************************/
{
    uint8_t buf[14];

    // Preproces frames in can chunks
    for( uint8_t idx = 0; idx < size; idx++ ) {
        uint8_t datalen = frames[idx].datalen <= 8 ? frames[idx].datalen : 8;

        // Construct buffer content for extended id
        buf[0] = (uint8_t) ((frames[idx].id << 3) >> 24) ;    // TXBnSIDH
        buf[1] = (uint8_t) ((((frames[idx].id << 3) | (frames[idx].id & 0x00030000)) >> 16) | EXIDE); // TXBnSIDL
        buf[2] = frames[idx].id >> 8;  // TXBnEID8      
        buf[3] = frames[idx].id;       // TXBnEID0
        buf[4] = datalen;              // TXBnDLC and RTR bit clear
    
        // TBnDm
        for(uint8_t idy = 0; idy < datalen; idy++) {
            buf[5+idy] = frames[idx].data[idy];
        }
    
        if( !queue_try_add(&tx_message_queue, buf) ) {
            printf("Error adding to message queue\n");
        }
    }
  
    // Trigger TX0 by writing first one

    // Write to CAN controller
    uint8_t tx_buf_id;
    uint8_t err;
    while( queue_try_remove(&tx_message_queue, buf) )
    {
        bool done = false;
        while (done == false) {
            
            err = 0;

            // Find free transmitbuffer (out of 3)
            uint8_t status = mcp2515_read_status();
            if( !(status & 0x04) ) {
                tx_buf_id = 0x00;
            } else if ( !(status & 0x10) ) {
                tx_buf_id = 0x01;
            } else if ( !(status & 0x40) ) {
                tx_buf_id = 0x02;
            } else {
                err = 1; // No free transmit slot 
            }
            
            if( 0 == err ) {
		
                // 
                mcp2515_load_tx_buffer(tx_buf_id, buf, 13);
    
                // ... and request transmit
                mcp2515_RTS(tx_buf_id);

                done = true;
            }
        }
    }
}


/**************************************************************** */
uint8_t can_tx_extended_data_frame(CAN_DATA_FRAME_STRUCT *frame)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
*******************************************************************/
{
    uint8_t err = 0, tx_buf_id;

    // Find free transmitbuffer (out of 3)
    uint8_t status = mcp2515_read_status();
    if( !(status & 0x04) ) {
        tx_buf_id = 0x00;
    } else if ( !(status & 0x10) ) {
	    tx_buf_id = 0x01;
    } else if ( !(status & 0x40) ) {
    	tx_buf_id = 0x02;
    } else {
    	err = 1; // No free transmit slot 
    }

    //printf("txbuf: %d\n", tx_buf_id);

    // If free transmitterbuffer	
    if( 0 == err ) {
		
        // Temp transmitbuffer and make sure lenght <= 8
        uint8_t buf[14];
        uint8_t datalen = frame->datalen <= 8 ? frame->datalen : 8;

        // Construct buffer content for extended id
        buf[0] = (uint8_t) ((frame->id << 3) >> 24) ;    // TXBnSIDH
        buf[1] = (uint8_t) ((((frame->id << 3) | (frame->id & 0x00030000)) >> 16) | EXIDE); // TXBnSIDL
        buf[2] = frame->id >> 8;        // TXBnEID8      
        buf[3] = frame->id;             // TXBnEID0
        buf[4] = datalen;               // TXBnDLC and RTR bit clear
    
        // TBnDm
        for(uint8_t idx = 0; idx < datalen; idx++) {
            buf[5+idx] = frame->data[idx];
        }
    
        // Write to CAN controller
        mcp2515_load_tx_buffer(tx_buf_id, buf, 13);
    
        // ... and request transmit
        mcp2515_RTS(tx_buf_id);
    }

    return err;
}

/* ************************************************************** */
void can_set_rx_handler( void (*f)(CAN_DATA_FRAME_STRUCT *) )
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    if( f != NULL ) {
        can_rx_fp = f;
    }
}

/* ************************************************************** */
void can_set_tx_handler( void (*f)(CAN_DATA_FRAME_STRUCT *) )
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    if( f != NULL ) {
	can_tx_fp = f;
    }
}

/* ************************************************************** */
void can_set_err_handler( void (*f)(CAN_ERR_FRAME_STRUCT *) )
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    if( f != NULL ) {
	can_err_fp = f;
    }
}


/* ************************************************************** */
void mcp2515_init(uint8_t mode) 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
	
    // Reset CAN controller
    mcp2515_reset();
    sleep_ms(10);

    // Clear masks RX messages
    mcp2515_write_register(RXM0SIDH, 0x00);
    mcp2515_write_register(RXM0SIDL, 0x00);
    mcp2515_write_register(RXM0EID8, 0x00);
    mcp2515_write_register(RXM0EID0, 0x00);

    // Clear filter and EXIDE bit
    mcp2515_write_register(RXF0SIDL, 0x00);

    // Set CNF1 (250kbps)
    mcp2515_write_register(CNF1, 0x00); // 0x00
	
    // Set CNF2 (
    mcp2515_write_register(CNF2, 0xB1); // 0xB8
	
    // Set CNF3 (
    mcp2515_write_register(CNF3, 0x85); // 0x05

    // Interrupts on rx buffers and errors
    mcp2515_write_register(CANINTE, MERRE | ERRIE | RX1IE | RX0IE); 

    // Set NORMAL mode (REQOP_NORMAL or REQOP_LOOPBACK for development)
    mcp2515_write_register(CANCTRL, mode);

    // Verify mode
    uint8_t dummy = mcp2515_read_register(CANSTAT);
    if (mode != (dummy && 0xE0)) {
        printf("Error setting mode!");
        mcp2515_write_register(CANCTRL, mode);
    }
}

/* ************************************************************** */
void mcp2515_callback(uint gpio, uint32_t events)
/* 
short	: Perform callback on CAN RX, TX and Err        
inputs	:        
outputs	: 
notes	: 
Version : DMK, Initial code
***************************************************************** */
{

    uint8_t status = mcp2515_read_register(CANINTF);
    //printf("\tCANINTF: 0x%.2X\n", status);
		
    uint8_t buf[14];

    // Check for RX0 and RX1 interrups
    if( (status & RX0IF) || (status & RX1IF) ) {

        // Check for RX0IF flag -> RX0 buffer contrains CAN message
        if( status & RX0IF ) {
            mcp2515_read_rx_buffer(0, buf, 14);
        } else {
            // Check for RX1IF flag -> RX1 buffer contrains CAN message
            if( status & RX1IF) {
                mcp2515_read_rx_buffer(1, buf, 14);
            } 
        }
        
        if( !queue_try_add(&rx_message_queue, buf) ) {
            printf("Error adding to message queue\n");
        }
    }
}

/* ************************************************************** */
void mcp2515_reset()
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
	uint8_t buf[] = { CAN_RESET };
        
    uint32_t irq_status = save_and_disable_interrupts();
	
    spi_cs_select();
	spi_write_blocking(spi_default, buf, 1);
	spi_cs_deselect();
        
    restore_interrupts(irq_status);
}


/* ************************************************************** */
uint8_t mcp2515_read_register(uint8_t addr)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
	
	uint8_t buf[2];
    buf[0] = CAN_READ;
	buf[1]= addr;
	uint8_t data;

    uint32_t irq_status = save_and_disable_interrupts();
	spi_cs_select();
	spi_write_blocking(spi_default, buf, 2);
	spi_read_blocking(spi_default, 0, &data, 1);
	spi_cs_deselect();
    restore_interrupts(irq_status);
	
	return data;
}

/* *************************************************************** */
void mcp2515_read_rx_buffer(uint8_t buffer_id, uint8_t *data, uint8_t len)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    uint8_t cmd;
    switch(buffer_id) {
        case 0x00:
       	    cmd = 0x90;	
            break;
        case 0x01:
       	    cmd = 0x94;	
            break;
    }
    
    uint32_t irq_status = save_and_disable_interrupts();
    spi_cs_select();
    spi_write_blocking(spi_default, &cmd, 1);
    spi_read_blocking(spi_default, 0, data, len);
    spi_cs_deselect();
    restore_interrupts(irq_status);
}

/* ************************************************************** */
void mcp2515_write_register(uint8_t addr, uint8_t data)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    uint8_t buf[3];
    buf[0] = CAN_WRITE;
    buf[1] = addr;
    buf[2] = data;
        
    uint32_t irq_status = save_and_disable_interrupts();
    spi_cs_select();
    spi_write_blocking(spi_default, buf, 3);
    spi_cs_deselect();
    restore_interrupts(irq_status);
} 


/* ************************************************************** */
void mcp2515_load_tx_buffer(uint8_t buffer_id, uint8_t *data, uint8_t len) 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    uint8_t cmd;
    switch(buffer_id) {
    case 0x00:
        cmd = 0x40;
        break;
    case 0x01:
        cmd = 0x42;
        break;
    case 0x02:
        cmd = 0x44;
        break;
    }

    uint32_t irq_status = save_and_disable_interrupts();
    spi_cs_select();
    spi_write_blocking(spi_default, &cmd, 1);
    spi_write_blocking(spi_default, data, len);
    spi_cs_deselect();
    restore_interrupts(irq_status);
}


/* ************************************************************** */
void mcp2515_RTS(uint8_t tx_buf_id)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{

    uint8_t err = 0;
    uint8_t data = 0x00;

    switch(tx_buf_id) {
	case 0x00:
        data = CAN_RTS_TXB0;
	    break;
	case 0x01:
        data = CAN_RTS_TXB1;
	    break;
	case 0x02:
	    data = CAN_RTS_TXB2;
	    break;
	default:
	    err = 1;
	    printf("\tInvalid tx_buf_id: 0x%.2X\n", tx_buf_id);
    }

    if(!err) {
        uint32_t irq_status = save_and_disable_interrupts();
        spi_cs_select();
	    spi_write_blocking(spi_default, &data, 1);
	    spi_cs_deselect();
        restore_interrupts(irq_status);
    }
}


/* ************************************************************** */
uint8_t mcp2515_read_status(void)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{

    uint8_t txbuf[3];
    txbuf[0] = CAN_RD_STATUS;
    txbuf[1]= 0x00;
    txbuf[2]= 0x00;
    uint8_t rxbuf[2];
	
    uint32_t irq_status = save_and_disable_interrupts();
    spi_cs_select();
    spi_write_blocking(spi_default, txbuf, 3);
    spi_read_blocking(spi_default, 0, rxbuf, 2);
    spi_cs_deselect();
    restore_interrupts(irq_status);
    
    return rxbuf[0];
}


/* ************************************************************** */
uint8_t mcp2515_rx_status(void)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    uint8_t txbuf[3];
    txbuf[0] = CAN_RX_STATUS;
    txbuf[1]= 0x00;
    txbuf[2]= 0x00;
    uint8_t rxbuf[2];
	
    uint32_t irq_status = save_and_disable_interrupts();
    spi_cs_select();
    spi_write_blocking(spi_default, txbuf, 3);
    spi_read_blocking(spi_default, 0, rxbuf, 2);
    spi_cs_deselect();
    restore_interrupts(irq_status);

    return rxbuf[0];
}


/* ************************************************************** */
void mcp2515_bit_modify(uint8_t addr, uint8_t mask, uint8_t data)
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    uint8_t buf[4];
    buf[0] = CAN_BIT_MODIFY;
    buf[1] = addr;
    buf[2] = mask;
    buf[2] = data;
    
    uint32_t irq_status = save_and_disable_interrupts();
    spi_cs_select();
    spi_write_blocking(spi_default, buf, 4);
    spi_cs_deselect();
    restore_interrupts(irq_status);
}

/* ************************************************************** */
static inline void spi_cs_select() 
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0); 
    asm volatile("nop \n nop \n nop");
}

/* ************************************************************** */
static inline void spi_cs_deselect()
/* 
short	:         
inputs	:        
outputs	: 
notes	:         
Version : DMK, Initial code
***************************************************************** */
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

