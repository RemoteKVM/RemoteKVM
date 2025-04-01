#ifndef SERIAL_H
#define SERIAL_H

#include <linux/kvm.h>
#include <pthread.h>

#include "bus.h"
#include "guest.h"
#include "serial_dev_priv.h"
#include "serial_dev.h"

#define COM1_PORT_BASE 0x03f8
#define COM1_PORT_LEN 8
#define SERIAL_IRQ 4


void serial_console(serial_dev_t* s);
int serial_init(serial_dev_t* s, bus_t* bus);
void serial_exit(serial_dev_t* s);
void serial_handle_io(void* owner,
                             void* data,
                             uint8_t is_write,
                             uint64_t offset,
                             uint8_t size);

#endif // SERIAL_H