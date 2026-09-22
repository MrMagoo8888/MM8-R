#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include <stddef.h>

#define PCIE_VIRTUAL_BASE                 0xFFFFFFFFC0000000ULL

#define PAGE_PRESENT                      (1ULL << 0)
#define PAGE_WRITE                        (1ULL << 1)
#define PAGE_PWT                          (1ULL << 3)
#define PAGE_PCD                          (1ULL << 4)
#define PAGE_LARGE                        (1ULL << 7)
#define MMIO_PAGE_FLAGS                   (PAGE_PRESENT | PAGE_WRITE | PAGE_PWT | PAGE_PCD)

#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)

typedef struct {
    char signature[8];      // must be "RSD PTR "
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;       // 0 = ACPI 1.0 (RSDT), 2 = ACPI 2.0+ (XSDT)
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;  // 64-bit physical pointer to use
    uint8_t ext_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_t;

typedef struct {
    char signature[4];      // magic signatures - "XSDT" "MCFG" "APIC"
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

typedef struct {
    uint64_t base_address;  // physical addr where PCIe registars map
    uint16_t pci_segment;   // segment group
    uint8_t start_bus;      // lowest bus no
    uint8_t end_bus;        // highest bus no
    uint32_t reserved;      
} __attribute__((packed)) mcfg_entry_t;

int strncmp(const char* s1, const char* s2, size_t n);
acpi_header_t* find_mcfg_table(void* rsdp_phys_ptr, uint64_t virtual_offset);
void initPcie(acpi_header_t* mcfg_header, uint64_t virtual_offset, uint64_t* pml4_virt);
void map_pcie_ecam_2mb(uint64_t* pml4_virt, uint64_t phys_base, uint64_t virt_base, uint64_t size_bytes, uint64_t virtual_offset);
void pcie_enumerate_devices(uint64_t ecam_virt_base, uint8_t start_bus, uint8_t end_bus);
volatile uint32_t* get_pcie_config_addr(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);

// Enable the device's command register bits (memory space, bus master)
void pcie_enable_device(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function);

// Read BAR address and size helpers
uint64_t pcie_get_bar(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);
uint64_t pcie_get_bar_size(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);

// Runtime info populated during MCFG parsing
extern int pcie_mcfg_count;
extern uint64_t pcie_mcfg_base[16];
extern uint8_t pcie_mcfg_start_bus[16];
extern uint8_t pcie_mcfg_end_bus[16];

// Simple device record for enumeration
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint64_t bars[6];
    uint64_t bar_size[6];
    uint64_t bars_virt[6];
} pci_device_t;

// device table populated by enumeration
#define PCIE_MAX_DEVICES 1024
extern pci_device_t pcie_devices[];
extern int pcie_device_count;

// Map a physical MMIO range into virtual memory under kernel-managed region.
void map_mmio_region(uint64_t phys_base, uint64_t virt_base, uint64_t size_bytes);

// Convenience: allocate and map a BAR for a device, returns virt base or 0 on fail
uint64_t pcie_map_bar_for_device(pci_device_t* dev, int bar_index);
// Map arbitrary physical MMIO and return virtual base
uint64_t pcie_map_phys_mmio(uint64_t phys, uint64_t size);

// Set PML4 and virtual offset to allow mapping helpers to allocate page tables
void pcie_set_pml4_and_offset(uint64_t* pml4, uint64_t virtual_offset);

// PCI capability helpers and MSI-X support
int pcie_find_capability(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint8_t cap_id);

// Map and inspect an MSI-X table for a given device. Returns number of table entries (0 on fail).
int pcie_map_msix_table(pci_device_t* dev, uint64_t ecam_virt_base, uintptr_t* out_table_virt, uint32_t* out_table_size_entries);

// Enable MSI-X for a device by programming table entries with vectors allocated from the kernel.
// Returns 0 on success, negative on failure.
int pcie_enable_msix_for_device(pci_device_t* dev, uint64_t ecam_virt_base);

#endif /* PCIE_H */
