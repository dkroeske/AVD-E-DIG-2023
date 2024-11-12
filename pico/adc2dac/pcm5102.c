/* *********************************************************

pcm5102.c 
Driver for the pcm5102 DAC. Using the PCM5102MK2 2.0 board

Connection

PICO    PCM5102MK2
---------------------
GPIO #26    BCK
GPIO #27    LRCK
GPIO #28    DATA
GND(28)     GND
GND(23)     GND
VBUS(1)     VCC - 5V??


***********************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
//#include "hardware/clocks.h"
//#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
//#include "pcm5102.h"






