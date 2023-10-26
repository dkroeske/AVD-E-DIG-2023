/* *************************************************

UDP example
 
(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/netif.h"

// Main loop
int main() {

    //
    stdio_init_all();

    // Setup wifi
    if( cyw43_arch_init() ) {
        printf("cyw43_arch_init() error\n");
    }

    // STA mode = connect to access point
    cyw43_arch_enable_sta_mode();
    if( cyw43_arch_wifi_connect_timeout_ms(
        "ssid",
        "password",
        CYW43_AUTH_WPA2_AES_PSK,
        30000)
    ){
        printf("cyw43_arch_init() error\n");
    } else {
        printf("IP = %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    }
       
    while(true){        
        sleep_ms(200);
    }
}

