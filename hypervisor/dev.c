#include "dev.h"

// initializes a device structure
void dev_init(device_t* dev, uint64_t base, uint64_t len, void* owner, dev_io_fn do_io)
{
    dev->base_addr = base;
    dev->len = len;
    dev->owner = owner;
    dev->do_io = do_io;
    dev->next = NULL;
}