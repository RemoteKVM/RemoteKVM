#ifndef DEV_H
#define DEV_H

#include <stdint.h>
#include <stddef.h>


/*
this is a function pointer type that is used to handle I/O operations on a device
owner: a pointer to the device's owner
data: a pointer to the data to be read or written
is_write: a flag that indicates whether the operation is a write (1) or a read (0)
offset: the offset from the base address of the device where the operation should take place
size: the size of the data to be read or written
return: void
*/
typedef void (*dev_io_fn)(void* owner, void* data, uint8_t is_write, uint64_t offset, uint8_t size);


typedef struct device 
{
    uint64_t base_addr; // the base address of the device
    uint64_t len;  // the length of the device's storage space
    void* owner; // a pointer to the device's owner
    dev_io_fn do_io; // the function to call when the device is accessed
    struct device* next; // used for linked list
} device_t;


// initializes a device structure
void dev_init(device_t* dev, uint64_t base, uint64_t len, void* owner, dev_io_fn do_io);


#endif // DEV_H