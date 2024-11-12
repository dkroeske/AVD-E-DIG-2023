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
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "pwm_audio.h"


int dma_data_channel; 
int dma_ctrl_channel;


//#define FS      44000
//#define f1      1000
//#define f2   1500
//#define BLK_SIZE 176

#define PWM_AUDIO_BUF_SIZE  1024


unsigned short wave_table[PWM_AUDIO_BUF_SIZE] = {0,};
unsigned short dbuf[PWM_AUDIO_BUF_SIZE] = {0,};

bool new_data = false;

// precalc memcpy 
size_t num = PWM_AUDIO_BUF_SIZE * sizeof(unsigned short);

static void __isr __time_critical_func(dma_isr)() {
    
    if(new_data) {
        memcpy(dbuf, wave_table, num);
        new_data = false;
    }

    dma_hw->ints0 = (1u << dma_ctrl_channel);
}


void pwm_audio_write(unsigned short *buf, size_t l)
{
    memcpy(wave_table, buf, l);
    new_data = true;
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
    printf("PWM_AUDIO init. Buffer size %u\n", num);
#endif

    // 
    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("frequency_count_khz = %d\n", f_sys_clk);

    // pwm
    gpio_set_function(PWM_RIGHT_GPIO_OUT, GPIO_FUNC_PWM);
    gpio_set_drive_strength(PWM_RIGHT_GPIO_OUT, GPIO_DRIVE_STRENGTH_12MA);
    uint slice_num = pwm_gpio_to_slice_num(PWM_RIGHT_GPIO_OUT);
    pwm_config pc = pwm_get_default_config();
    pwm_config_set_clkdiv(&pc, 2.3191);
    pwm_config_set_wrap(&pc, 254);
    pwm_init(slice_num, &pc, true);

    // dma
//    dma_data_channel = dma_claim_unused_channel(true);
//    dma_ctrl_channel = dma_claim_unused_channel(true);

    // DMA CTRL channel

    dma_ctrl_channel = dma_claim_unused_channel(true);
    dma_channel_config cc  = dma_channel_get_default_config(dma_ctrl_channel);
    channel_config_set_transfer_data_size(&cc, DMA_SIZE_32);
    channel_config_set_read_increment(&cc, false);
    channel_config_set_write_increment(&cc, false);
    channel_config_set_chain_to(&cc, dma_data_channel);
    dma_channel_configure(
        dma_ctrl_channel,
        &cc,
        &dma_hw->ch[dma_data_channel].read_addr,    
        dbuf,
        1,
        false
    );

    
    // DMA DATA channel
    dma_data_channel = dma_claim_unused_channel(true);
    printf("dma_data_channel: %d\n", dma_data_channel);
    dma_channel_config dc  = dma_channel_get_default_config(dma_data_channel);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_16);
    channel_config_set_read_increment(&dc, true);
    channel_config_set_write_increment(&dc, false);
    dma_timer_set_fraction(0, 0x17, 0xFFFF);     // N/D * sys_clk (125MHz)
    channel_config_set_dreq(&dc, 0x3b);          // DREQ paced by timer 0
    channel_config_set_chain_to(&dc, dma_ctrl_channel);

    dma_channel_configure(
        dma_data_channel,
        &dc,
        &pwm_hw->slice[slice_num].cc,
        dbuf,
        PWM_AUDIO_BUF_SIZE,
        false
    );

    // Interrupt
    dma_channel_set_irq1_enabled(dma_ctrl_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_isr);
    irq_set_enabled(DMA_IRQ_1, true);
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
    dma_start_channel_mask(1u << dma_ctrl_channel);
}
