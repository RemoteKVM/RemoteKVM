#ifndef UTILS_H
#define UTILS_H

#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *) (ptr);                \
        ((type *) (__mptr - offsetof(type, member))); \
    })

#define FIFO_LEN 64
#define FIFO_MASK (FIFO_LEN - 1)

struct fifo {
    unsigned int data[FIFO_LEN];
    unsigned int head, tail;
};

#define fifo_is_empty(fifo) ((fifo)->head == (fifo)->tail)

#define fifo_is_full(fifo) ((fifo)->tail - (fifo)->head > FIFO_MASK)

#define fifo_capacity(fifo) ((fifo)->tail - (fifo)->head)

#define fifo_put(fifo, value)                               \
    ({                                                      \
        unsigned int __ret = !fifo_is_full(fifo);           \
        if (__ret) {                                        \
            (fifo)->data[(fifo)->tail & FIFO_MASK] = value; \
            (fifo)->tail++;                                 \
        }                                                   \
        __ret;                                              \
    })

#define fifo_get(fifo, value)                               \
    ({                                                      \
        unsigned int __ret = !fifo_is_empty(fifo);          \
        if (__ret) {                                        \
            value = (fifo)->data[(fifo)->head & FIFO_MASK]; \
            (fifo)->head++;                                 \
        }                                                   \
        __ret;                                              \
    })

#endif
