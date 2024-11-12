/* *********************************************************

audio_pwm.h 
Playback audio via the Cytron Maker PI audio interface.
See schematic Maker PI hardware. Audio is passive bandpass
filtered with DC blocking.
Audio playback using DMA

Connection

PICO        Cytron hardware
---------------------------
GPIO #18    Audio LEFT    
GPIO #19    Audio RIGHT

***********************************************************/

#ifndef PWM_AUDIO_INC
#define PWM_AUDIO_INC

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"



#define PWM_LEFT_GPIO_OUT   18
#define PWM_RIGHT_GPIO_OUT  19

#define PWM_AUDIO_DEBUG 1


//#define FS      44000
//#define f1      1000
//#define f2   1500
//#define BLK_SIZE 176

//unsigned short *address_pointer = &wave_table[0];
void pwm_audio_write(unsigned short *buf, size_t l);

/* ****************************************************** */
void pwm_audio_init();
/*
short   : 
inputs  : 
outputs :
notes   :
***********************************************************/

/* ****************************************************** */
void pwm_audio_start();
/*
short   : Start PWM audio playback
inputs  : 
outputs :
notes   : 
***********************************************************/

#endif 
