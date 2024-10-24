//
// DMA exploration
//

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
#include "math.h"


int dma_data_channel; 
int dma_ctrl_channel;
uint8_t value = 0;

#define FS  44000
#define f1   1000
#define f2   1500
#define BLK_SIZE 176
unsigned short wave_table[BLK_SIZE];

void calc_wave_table() {
    for(uint16_t n = 0; n < BLK_SIZE; n++) {
        wave_table[n] = (unsigned short)(160.0 +
            50.0 * sin(2.0 * 3.1415 * f1 * n / FS) +
            50.0 * sin(2.0 * 3.1415 * f2 * n / FS));
    }
}

//unsigned short src[BLK_SIZE] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
//unsigned short dest[BLK_SIZE] = {9};
unsigned short *address_pointer = &wave_table[0];

static void __isr __time_critical_func(dma_isr)() {
    value += 1;
    
    dma_hw->ints0 = (1u << dma_ctrl_channel);
}

#define PWM_GPIO_OUT    19

int main() {

    stdio_init_all();
    printf("DMA experiment with rp2040\n");

    //
    calc_wave_table();

    // 
    uint f_sys_clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    printf("frequency_count_khz = %d\n", f_sys_clk);

    // pwm
    gpio_set_function(PWM_GPIO_OUT, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_GPIO_OUT);
    pwm_config pc = pwm_get_default_config();
    pwm_config_set_clkdiv(&pc, 2.3191); //
    pwm_config_set_wrap(&pc, 254);
    pwm_init(slice_num, &pc, true);


    dma_data_channel = dma_claim_unused_channel(true);
    dma_ctrl_channel = dma_claim_unused_channel(true);

    // DMA CTRL channel
//    dma_ctrl_channel = dma_claim_unused_channel(true);
    printf("dma_ctrl_channel: %d\n", dma_ctrl_channel);
    dma_channel_config cc  = dma_channel_get_default_config(dma_ctrl_channel);
    channel_config_set_transfer_data_size(&cc, DMA_SIZE_32);
    channel_config_set_read_increment(&cc, false);
    channel_config_set_write_increment(&cc, false);
    channel_config_set_chain_to(&cc, dma_data_channel);
    dma_channel_configure(
        dma_ctrl_channel,
        &cc,
        &dma_hw->ch[dma_data_channel].read_addr,    
        &address_pointer,
        1,
        false
    );
    
    // DMA DATA channel
//    dma_data_channel = dma_claim_unused_channel(true);
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
        &wave_table[0],
        BLK_SIZE,
        false
    );

    // Interrupt
    dma_channel_set_irq0_enabled(dma_ctrl_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    // Delayed start DMA
    sleep_ms(3000);
    dma_start_channel_mask(1u << dma_ctrl_channel);


    while (true) {
//        dma_channel_wait_for_finish_blocking(dma_data_channel);
        printf("[%d] ", value); 
//        for(uint8_t idx = 0; idx < BLK_SIZE; idx++ ) {
//            printf("%.2d ",dest[idx]);
//        }
        printf("\n");
        sleep_ms(250);
        
//        while(1) {
//            tight_loop_contents();
//        }
    }
}
