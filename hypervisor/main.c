#include "serial.h"
#include "bus.h"
#include "guest.h"
#include "vm.h"

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        return 1;
    }

    guest vm;

    setup_vm(&vm);
    serial_init(&vm.serial, &vm.io_bus);

    if (load_image(&vm, argv[1]) != 0) {
        printf("Error loading image - Check if the image path is correct\n");
        return 1;
    }

    if (diskimg_init(&vm.diskimg, argv[2]) < 0)
    {
        printf("Error initializing disk image.\n");
        return -1;
    }
    virtio_blk_init_pci(&vm.virtio_blk_dev, &vm.diskimg, &vm.pci, &vm.io_bus, &vm.mmio_bus);
    load_initrd(&vm, INITRD_PATH);


    run_vm(&vm);

    close(vm.kvm_fd);
    close(vm.vm_fd);
    close(vm.vcpu_fd);
    munmap(vm.mem, GUEST_MEMORY_SIZE);

    return 0;
}