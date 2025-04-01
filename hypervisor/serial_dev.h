#ifndef SERIAL_DEV_H
#define SERIAL_DEV_H

#include <pthread.h>
#include "bus.h"

#define COM1_PORT_BASE 0x03f8
#define COM1_PORT_LEN 8
#define SERIAL_IRQ 4

typedef struct serial_dev serial_dev_t;

struct serial_dev {
    void* priv;
    pthread_t worker_tid;
    int infd; /* file descriptor for serial input */
    device_t dev;
    int irq_num;
};

#endif // SERIAL_DEV_H