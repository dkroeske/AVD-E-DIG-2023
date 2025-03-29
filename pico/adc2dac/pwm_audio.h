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

#define PWM_CLK             125000000
#define PWM_M               16
#define PWM_WRAP            254
#define PWM_FS              11000    
#define PWM_CLKDIV  ((float)(PWM_CLK))/(PWM_M*PWM_WRAP*PWM_FS)

/* ****************************************************** */
void pwm_audio_write(uint8_t *buf, size_t l);
/*
short   : 
inputs  : 
outputs :
notes   :
***********************************************************/

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
