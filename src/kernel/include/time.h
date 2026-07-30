#ifndef TIME_H
#define TIME_H

#include "stdint.h"
#include "stdbool.h"

void time_initialize(void);
uint64_t get_uptime_ms(void);
uint64_t get_uptime_seconds(void);
bool time_is_periodic(uint64_t* last_ms, uint64_t interval_ms);
void sleep_ms(uint64_t milliseconds);
void sleep_seconds(uint64_t seconds);

#endif
