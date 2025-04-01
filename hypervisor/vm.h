#ifndef VM_H
#define VM_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>
#include <linux/kvm_para.h>
#include <string.h>
#include <asm/bootparam.h>
#include <sys/stat.h>
#include <asm/processor-flags.h>
#include <errno.h>
#include <stddef.h>
#include <asm/e820.h>
#include <stdbool.h>

#include "guest.h"
#include "pci.h"

// Can't be changed
#define MAP_ANONYMOUS 0x20
#define FLAGS_INIT (1 << 1)
#define KERNEL_START 0x100000
#define BOOT_PARAMS_START 0x10000
#define CMD_LINE_START 0x20000

// Can be changed
#define GUEST_MEMORY_SIZE (1 << 30)  // 2 Mb
#define TSS_ADDRESS 0xffffd000
#define KERNEL_OPTIONS "console=ttyS0 pci=conf1"
#define INITRD_PATH "rootfs.cpio"

// x86 flags
#define X86_EFER_LME (1<<8) // Long Moede Enabled
#define X86_EFER_LMA (1<<10) // Long Mode Active


void run_vm(guest* g);
void setup_vm(guest* g);
void init_regs(guest* g);
void init_cpuid(guest* g);
void init_msrs(guest* g);
int load_image(struct guest *g, const char* image_path); // reutrns 0 on success
void load_initrd(guest* g, const char* initrd_path);
void print_debug_info(guest* g);

#endif // VM_H
