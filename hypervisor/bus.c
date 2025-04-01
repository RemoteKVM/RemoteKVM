#include "bus.h"
#include <stddef.h>

/*
this function find the divice on the given bus that matches the given address
bus: a pointer to the bus
addr: the address to search for
return: a pointer to the device that matches the address, or NULL if no device is found
*/
static inline device_t* bus_find_dev(bus_t* bus, uint64_t addr)
{
    device_t* curr_dev = bus->head; // get the first device on the bus
    while (curr_dev) // iterate through the devices on the bus
    {
        // check if the address is within the range of the device (from the base address to the base address + length - 1)
        if (addr >= curr_dev->base_addr && addr <= curr_dev->base_addr + curr_dev->len - 1) 
        {
            return curr_dev; 
        }
        curr_dev = curr_dev->next;
    }
    return NULL;
}


/**
* @brief this function call the do_io function of the device that matches the given address
* bus: a pointer to the bus
* data: a pointer to the data to be read or written
* is_write: a flag that indicates whether the operation is a write (1) or a read (0)
* addr: the address to access
size: the size of the data to be read or written
return: void
*/
void bus_handle_io(bus_t* bus, void* data, uint8_t is_write, uint64_t addr, uint8_t size)
{
    device_t* dev = bus_find_dev(bus, addr);

    // if a device is found and the address (and the size to be wirten) is within the device's range
    if (dev && addr + size - 1 <= dev->base_addr + dev->len - 1) 
    {
        dev->do_io(dev->owner, data, is_write, addr - dev->base_addr, size);
    }
}

// add the given device to the bus
void bus_register_dev(bus_t* bus, device_t* dev)
{
    dev->next = bus->head;
    bus->head = dev;
    bus->dev_count++;
}

//remove a device from the bus
void bus_deregister_dev(bus_t* bus, device_t* dev)
{
    device_t** p = &bus->head;

    while (*p != dev && *p) 
    {
        p = &(*p)->next;
    }

    if (*p)
    {
        *p = (*p)->next;
    }

    bus->dev_count--; // omer added this line, maybe it is not correct.
}

// initialize the bus
void bus_init(bus_t* bus)
{
    bus->dev_count = 0;
    bus->head = NULL;
}