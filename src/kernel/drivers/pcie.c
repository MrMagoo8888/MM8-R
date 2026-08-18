#include "pcie.h"
#include "stdint.h"
#include "string.h"
#include "stddef.h"
#include "memory.h"

static int pcie_strncmp(const char* s1, const char* s2, size_t n) {
    while (n > 0) {
        if (*s1 != *s2) {
            return (int)((unsigned char)*s1 - (unsigned char)*s2);
        }
        if (*s1 == '\0') {
            break;
        }
        s1++;
        s2++;
        n--;
    }
    return 0;
}

static uint64_t allocate_physical_frame_with_offset(uint64_t virtual_offset) {
    void* ptr = memory_alloc_pages(1);
    if (ptr == NULL) {
        while(1) { __asm__ volatile("cli; hlt"); }
    }
    memset((void*)((uint64_t)ptr + virtual_offset), 0, 4096);
    return (uint64_t)ptr;
}

acpi_header_t* find_mcfg_table(void* rsdp_phys_ptr, uint64_t virtual_offset) {
    rsdp_t* rsdp = (rsdp_t*)((uint64_t)rsdp_phys_ptr + virtual_offset);

    // verify sig
    if (pcie_strncmp(rsdp->signature, "RSD PTR ", 8) != 0) {
        return NULL;
    }

    // ensure there is an XSDT avaliable (and revision >=2)
    if (rsdp->revision < 2 || rsdp->xsdt_address == 0) {
        return NULL;
    }

    acpi_header_t* xsdt = (acpi_header_t*)(rsdp->xsdt_address + virtual_offset);

    // calc table entries - total table size subtract header size and dived by 8 byte pointer chunks
    int entries = (xsdt->length - sizeof(acpi_header_t)) / 8;
    uint64_t* table_pointers = (uint64_t*)((uint64_t)xsdt + sizeof(acpi_header_t));

    for (int i = 0; i < entries; i++) {
        acpi_header_t* table = (acpi_header_t*)(table_pointers[i] + virtual_offset);

        if (pcie_strncmp(table->signature, "MCFG", 4) == 0) {
            return table;
        }
    }

    return NULL;
}

void map_pcie_ecam_2mb(uint64_t* pml4_virt, uint64_t phys_base, uint64_t virt_base, uint64_t size_bytes, uint64_t virtual_offset) {
    uint64_t virt_end = virt_base + size_bytes;
    uint64_t current_phys = phys_base;

    for (uint64_t v = virt_base; v < virt_end; v += 0x200000) {
        uint64_t pml4_idx = PML4_INDEX(v);
        if (!(pml4_virt[pml4_idx] & PAGE_PRESENT)) {
            uint64_t new_frame = allocate_physical_frame_with_offset(virtual_offset);
            pml4_virt[pml4_idx] = new_frame | PAGE_PRESENT | PAGE_WRITE;
        }
        uint64_t* pdpt_virt = (uint64_t*)((pml4_virt[pml4_idx] & ~0xFFF) + virtual_offset);

        uint64_t pdpt_idx = PDPT_INDEX(v);
        if (!(pdpt_virt[pdpt_idx] & PAGE_PRESENT)) {
            uint64_t new_frame = allocate_physical_frame_with_offset(virtual_offset);
            pdpt_virt[pdpt_idx] = new_frame | PAGE_PRESENT | PAGE_WRITE;
        }
        uint64_t* pd_virt = (uint64_t*)((pdpt_virt[pdpt_idx] & ~0xFFF) + virtual_offset);

        uint64_t pd_idx = PD_INDEX(v);
        pd_virt[pd_idx] = current_phys | MMIO_PAGE_FLAGS | PAGE_LARGE;

        current_phys += 0x200000;
    }

    __asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
}

void initPcie(acpi_header_t* mcfg_header, uint64_t virtual_offset, uint64_t* pml4_virt) {

    //take away header size and 8 reserved bytes to calc array bounds
    int total_entries = (mcfg_header->length - sizeof(acpi_header_t) - 8) / sizeof(mcfg_entry_t);
    mcfg_entry_t* entries = (mcfg_entry_t*)((uint64_t)mcfg_header + sizeof(acpi_header_t) + 8);

    for (int i = 0; i < total_entries; i++) {
        uint64_t ecam_phys_base = entries[i].base_address;
        uint8_t start_bus = entries[i].start_bus;
        uint8_t end_bus = entries[i].end_bus;

        uint32_t total_buses = (end_bus - start_bus) + 1;
        uint64_t mapping_size = (uint64_t)total_buses * 1024 * 1024;
        uint64_t ecam_virt_base = PCIE_VIRTUAL_BASE + ((uint64_t)start_bus * 1024 * 1024);

        map_pcie_ecam_2mb(pml4_virt, ecam_phys_base, ecam_virt_base, mapping_size, virtual_offset);
        pcie_enumerate_devices(ecam_virt_base);
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

// calc mem addr for pcie device config space
volatile uint32_t* get_pcie_config_addr(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    return (volatile uint32_t*)(ecam_virt_base + 
                               (((uint64_t)bus << 20) | 
                               ((uint64_t)device << 15) | 
                               ((uint64_t)function << 12) | 
                               (offset & 0xFFF)));
}

void pcie_enumerate_devices(uint64_t ecam_virt_base) {
    // loop all 256 possible busses
    for (uint16_t bus = 0; bus < 256; bus++) {
        // loop all 32 possiple device per bus
        for (uint8_t dev = 0; dev < 32; dev++) {
            
            // read Offset 0x00 to get Vendor ID (lower 16bits) and Device ID (upper 16 bits)
            volatile uint32_t* reg0 = get_pcie_config_addr(ecam_virt_base, bus, dev, 0, 0x00);
            uint32_t id_reg = *reg0;
            uint16_t vendor_id = (uint16_t)(id_reg & 0xFFFF);

            // 0xFFFF means hardware is absent or not responding on this slot
            if (vendor_id == 0xFFFF) {
                continue;
            }

            // read Offset 0x0C to inspect header type (determines if it's multi-function)
            volatile uint32_t* reg3 = get_pcie_config_addr(ecam_virt_base, bus, dev, 0, 0x0C);
            uint8_t header_type = (uint8_t)((*reg3 >> 16) & 0xFF);
            
            // ff bit7 of header type is set - this is a multi-function device (up to 8 functions)
            uint8_t max_functions = (header_type & 0x80) ? 8 : 1;

            // iterate over the funcs of device
            for (uint8_t func = 0; func < max_functions; func++) {
                volatile uint32_t* func_reg0 = get_pcie_config_addr(ecam_virt_base, bus, dev, func, 0x00);
                uint32_t func_id = *func_reg0;
                
                if ((uint16_t)(func_id & 0xFFFF) == 0xFFFF) {
                    continue; // skip inactive sub-functions
                }

                // read: Class Code, Subclass - and Prog IF from Offset 0x08
                volatile uint32_t* reg2 = get_pcie_config_addr(ecam_virt_base, bus, dev, func, 0x08);
                uint32_t class_reg = *reg2;
                uint8_t class_code = (uint8_t)(class_reg >> 24);
                uint8_t subclass   = (uint8_t)((class_reg >> 16) & 0xFF);

                // identification exapmle
                // if Class == 0x03 and Subclass == 0x00, it's a VGA Compatible Graphics Controller
                if (class_code == 0x03 && subclass == 0x00) {
                    // This matches VBE framebuffer hardware target
                    // Log or handle graphic acceleration setup here
                }
            }
        }
    }
}


// call this enable when device is found that is used
void pcie_enable_device(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function) {

    // command reg is lower 16bits of offset 0x04
    volatile uint32_t* cmd_status_reg = get_pcie_config_addr(ecam_virt_base, bus, device, function, 0x04);
    uint32_t value = *cmd_status_reg;

    uint16_t command = (uint16_t)(value & 0xFFFF);

    // bit1 mem space enable (allows device to respond to BAR MMIO)
    // bit2 bus master enable (allows device to proform DMA transfere to sys ram)
    command |= (1 << 1) | (1 << 2);     // these bitwise oporators are so funky lol

    // write back updated command reg whilst leaving status reg (upper 16 rember) the same
    *cmd_status_reg = (value & 0xFFFF0000) | command;
}


// 64bit bar addr parsr 
// discover mem ranges map
// dis is stored in base address registers (bars) at offset 0x10
// pcie devices often nowadays use 64bit bar footprints - consuming two consecutive 32bit bar register slots
// understand? me neither
// letsa go

uint64_t pcie_get_bar(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) {

    // bar pos's range from index 0 through 5 (offsets 0x10 0x14 0x18 0x1C 0x20 0x24)
    uint16_t offset = 0x10 + (bar_index * 4);

    // i know a guy who knows a guy who knows a guy..
    volatile uint32_t* bar_low_ptr = get_pcie_config_addr(ecam_virt_base, bus, device, function, offset);
    uint32_t bar_low = *bar_low_ptr;

    // bit0 == 1 - legecy I/O space BAR (practically deprecated but who knows what will be pugged in)
    if ((bar_low >> 1) == 0) {
        //bit2:1 indicates architcture type
        // 0x00 - 32bit adress space mappin
        // 0x02 - 64bit
        
        if (((bar_low >> 1) & 3) == 0x02) {
            volatile uint32_t* bar_high_ptr = get_pcie_config_addr(ecam_virt_base, bus, device, function, offset + 4);
            uint32_t bar_high = *bar_high_ptr;

            // combine up and low regs masking out lower 4bits of metadata flgs
            return ((uint64_t)bar_high << 32) | (bar_low & 0xFFFFFFFFFFFFFFF0ULL);
        }

        // 32bit mem bar path fallback
        return (bar_low & 0xFFFFFFF0);
    }

    // legacy i/o space path fallback (mask low 2 bits)
    return (bar_low & 0xFFFFFFFC);

}


uint64_t pcie_get_bar_size(uint64_t ecam_virt_base, uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) {
    uint16_t offset = 0x10 + (bar_index * 4);
    volatile uint32_t* bar_low_ptr = get_pcie_config_addr(ecam_virt_base, bus, device, function, offset);
    uint32_t og_low = *bar_low_ptr;

    // check if mem space otr legcy io space b'cause bit0 == 0 means mem, bit0 == 1 means legio
    uint32_t mask_flags = (og_low & 1) ? 0xFFFFFFFC : 0xFFFFFFF0; // crazy new syntax just dropped, clonditional - expression
    uint8_t is_64_bit = ((og_low & 1) == 0) && (((og_low >> 1) & 3) == 0x02);

    // is mem space
    if (is_64_bit) {
        volatile uint32_t* bar_high_ptr = get_pcie_config_addr(ecam_virt_base, bus, device, function, offset + 4);
        uint32_t og_high = *bar_high_ptr;

        // write 1s to all bits in both registers
        *bar_low_ptr = 0xFFFFFFFF;
        *bar_high_ptr = 0xFFFFFFFF;

        // read back hardware response
        uint32_t response_low = *bar_low_ptr;
        uint32_t response_high = *bar_high_ptr;

        // restone og vals
        *bar_low_ptr = og_low;
        *bar_high_ptr = og_high;

        // calc 64bit
        uint64_t combined_result = ((uint64_t)response_high << 32) | (response_low & mask_flags);
        if (combined_result == 0) {
            return 0;
        }
        return (~combined_result) + 1;
    } else {
        // 32bit io bar
        *bar_low_ptr = 0xFFFFFFFF;
        uint32_t response_low = *bar_low_ptr;
        *bar_low_ptr = og_low;

        uint32_t masked_response = response_low & mask_flags;
        if (masked_response == 0) {
            return 0;
        }
        return (uint64_t)((~masked_response) + 1);
    }

}

/*
Plan:
Get MCFG parsing working
Map ECAM region into virtual memory
Enumerate all buses and devices
Detect bridges and recurse
Read and size every BAR correctly
Assign resources
Enable memory space and bus mastering
Route interrupts
Register drivers per device type
Then add device-specific init
*/