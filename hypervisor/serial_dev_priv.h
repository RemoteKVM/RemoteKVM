#ifndef SERIAL_DEV_PRIV_H
#define SERIAL_DEV_PRIV_H

#include <pthread.h>
#include <stdint.h>
#include "utils.h"

typedef struct serial_dev_priv {
    uint8_t dll;
    uint8_t dlm;
    uint8_t iir;
    uint8_t ier;
    uint8_t fcr;
    uint8_t lcr;
    uint8_t mcr;
    uint8_t lsr;
    uint8_t msr;
    uint8_t scr;

    struct fifo rx_buf;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} serial_dev_priv_t;

#endif // SERIAL_DEV_PRIV_H