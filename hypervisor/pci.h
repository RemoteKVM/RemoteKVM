#pragma once

#include <linux/kvm.h>
#include <linux/pci_regs.h>
#include <stdbool.h>
#include <stdint.h>
#include "bus.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_MMIO_SIZE (1UL << 16)
#define PCI_STD_NUM_BARS 6
#define PCI_CFG_HDR_SIZE 64
#define PCI_ADDR_ENABLE_BIT (1UL << 31)

typedef union pci_config_address
{
    struct
    {
        unsigned reg_offset : 2;
        unsigned reg_num : 6;
        unsigned func_num : 3;
        unsigned dev_num : 5;
        unsigned bus_num : 8;
        unsigned reserved : 7;
        unsigned enable_bit : 1;
    };
    uint32_t value;
} pci_config_addres_t;

#pragma pack(push, 1) // Ensure no padding is added by the compiler

union class
{
    struct 
    {
        uint8_t revision_id;       // Offset 0x08
        uint8_t prog_if;           // Offset 0x09
        uint8_t subclass;          // Offset 0x0A
        uint8_t class_code;        // Offset 0x0B
    };
    uint32_t class_id;
};


/*
most fields are not used but i thought its better than just having huge reserved chunks
this is a type 0 header
*/
typedef struct pci_config_hdr {
    uint16_t vendor_id;        // Offset 0x00
    uint16_t device_id;        // Offset 0x02
    uint16_t command;          // Offset 0x04
    uint16_t status;           // Offset 0x06
    union class class_id;
    uint8_t cache_line_size;   // Offset 0x0C
    uint8_t latency_timer;     // Offset 0x0D
    uint8_t header_type;       // Offset 0x0E
    uint8_t bist;              // Offset 0x0F
    uint32_t bars[6];           // Offsets 0x10 to 0x27 (Base Address Registers)
    uint32_t cardbus_cis_ptr;  // Offset 0x28
    uint16_t subsystem_vendor_id; // Offset 0x2C
    uint16_t subsystem_id;     // Offset 0x2E
    uint32_t expansion_rom_base; // Offset 0x30
    uint8_t capabilities_ptr;  // Offset 0x34
    uint8_t reserved[7];       // Offsets 0x35 to 0x3B
    uint8_t interrupt_line;    // Offset 0x3C
    uint8_t interrupt_pin;     // Offset 0x3D
    uint8_t min_grant;         // Offset 0x3E
    uint8_t max_latency;       // Offset 0x3F
    uint8_t caps_space[PCI_CFG_SPACE_SIZE - PCI_CFG_HDR_SIZE];
} pci_config_hdr_t;

#pragma pack(pop) // Restore default padding

typedef struct pci_dev 
{
    pci_config_hdr_t hdr;
    uint32_t bar_size[PCI_STD_NUM_BARS]; // already stored in space_dev
    bool bar_active[PCI_STD_NUM_BARS];
    bool bar_is_io_space[PCI_STD_NUM_BARS];
    device_t space_dev[PCI_STD_NUM_BARS];
    device_t config_dev;
    struct bus *io_bus;
    struct bus *mmio_bus;
    struct bus *pci_bus;
} pci_dev_t;

typedef struct pci 
{
    union pci_config_address pci_addr;
    bus_t pci_bus;
    device_t pci_data_dev;
    device_t pci_addr_dev;
    device_t pci_mmio_dev;
} pci_t;

void pci_set_bar(struct pci_dev *dev, uint8_t bar, uint32_t bar_size, bool is_io_space, dev_io_fn do_io);
void pci_set_status(struct pci_dev *dev, uint16_t status);
void pci_dev_register(struct pci_dev *dev);
void pci_dev_init(struct pci_dev *dev, struct pci *pci, struct bus *io_bus, struct bus *mmio_bus);
void pci_init(struct pci *pci);
