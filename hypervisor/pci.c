#include <stdlib.h>
#include <string.h>

#include "pci.h"
#include "utils.h"

/**
 * @brief Handles I/O access to the PCI configuration address register.
 *
 * This function is called when the guest OS attempts to read from or write to
 * the PCI configuration address register (at address 0xCF8). It interprets
 * the data as the address for accessing the PCI configuration space of a specific device.
 *
 * @param owner Pointer to the pci_t structure (PCI bus controller).
 * @param data Pointer to the data being read or written.
 * @param is_write Boolean flag indicating whether the operation is a write (1) or a read (0).
 * @param offset Byte offset within the register.
 * @param size Number of bytes being accessed.
 */
static void pci_address_io(void* owner, void* data, uint8_t is_write, uint64_t offset, uint8_t size)
{
    pci_t* pci = (pci_t*) owner;
    void* p = (void*) ((uintptr_t) &pci->pci_addr + offset);
    /* The data in port 0xCF8 is as an address when Guest Linux accesses the
     * configuration space.
     */
    if (is_write)
    {
        memcpy(p, data, size);
    }
    else
    {
        memcpy(data, p, size);
    }

    // reset because the offset is passed in pci_data_io
    pci->pci_addr.reg_offset = 0;
}

/**
 * @brief Activates a BAR (Base Address Register) for a PCI device.
 *
 * Registers the device's memory or I/O space with the corresponding bus, making it
 * accessible. It also sets the bar_active flag for the BAR.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 * @param bar_num The index of the BAR to activate.
 * @param bus Pointer to the bus_t structure representing the bus to register the BAR to (IO or MMIO bus).
 */
static inline void pci_activate_bar(pci_dev_t* dev, uint8_t bar_num, bus_t* bus)
{
    uint32_t mask = ~(dev->bar_size[bar_num] - 1);
    if (!dev->bar_active[bar_num] && (dev->space_dev[bar_num].base_addr & mask)) //Only activate if not already active and base address is valid
    {
        bus_register_dev(bus, &dev->space_dev[bar_num]);
    }
    dev->bar_active[bar_num] = true;
}

/**
 * @brief Deactivates a BAR for a PCI device.
 *
 * Deregisters the device's memory or I/O space from the corresponding bus, making it
 * inaccessible. It also clears the bar_active flag for the BAR.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 * @param bar_num The index of the BAR to deactivate.
 * @param bus Pointer to the bus_t structure representing the bus to deregister the BAR from (IO or MMIO bus).
 */
static inline void pci_deactivate_bar(pci_dev_t* dev, uint8_t bar_num, bus_t* bus)
{
    uint32_t mask = ~(dev->bar_size[bar_num] - 1);
    if (dev->bar_active[bar_num] && (dev->space_dev[bar_num].base_addr & mask))//Only deactive if bar is active and base address is valid.
    {
        bus_deregister_dev(bus, &dev->space_dev[bar_num]);
    }
    dev->bar_active[bar_num] = false;
}

/**
 * @brief Enables or disables a PCI device's BARs based on the command register.
 *
 * This function checks the PCI command register to determine whether I/O and/or memory
 * access are enabled. It then activates or deactivates each BAR accordingly.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 */
static void pci_command_bar(pci_dev_t* dev)
{
    bool enable_io = dev->hdr.command & PCI_COMMAND_IO;
    bool enable_mem = dev->hdr.command & PCI_COMMAND_MEMORY;

    for (int i = 0; i < PCI_STD_NUM_BARS; i++)
    {
        bus_t* bus = dev->bar_is_io_space[i] ? dev->io_bus : dev->mmio_bus;
        bool enable = dev->bar_is_io_space[i] ? enable_io : enable_mem;

        if (enable)
        {
            pci_activate_bar(dev, i, bus);
        }
        else
        {
            pci_deactivate_bar(dev, i, bus);
        }
    }
}

/**
 * @brief Configures a BAR of a PCI device.
 *
 *  Updates the base address of the specified BAR. This function is called when the BAR register
 *  is written to.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 * @param bar_num The index of the BAR to configure.
 */
static void pci_config_bar(pci_dev_t* dev, uint8_t bar_num)
{
    uint32_t mask = ~(dev->bar_size[bar_num] - 1);
    uint32_t old_bar = dev->hdr.bars[bar_num];
    uint32_t new_bar = (old_bar & mask) | dev->bar_is_io_space[bar_num];

    dev->hdr.bars[bar_num] = new_bar;
    dev->space_dev[bar_num].base_addr = new_bar;
}

/**
 * @brief Handles writes to the PCI configuration space of a device.
 *
 * Writes the provided data to the specified offset in the device's configuration header.
 * If the offset corresponds to the command register, it calls pci_command_bar.
 * If the offset corresponds to a BAR, it calls pci_config_bar to update it.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 * @param data Pointer to the data to write.
 * @param offset Byte offset within the configuration space.
 * @param size Number of bytes to write.
 */
static void pci_config_write(pci_dev_t* dev, void* data, uint64_t offset, uint8_t size)
{
    void* p = (void*) ((uintptr_t) &dev->hdr + offset);
    memcpy(p, data, size);

    if (offset == PCI_COMMAND) 
    {
        pci_command_bar(dev);
    } else if (offset >= PCI_BASE_ADDRESS_0 && offset <= PCI_BASE_ADDRESS_5) 
    {
        uint8_t bar = (offset - PCI_BASE_ADDRESS_0) >> 2;
        pci_config_bar(dev, bar);
    } else if (offset == PCI_ROM_ADDRESS)
    {
        dev->hdr.expansion_rom_base = 0;
    }
    /* TODO: write to capability */
}

/**
 * @brief Handles reads from the PCI configuration space of a device.
 *
 * Reads data from the specified offset in the device's configuration header into the provided data buffer.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 * @param data Pointer to the buffer to store the read data.
 * @param offset Byte offset within the configuration space.
 * @param size Number of bytes to read.
 */
static void pci_config_read(pci_dev_t* dev, void* data, uint64_t offset, uint8_t size)
{
    void* p = (void*) ((uintptr_t) &dev->hdr + offset);
    memcpy(data, p, size);
}

/**
 * @brief Handles I/O access to the PCI configuration space of a device.
 *
 *  This function serves as the interface for I/O access to the PCI configuration space.
 *  It dispatches to the appropriate read or write handler based on the `is_write` flag.
 *
 * @param owner Pointer to the pci_dev_t structure representing the PCI device.
 * @param data Pointer to the data being read or written.
 * @param is_write Boolean flag indicating whether the operation is a write (1) or a read (0).
 * @param offset Byte offset within the configuration space.
 * @param size Number of bytes being accessed.
 */
static void pci_config_do_io(void* owner, void* data, uint8_t is_write, uint64_t offset, uint8_t size)
{
    pci_dev_t* dev = (pci_dev_t*) owner;
    if (is_write)
    {
        pci_config_write(dev, data, offset, size);
    }
    else
    {
        pci_config_read(dev, data, offset, size);
    }
}

/**
 * @brief Handles I/O access to a PCI device's memory/IO space through the data register.
 *
 *  This function is called when the guest OS attempts to access memory/IO space of a
 *  PCI device using the PCI configuration data register (at address 0xCFC).
 *  The address for the access is derived from the PCI address register.
 *
 * @param owner Pointer to the pci_t structure (PCI bus controller).
 * @param data Pointer to the data being read or written.
 * @param is_write Boolean flag indicating whether the operation is a write (1) or a read (0).
 * @param offset Byte offset within the device's memory/IO space.
 * @param size Number of bytes being accessed.
 */
static void pci_data_io(void* owner, void* data, uint8_t is_write, uint64_t offset, uint8_t size)
{
    pci_t* pci = (pci_t*) owner;
    if (pci->pci_addr.enable_bit) 
    {
        uint64_t addr = (pci->pci_addr.value | offset) & ~(PCI_ADDR_ENABLE_BIT);
        bus_handle_io(&pci->pci_bus, data, is_write, addr, size);
    }
}

/**
 * @brief Handles MMIO access to PCI devices directly.
 *
 * This function acts as the I/O handler for a device's MMIO region.
 * It directly forwards the I/O requests to the PCI bus with the provided offset.
 *
 * @param owner Pointer to the pci_t structure (PCI bus controller).
 * @param data Pointer to the data being read or written.
 * @param is_write Boolean flag indicating whether the operation is a write (1) or a read (0).
 * @param offset Byte offset within the device's MMIO space.
 * @param size Number of bytes being accessed.
 */
static void pci_mmio_io(void* owner, void* data, uint8_t is_write, uint64_t offset, uint8_t size)
{
    pci_t* pci = (pci_t*) owner;
    bus_handle_io(&pci->pci_bus, data, is_write, offset, size);
}

/**
 * @brief Configures a BAR for a PCI device.
 *
 * Sets the size and IO/Memory space of a BAR for PCI device and initialize the corresponding device struct
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 * @param bar_num The index of the BAR to configure.
 * @param bar_size The size of the BAR (must be a power of 2).
 * @param is_io_space Boolean flag indicating whether the BAR is in I/O space (1) or memory space (0).
 * @param do_io The I/O handler function for the BAR's memory or I/O region.
 */
void pci_set_bar(pci_dev_t* dev, uint8_t bar_num, uint32_t bar_size, bool is_io_space, dev_io_fn do_io)
{
    /* TODO: mem type, prefetch */
    /* FIXME: bar_size must be power of 2 */
    printf("[SET BAR] bar: %d bar_size: 0x%x is_io_space: %d\n", bar_num, bar_size, is_io_space);
    dev->hdr.bars[bar_num] = is_io_space;
    dev->bar_size[bar_num] = bar_size;
    dev->bar_is_io_space[bar_num] = is_io_space;
    dev_init(&dev->space_dev[bar_num], 0, bar_size, dev, do_io);
}

/**
 * @brief Initializes a pci_dev_t structure.
 *
 * Sets up the basic fields for a PCI device, including linking it to the PCI bus
 * and the I/O and MMIO buses.
 *
 * @param dev Pointer to the pci_dev_t structure to initialize.
 * @param pci Pointer to the pci_t structure representing the PCI bus controller.
 * @param io_bus Pointer to the I/O bus.
 * @param mmio_bus Pointer to the MMIO bus.
 */
void pci_dev_init(pci_dev_t* dev, pci_t* pci, bus_t* io_bus, bus_t* mmio_bus)
{
    memset(dev, 0x00, sizeof(pci_dev_t));
    dev->pci_bus = &pci->pci_bus;
    dev->io_bus = io_bus;
    dev->mmio_bus = mmio_bus;
}

/**
 * @brief Registers a PCI device with the PCI bus.
 *
 *  This function registers the device's configuration space with the PCI bus.
 *
 * @param dev Pointer to the pci_dev_t structure representing the PCI device.
 */
void pci_dev_register(pci_dev_t* dev)
{
    /* FIXEME: It just simplifies the registration on pci bus 0 */
    /* FIXEME: dev_num might exceed 32 */
    union pci_config_address addr = {.dev_num = dev->pci_bus->dev_count};
    dev_init(&dev->config_dev, addr.value, PCI_CFG_SPACE_SIZE, dev, pci_config_do_io);
    bus_register_dev(dev->pci_bus, &dev->config_dev);
}

/**
 * @brief Initializes the PCI bus controller.
 *
 *  This function initializes the PCI bus controller, including the address register,
 *  the data register, and the MMIO region.
 *
 * @param pci Pointer to the pci_t structure to initialize.
 */
void pci_init(pci_t* pci)
{
    dev_init(&pci->pci_addr_dev, PCI_CONFIG_ADDR, sizeof(uint32_t), pci, pci_address_io);
    dev_init(&pci->pci_data_dev, PCI_CONFIG_DATA, sizeof(uint32_t), pci, pci_data_io);
    dev_init(&pci->pci_mmio_dev, 0, PCI_MMIO_SIZE, pci, pci_mmio_io); // FIXME: might be useless because we only support x86
    bus_init(&pci->pci_bus);
}