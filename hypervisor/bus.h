#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "dev.h"

typedef struct bus 
{
    uint64_t dev_count; // the number of devices on the bus
    device_t* head; // the first device on the bus
} bus_t;

void bus_register_dev(bus_t* bus, device_t* dev); // add the given device to the bus
void bus_deregister_dev(bus_t* bus, device_t* dev); // remove a device from the bus
void bus_handle_io(bus_t* bus, void* data, uint8_t is_write, uint64_t addr, uint8_t size); // call the do_io function of the device that matches the given address
void bus_init(bus_t* bus); // initialize the bus

#endif // BUS_H