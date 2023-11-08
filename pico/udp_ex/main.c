/* *************************************************

UDP example icm ws2812 leds
 
(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "artnet.h"
#include "ws2812.h"

#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/netif.h"

// Main loop
int main() {

    //
    stdio_init_all();

    bool err = false;

    // Setup wifi
    if( cyw43_arch_init() ) {
        printf("cyw43_arch_init() error\n");
        err = true;
    }

    // STA mode = connect to access point
    if(!err) {
        cyw43_arch_enable_sta_mode();
        if( cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            30000)
        ){
            err = true;
            printf("cyw43_enable_sta_mode error\n");
        }
    }

    if(!err) {
        printf("IP = %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        ws2812_init();
        initArtNet(NULL);
    }

    while(true){        
        sleep_ms(200);
    }
}

