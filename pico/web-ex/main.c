/* *************************************************

Web server example/test 
 
(c) dkroeske@gmail.com

v1.0    Initial code

***************************************************/


#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"

#include "lwip/apps/httpd.h"
#include "lwip/netif.h"

// function prototyping
void cgi_init(void);
const char* cgi_handler_cb(int index, int numParams, char* params[], char* values[]); 

// Global var
static const tCGI cgi_handlers[] = {
    {
        "/leds.cgi", cgi_handler_cb
    }
};


/* ****************************************************** */
int main() {
/*
 * 
 * notes   : mainloop
 * version : DMK, initial code
 ******************************************************** */

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
        // Check volgende regel in documentatie.
        cyw43_wifi_pm(&cyw43_state, cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 20, 1, 1, 1));
        printf("IP = %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    }

    if(!err) {
        httpd_init();
        cgi_init();
    }

        
    uint8_t idx = 0;
    while (true) {

        // Do something useless
        printf("0x%.2X\n", idx++);
        sleep_ms(500);
        
    }
}

void cgi_init(void){
    http_set_cgi_handlers(cgi_handlers, 1);
}

const char* cgi_handler_cb(int index, int numParams, char* params[], char* values[]) {

    printf("index %d, numParams %d\n", index, numParams);
        
    return "/cgi.html";
}
