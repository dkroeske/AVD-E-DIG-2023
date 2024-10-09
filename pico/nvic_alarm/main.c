#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

// Globale variablen
uint16_t counter_value = 0;

// Defines

// Functieprototypes
int64_t alarm_callback(alarm_id_t id, void *data);

// Interrupts Service Routine (ISR) alarm
int64_t alarm_callback(alarm_id_t id, void *data){
    printf("timer fired!\n");
    counter_value++;
    return 0;
}

// Main loop, wordt onderbroken bij een interrupt (in dit geval IO interrupt)
int main() {

    stdio_init_all();

    printf("Alarm interrupt example\n");

    add_alarm_in_ms(5000, alarm_callback, NULL, false);

    // While forever
    while (true) {
        printf("countervalue = %d\n", counter_value);
        sleep_ms(500);
    }
}
