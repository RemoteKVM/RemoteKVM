#include <linux/serial_reg.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "err.h"
#include "serial.h"
#include "utils.h"

#define IO_READ8(data)* ((uint8_t *) data)
#define IO_WRITE8(data, value) ((uint8_t *) data)[0] = value


static serial_dev_priv_t serial_dev_priv = {
    .iir = UART_IIR_NO_INT,
    .mcr = UART_MCR_OUT2,
    .lsr = UART_LSR_TEMT | UART_LSR_THRE,
    .msr = UART_MSR_DCD | UART_MSR_DSR | UART_MSR_CTS,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
};

/* FIXME: This implementation is incomplete */
static void serial_update_irq(serial_dev_t* s)
{
    serial_dev_priv_t* priv = (serial_dev_priv_t*) s->priv;
    uint8_t iir = UART_IIR_NO_INT;

    /* If enable receiver data interrupt and receiver data ready */
    if ((priv->ier & UART_IER_RDI) && (priv->lsr & UART_LSR_DR))
        iir = UART_IIR_RDI;
    /* If enable transmiter data interrupt and transmiter empty */
    else if ((priv->ier & UART_IER_THRI) && (priv->lsr & UART_LSR_TEMT))
        iir = UART_IIR_THRI;

    __atomic_store_n(&priv->iir, iir | 0xc0, __ATOMIC_RELEASE);

    /* FIXME: the return error of vm_irq_line should be handled */
    vm_irq_line(container_of(s, guest, serial), s->irq_num,
                iir == UART_IIR_NO_INT ? 0 /* inactive */ : 1 /* active */);
}

static int serial_readable(serial_dev_t* s, int timeout)
{
    struct pollfd pollfd = (struct pollfd){
        .fd = s->infd,
        .events = POLLIN,
    };
    return (poll(&pollfd, 1, timeout) > 0) && (pollfd.revents & POLLIN);
}

/* global state to stop the loop of thread */
static volatile bool thread_stop = false;

static void* serial_thread(serial_dev_t* s)
{
    serial_dev_priv_t* priv = (serial_dev_priv_t*) s->priv;
    while (!__atomic_load_n(&thread_stop, __ATOMIC_RELAXED)) {
        if (!serial_readable(s, -1))
            continue;
        pthread_mutex_lock(&priv->lock);
        if (fifo_is_full(&priv->rx_buf)) {
            /* stdin is readable, but the rx_buf is full.
             * Wait for notification.
             */
            pthread_cond_wait(&priv->cond, &priv->lock);
        }
        serial_console(s);
        pthread_mutex_unlock(&priv->lock);
    }

    return NULL;
}

#define TERMINAL_ESCAPE_CHAR 0x01
#define TERMINAL_EXIT_CHAR 'x'

void serial_console(serial_dev_t *s)
{
    serial_dev_priv_t* priv = (serial_dev_priv_t*) s->priv;
    static bool escaped = false;

    while (!fifo_is_full(&priv->rx_buf) && serial_readable(s, 0)) {
        char c;
        if (read(s->infd, &c, 1) == -1)
            break;
        if (escaped && c == TERMINAL_EXIT_CHAR) {
            /* Terminate */
            fprintf(stderr, "\n");
            exit(0);
        }
        if (!escaped && c == TERMINAL_ESCAPE_CHAR) {
            escaped = true;
            continue;
        }
        escaped = false;
        if (!fifo_put(&priv->rx_buf, c))
            break;
        __atomic_store_n(&priv->lsr, priv->lsr | UART_LSR_DR, __ATOMIC_RELEASE);
    }
    serial_update_irq(s);
}

static void serial_in(serial_dev_t* s, uint16_t offset, void* data)
{
    serial_dev_priv_t* priv = (serial_dev_priv_t*) s->priv;
    uint8_t value;

    switch (offset) {
    case UART_RX:
        if (priv->lcr & UART_LCR_DLAB) {
            IO_WRITE8(data, priv->dll);
        } else {
            pthread_mutex_lock(&priv->lock);
            if (fifo_get(&priv->rx_buf, value))
                IO_WRITE8(data, value);

            if (fifo_is_empty(&priv->rx_buf)) {
                priv->lsr &= ~UART_LSR_DR;
                serial_update_irq(s);
            }
            /* The worker thread waits on the condition variable when rx_buf is
             * full and stdin is still readable. Notify the worker thread when
             * the capacity of the buffer drops to its half size, so the worker
             * thread can read up to half of the buffer size before it is
             * blocked again.
             */
            if (fifo_capacity(&priv->rx_buf) == FIFO_LEN / 2)
                pthread_cond_signal(&priv->cond);
            pthread_mutex_unlock(&priv->lock);
        }
        break;
    case UART_IER:
        if (priv->lcr & UART_LCR_DLAB)
            IO_WRITE8(data, priv->dlm);
        else
            IO_WRITE8(data, priv->ier);
        break;
    case UART_IIR:
        value = __atomic_load_n(&priv->iir, __ATOMIC_ACQUIRE);
        IO_WRITE8(data, value | 0xc0); /* 0xc0 stands for FIFO enabled */
        break;
    case UART_LCR:
        IO_WRITE8(data, priv->lcr);
        break;
    case UART_MCR:
        IO_WRITE8(data, priv->mcr);
        break;
    case UART_LSR:
        value = __atomic_load_n(&priv->lsr, __ATOMIC_ACQUIRE);
        IO_WRITE8(data, priv->lsr);
        break;
    case UART_MSR:
        IO_WRITE8(data, priv->msr);
        break;
    case UART_SCR:
        IO_WRITE8(data, priv->scr);
        break;
    default:
        break;
    }
}

static void serial_out(serial_dev_t* s, uint16_t offset, void* data)
{
    serial_dev_priv_t* priv = (serial_dev_priv_t*) s->priv;

    switch (offset) {
    case UART_TX:
        if (priv->lcr & UART_LCR_DLAB) {
            priv->dll = IO_READ8(data);
        } else {
            putchar(((char*) data)[0]);
            fflush(stdout);
            pthread_mutex_lock(&priv->lock);
            priv->lsr |= (UART_LSR_TEMT | UART_LSR_THRE); /* flush TX */
            serial_update_irq(s);
            pthread_mutex_unlock(&priv->lock);
        }
        break;
    case UART_IER:
        if (!(priv->lcr & UART_LCR_DLAB)) {
            pthread_mutex_lock(&priv->lock);
            priv->ier = IO_READ8(data);
            serial_update_irq(s);
            pthread_mutex_unlock(&priv->lock);
        } else {
            priv->dlm = IO_READ8(data);
        }
        break;
    case UART_FCR:
        priv->fcr = IO_READ8(data);
        break;
    case UART_LCR:
        priv->lcr = IO_READ8(data);
        break;
    case UART_MCR:
        priv->mcr = IO_READ8(data);
        break;
    case UART_LSR: /* factory test */
    case UART_MSR: /* not used */
        break;
    case UART_SCR:
        priv->scr = IO_READ8(data);
        break;
    default:
        break;
    }
}


void serial_handle_io(void* owner,
                             void* data,
                             uint8_t is_write,
                             uint64_t offset,
                             uint8_t size)
{
    serial_dev_t* s = (serial_dev_t*) owner;
    void (*serial_op)(serial_dev_t*, uint16_t, void*) =
        is_write ? serial_out : serial_in;

    serial_op(s, offset, data);
}

int serial_init(serial_dev_t *s, bus_t *bus)
{
    *s = (serial_dev_t){
        .priv = (void*) &serial_dev_priv,
        .infd = STDIN_FILENO,
        .irq_num = SERIAL_IRQ,
    };
    pthread_create(&s->worker_tid, NULL, (void*) serial_thread, (void*) s);

    dev_init(&s->dev, COM1_PORT_BASE, COM1_PORT_LEN, s, serial_handle_io);
    bus_register_dev(bus, &s->dev);

    return 0;
}

void serial_exit(serial_dev_t* s)
{
    __atomic_store_n(&thread_stop, true, __ATOMIC_RELAXED);
    pthread_join(s->worker_tid, NULL);
}
