#ifndef APIC_H
#define APIC_H

#include <stdint.h>
#include <stdbool.h>

// check if CPU/APIC is present and enabled
bool check_apic(void);

// i wonder
void cpu_set_apic_base(uintptr_t apic_phys_base);
uintptr_t cpu_get_apic_base(void);

// read the func dummy
void enable_apic(void);

#endif // APIC_H
