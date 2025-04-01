#ifndef GUEST_H
#define GUEST_H

#include <linux/kvm.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "serial_dev.h"
#include "bus.h"
#include "pci.h"
#include "virtio-blk.h"
#include "diskimg.h"

typedef struct guest {
    int kvm_fd;
    int vm_fd;
    int vcpu_fd;
    void* mem;
    struct serial_dev serial;
    bus_t io_bus;
    bus_t mmio_bus;
    pci_t pci;
    struct virtio_blk_dev virtio_blk_dev;
    struct diskimg diskimg;
} guest;

int vm_irq_line(guest* v, int irq, int level);

#endif // GUEST_H