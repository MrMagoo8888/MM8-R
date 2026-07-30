#include "time.h"
#include "stdio.h"

// 64bit
extern volatile uint64_t g_ticks;

#define TIMER_FREQUENCY_HZ 100

void time_initialize(void) {
    printf("Time module initialized.\n");
}

uint64_t get_uptime_ms(void) {
    if (TIMER_FREQUENCY_HZ == 0) return 0;
    return (g_ticks * 1000) / TIMER_FREQUENCY_HZ;
}

uint64_t get_uptime_seconds(void) {
    if (TIMER_FREQUENCY_HZ == 0) return 0;
    return g_ticks / TIMER_FREQUENCY_HZ;
}

bool time_is_periodic(uint64_t* last_ms, uint64_t interval_ms) {
    uint64_t current = get_uptime_ms();
    if (current - *last_ms >= interval_ms) {
        *last_ms = current;
        return true;
    }
    return false;
}

void sleep_ms(uint64_t milliseconds) {
    if (milliseconds == 0) return;

    uint64_t target_ticks_increment = (milliseconds * TIMER_FREQUENCY_HZ) / 1000;
    if (target_ticks_increment == 0 && milliseconds > 0) {
        target_ticks_increment = 1; 
    }
    uint64_t target_ticks = g_ticks + target_ticks_increment;

    while (g_ticks < target_ticks) {
        // needs active, enabled idt
        __asm__ volatile("hlt");
    }
}

void sleep_seconds(uint64_t seconds) {
    sleep_ms(seconds * 1000);
}
