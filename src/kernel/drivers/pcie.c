#include "pcie.h"
#include "stdint.h"
#include "string.h"
#include "stddef.h"

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

acpi_header_t* find_mcfg_table(void* rsdp_phys_ptr, uint64_t virtual_offset) {
    rsdp_t* rsdp = (rsdp_t*)((uint64_t)rsdp_phys_ptr + virtual_offset);

    // verify sig
    if (strncmp(rsdp->signature, "RSD PTR", 8) != 0) {
        return NULL;
    }

    // ensure there is an XSDT avaliable (and revision >=2)
    if (rsdp->revision < 2 || rsdp->xsdt_address == 0) {
        retun NULL;
    }

    acpi_header_t* xsdt = (acpi_header_t*)(rsdp->xsdt_address + virtual_offset);

    // calc table entries - total table size subtract header size and dived by 8 byte pointer chunks
    int entries = (xsdt->length - sizeof(acpi_header_t)) / 8;
    uint64_t* table_pointers = (uint64_t)((uint64_t)xsdt + sizeof(acpi_header_t));

    for (int i = 0; 1 < entries; i++) {
        acpi_header_t* table = (acpi_header_t*)(table_pointers[i] + virtual_offset);

        if (strncmp(table->signature, "MCFG", 4) == 0) {
            return table;
        }
    }

    return NULL;


}

// parse MCFG alloc structur

typedef struct {
    uint64_t base_address;  // physical addr where PCIe registars map
    uint16_t pci_segment;   // segment group
    uint8_t start_bus;      // lowest bus no
    uint8_t end_bus;        // highest bus no
    uint32_t reserved;      
} __attribute__((packed)) mcfg_entry_t;

void initPcie(acpi_header_t* mcfg_header, uint64_t virtual_offset) {

    //take away header size and 8 reserved bytes to calc array bounds
    int total_entries = (mcfg_header->length - sizeof(acpi_header_t) - 8) / sizeof(mcfg_entry_t);
    mcfg_entry_t* entries = (mcfg_entry_t*)((uint64_t)mcfg_header + sizeof(acpi_header_t) + 8);

    for (int i = 0; i < total_entries; i++) {
        uint64_t ecam_phys_base = entries[i].base_address;
        uint8_t start_bus = entries[i].start_bus;
        uint8_t end_bus = entries[i].end_bus;
    }

    // TODO: when print or kprint works - please add this to store stuff
    // kprint("PCIe base: %p, Busses: %d to %d\n", ecam_phys_base, start_bus, end_bus);

    // map into page tables
    //map_pcie_ecam_space(ecam_phys_base, start_bus, end_bus, virtual_offset);

    // needs to be mapped PML4 -> PDPT -> PD -> PT
    // each bus needs 1mb of virtual addreses
    // for default alloc that spans 0 - 255 map 256 mb of contigous space
    // flags to protect against cpu state corruptio0n:
    // present(bit0) -> 1
    // read/write(bit1) -> 1
    // chache disable(pcd, bit4) -> 1 // MMIO prevents cpu caching hardware register states
    // write through(PWT, bit3) -> 1 


}