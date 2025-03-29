/* *********************************************************

audio_pwm.c 
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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "pwm_audio.h"


int dma_data_channel; 
int dma_ctrl_channel;


#define PWM_AUDIO_BUF_SIZE  8192

uint8_t wave_table[PWM_AUDIO_BUF_SIZE] = {0,};
uint8_t dbuf[PWM_AUDIO_BUF_SIZE] = {0,};
unsigned int buf_index = 0;
bool new_data = false;
uint8_t repeat_value = 0;

static void __isr __time_critical_func(pwm_audio_isr)() {
    
    // if new data copy to buf
    if(new_data) {
        memcpy(dbuf, wave_table, PWM_AUDIO_BUF_SIZE * sizeof(uint8_t));
        new_data = false;
        buf_index = 0;
    }

    // clear pwm isr
    pwm_clear_irq(pwm_gpio_to_slice_num(PWM_RIGHT_GPIO_OUT));
    // set pwm level
    pwm_set_gpio_level(PWM_RIGHT_GPIO_OUT, dbuf[buf_index]);
    
    // update buffer index. increase after 16 times
    repeat_value++;
    repeat_value%=32;
    if(!repeat_value) {
        buf_index++;
        buf_index%=PWM_AUDIO_BUF_SIZE;   
    }
}

void pwm_audio_write(uint8_t *buf, size_t l)
{
    memcpy(wave_table, buf, l);
    new_data = true;
}

void test_data() {
    for(uint16_t idx = 0; idx < PWM_AUDIO_BUF_SIZE; idx++) {
        dbuf[idx] = (uint8_t) ((80 * 254) / 100);
    }
}


/* ****************************************************** */
void pwm_audio_init()
/*
short   : 
inputs  : 
outputs :
notes   :
***********************************************************/
{

#ifdef PWM_AUDIO_DEBUG
    printf("PWM_AUDIO init. Buffer size %u. PWM_CLKDIV %f\n", sizeof(dbuf), PWM_CLKDIV);
    test_data();
#endif

    // 
//    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
//    printf("frequency_count_khz = %d\n", f_sys_clk);

    // pwm
    gpio_set_function(PWM_RIGHT_GPIO_OUT, GPIO_FUNC_PWM);
    gpio_set_drive_strength(PWM_RIGHT_GPIO_OUT, GPIO_DRIVE_STRENGTH_12MA);
    uint slice_num = pwm_gpio_to_slice_num(PWM_RIGHT_GPIO_OUT);
    pwm_config pc = pwm_get_default_config();
    pwm_config_set_clkdiv(&pc, PWM_CLKDIV);
    pwm_config_set_wrap(&pc, PWM_WRAP);
    pwm_init(slice_num, &pc, true);

    // set pwm irq    
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_audio_isr);
    irq_set_enabled(PWM_IRQ_WRAP, true);

}


/* ****************************************************** */
void pwm_audio_start()
/*
short   : Start PWM audio playback
inputs  : 
outputs :
notes   : 
***********************************************************/
{
}
