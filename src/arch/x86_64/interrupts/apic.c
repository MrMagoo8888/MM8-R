// largly borrowed from osdev.org/APIC

#include "stdbool.h"
#include "stdint.h"
#include "pcie.h"
#include "stddef.h"

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800
#define CPUID_FEAT_EDX_APIC (1u << 9)

// helpers: cpuid, rdmsr/wrmsr and APIC MMIO access
// forward-declare cpu_get_apic_base
uintptr_t cpu_get_apic_base();

// --- ACPI MADT / IO-APIC discovery and simple routing ---
acpi_header_t* find_madt_table(void* rsdp_phys_ptr, uint64_t virtual_offset) {
   rsdp_t* rsdp = (rsdp_t*)((uint64_t)rsdp_phys_ptr + virtual_offset);
   if (pcie_strncmp(rsdp->signature, "RSD PTR ", 8) != 0) return NULL;
   if (rsdp->revision < 2 || rsdp->xsdt_address == 0) return NULL;

   acpi_header_t* xsdt = (acpi_header_t*)(rsdp->xsdt_address + virtual_offset);
   int entries = (xsdt->length - sizeof(acpi_header_t)) / 8;
   uint64_t* table_pointers = (uint64_t*)((uint64_t)xsdt + sizeof(acpi_header_t));
   for (int i = 0; i < entries; i++) {
      acpi_header_t* table = (acpi_header_t*)(table_pointers[i] + virtual_offset);
      if (pcie_strncmp(table->signature, "APIC", 4) == 0) return table;
   }
   return NULL;
}

// IO-APIC runtime state
static volatile uint32_t* g_ioapic = NULL;
static uint32_t g_ioapic_gsi_base = 0;

static inline uint32_t ioapic_read(uint32_t reg) {
   if (!g_ioapic) return 0xFFFFFFFF;
   g_ioapic[0] = reg;
   return g_ioapic[4]; // IOWIN at offset 0x10 -> index 4 of uint32 array
}

static inline void ioapic_write(uint32_t reg, uint32_t val) {
   if (!g_ioapic) return;
   g_ioapic[0] = reg;
   g_ioapic[4] = val;
}

// Route a global IRQ to a vector on the local APIC.
// delivery: fixed, active low/high not handled here — simple fixed mapping.
void ioapic_route_irq(uint32_t global_irq, uint8_t vector, uint8_t dest_apic_id) {
   if (!g_ioapic) return;
   if (global_irq < g_ioapic_gsi_base) return;
   uint32_t index = global_irq - g_ioapic_gsi_base;
   uint32_t reg_low = 0x10 + index * 2;
   uint32_t reg_high = reg_low + 1;
   // lower dword: vector in bits 0-7, delivery mode fixed (0), polarity/trigger left default, mask=0
   uint32_t low = (uint32_t)vector;
   ioapic_write(reg_low, low);
   // upper dword: destination APIC id in bits 24-31
   uint32_t high = ((uint32_t)dest_apic_id) << 24;
   ioapic_write(reg_high, high);
}

void init_ioapic(acpi_header_t* madt, uint64_t virtual_offset) {
   if (!madt) return;
   uint8_t* start = (uint8_t*)madt + sizeof(acpi_header_t) + 8; // skip local apic addr + flags
   uint8_t* end = (uint8_t*)madt + madt->length;
   for (uint8_t* p = start; p < end; ) {
      uint8_t type = *p;
      uint8_t len = *(p + 1);
      if (type == 1 && len >= 12) {
         uint8_t ioapic_id = *(p + 2);
         // reserved at p+3
            uint32_t ioapic_addr = *(uint32_t*)(p + 4);
            uint32_t gsi_base = *(uint32_t*)(p + 8);
            g_ioapic_gsi_base = gsi_base;
            // map IO-APIC physical addr into virtual using pcie helper
            extern uint64_t pcie_map_phys_mmio(uint64_t phys, uint64_t size);
            uint64_t virt = pcie_map_phys_mmio((uint64_t)ioapic_addr, 0x1000);
            g_ioapic = (volatile uint32_t*)(virt);
         return;
      }
      if (len == 0) break;
      p += len;
   }
}

static void cpuid(uint32_t leaf, uint32_t *out_eax, uint32_t *out_edx) {
   uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
   __asm__ volatile("cpuid"
                : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                : "a" (leaf));
   if (out_eax) *out_eax = eax;
   if (out_edx) *out_edx = edx;
}

static void cpuSetMSR(uint32_t msr, uint32_t eax, uint32_t edx) {
   __asm__ volatile ("wrmsr" :: "c" (msr), "a" (eax), "d" (edx));
}

static void cpuGetMSR(uint32_t msr, uint32_t* eax, uint32_t* edx) {
   uint32_t _a = 0, _d = 0;
   __asm__ volatile ("rdmsr" : "=a" (_a), "=d" (_d) : "c" (msr));
   if (eax) *eax = _a;
   if (edx) *edx = _d;
}

static volatile uint32_t* _lapic_base = NULL;
static inline volatile uint32_t* lapic_base_ensure(void) {
   if (!_lapic_base) {
      uintptr_t base = cpu_get_apic_base();
      _lapic_base = (volatile uint32_t*)base;
   }
   return _lapic_base;
}

static inline uint32_t ReadRegister(uint32_t reg) {
   volatile uint32_t* base = lapic_base_ensure();
   return base[reg >> 2];
}

static inline void write_reg(uint32_t reg, uint32_t value) {
   volatile uint32_t* base = lapic_base_ensure();
   base[reg >> 2] = value;
}

/** returns a 'true' value if the CPU supports APIC
 *  and if the local APIC hasn't been disabled in MSRs
 *  note that this requires CPUID to be supported.
 */
bool check_apic() {
   uint32_t eax, edx;
   cpuid(1, &eax, &edx);
   return (edx & CPUID_FEAT_EDX_APIC) != 0;
}

/* Set the physical address for local APIC registers */
void cpu_set_apic_base(uintptr_t apic) {
   uint32_t edx = 0;
   uint32_t eax = (apic & 0xfffff0000) | IA32_APIC_BASE_MSR_ENABLE;

#ifdef __PHYSICAL_MEMORY_EXTENSION__
   edx = (apic >> 32) & 0x0f;
#endif

   cpuSetMSR(IA32_APIC_BASE_MSR, eax, edx);
}

/**
 * Get the physical address of the APIC registers page
 * make sure you map it to virtual memory ;)
 */
uintptr_t cpu_get_apic_base() {
   uint32_t eax, edx;
   cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);

#ifdef __PHYSICAL_MEMORY_EXTENSION__
   return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
#else
   return (eax & 0xfffff000);
#endif
}

void enable_apic() {
    /* Section 11.4.1 of 3rd volume of Intel SDM recommends mapping the base address page as strong uncacheable for correct APIC operation. */

    /* Hardware enable the Local APIC if it wasn't enabled */
    cpu_set_apic_base(cpu_get_apic_base());

    /* Set the Spurious Interrupt Vector Register bit 8 to start receiving interrupts */
    write_reg(0xF0, ReadRegister(0xF0) | 0x100);
}