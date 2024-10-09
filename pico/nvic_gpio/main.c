#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

// Functieprototypes
void gpio_isr();

// Globale variablen
uint16_t counter_value = 0;

// Defines
#define PUSHBUTTON  22

// Interrupts Service Routine (ISR) gpio
void gpio_isr() {
    counter_value += 1;
}

// Main loop, wordt onderbroken bij een interrupt (in dit geval IO interrupt)
int main() {

    stdio_init_all();

    printf("GPIO interrupt example\n");

    // Init GPIO interrupt, let op de EDGE_RISING en de functie pointer
    gpio_set_irq_enabled_with_callback(PUSHBUTTON, GPIO_IRQ_EDGE_RISE, true, &gpio_isr);

    // Init GPIO port
    gpio_init(PUSHBUTTON);
    gpio_set_dir(PUSHBUTTON, GPIO_IN);

    // While forever
    while (true) {
        printf("countervalue = %d\n", counter_value);
        sleep_ms(500);
    }
}
