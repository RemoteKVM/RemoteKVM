#include "vm.h"

void run_vm(guest* g)
{
    // get the VCPU's memory size and mmap it
    size_t vcpu_mmap_size = ioctl(g->kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size < 0) 
    {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        return;
    }

    struct kvm_run* run = mmap(NULL, vcpu_mmap_size, 
                               PROT_READ | PROT_WRITE, 
                               MAP_SHARED, g->vcpu_fd, 0);
    if (run == MAP_FAILED) 
    {
        perror("mmap vcpu");
        return;
    }
    printf("VCPU memory mapped successfully.\n");

    // Set up debugging
    struct kvm_guest_debug debug = {
        .control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP,
    };
    if (ioctl(g->vcpu_fd, KVM_SET_GUEST_DEBUG, &debug) < 0) {
        perror("KVM_SET_GUEST_DEBUG");
        return;
    }

     // run the virtual CPU
    printf("Starting the virtual CPU...\n");
    printf("test\n");
    while (1) {
        int err = ioctl(g->vcpu_fd, KVM_RUN, 0);
        if (err < 0 && (errno != EINTR && errno != EAGAIN)) {
            munmap(run, vcpu_mmap_size);
            perror("Failed to execute kvm_run");
            return;
        }
        // check the exit reason
        switch (run->exit_reason) {
            case KVM_EXIT_HLT:
                printf("KVM_EXIT_HLT\n");
                return;
            case KVM_EXIT_IO:
            {
                uint64_t addr = run->io.port;
                void *data = (void *) ((uintptr_t) run + run->io.data_offset);
                bool is_write = run->io.direction == KVM_EXIT_IO_OUT;
                for (int i = 0; i < run->io.count; i++) 
                {
                    bus_handle_io(&g->io_bus, data, is_write, addr, run->io.size);
                    addr += run->io.size;
                }
                break;
            }

            case KVM_EXIT_MMIO:
            {
                bus_handle_io(&g->mmio_bus, run->mmio.data, run->mmio.is_write,
                                run->mmio.phys_addr, run->mmio.len);
                break;
            }
            
            case KVM_EXIT_DEBUG:
            {
                // print_debug_info(g);
                break;
            }

            case KVM_EXIT_INTERNAL_ERROR:
            {
                printf("KVM_EXIST_INTERNAL_ERROR. suberror 0x%x\n", run->internal.suberror);
                struct kvm_regs debug_regs;
                print_debug_info(g);
                break;
            }
            case KVM_EXIT_SHUTDOWN:
            {
                printf("Shutting down.\n");
                return;
            } 
            default:
                printf("Unhandled KVM exit reason: %d\n", run->exit_reason);
                print_debug_info(g);
                return;
        }
    }
}

void setup_vm(guest* g)
{
    // Open the KVM device
    g->kvm_fd = open("/dev/kvm", O_RDWR | __O_CLOEXEC);
    if (g->kvm_fd < 0) 
    {
        perror("open /dev/kvm");
        return;
    }
    printf("Opened /dev/kvm successfully.\n");

    // Check for guest memory extension
    if (!ioctl(g->kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY))
    {
        perror("KVM_CAP_USER_MEM extension is not available");
    }
    printf("KVM_CAP_USER_MEM extension is available.\n");

    // Create a new virtual machine
    g->vm_fd = ioctl(g->kvm_fd, KVM_CREATE_VM, 0);
    if (g->vm_fd < 0) 
    {
        perror("KVM_CREATE_VM");
        return;
    }
    printf("Virtual machine created successfully.\n");

    ioctl(g->vm_fd, KVM_SET_TSS_ADDR, TSS_ADDRESS); // required for intel virtualization
    ioctl(g->vm_fd, KVM_SET_IDENTITY_MAP_ADDR, 0); // also required, 0 causes it to default to address 0xfffbc000
    ioctl(g->vm_fd, KVM_CREATE_IRQCHIP, 0);
    ioctl(g->vm_fd, KVM_CREATE_PIT2, 0); // needs to be after the irq chip is created
    struct kvm_pit_config pit = { .flags = 0 };
    ioctl(g->vm_fd, KVM_CREATE_PIT2, &pit);

    // Allocate memory for the guest
    g->mem = mmap(NULL, GUEST_MEMORY_SIZE,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 
                              -1, 0);
    if (g->mem == MAP_FAILED) 
    {
        perror("mmap guest_memory");
        return;
    }
    printf("Guest memory allocated successfully.\n");

    // set up the guest memory mapping
    struct kvm_userspace_memory_region mem_region = {
        .slot = 0,
        .guest_phys_addr = 0,
        .memory_size = GUEST_MEMORY_SIZE,
        .userspace_addr = (uint64_t)g->mem,
    };
    if (ioctl(g->vm_fd, KVM_SET_USER_MEMORY_REGION, &mem_region) < 0) 
    {
        perror("KVM_SET_USER_MEMORY_REGION");
        return;
    }
    printf("Guest memory mapped successfully.\n");

    // create a virtual CPU
    g->vcpu_fd = ioctl(g->vm_fd, KVM_CREATE_VCPU, 0);
    if (g->vcpu_fd < 0) 
    {
        perror("KVM_CREATE_VCPU");
        return;
    }
    printf("Virtual CPU created successfully.\n");

    init_regs(g);
    init_cpuid(g);
    init_msrs(g);

    bus_init(&g->io_bus);
    bus_init(&g->mmio_bus);

    // init pci
    pci_init(&g->pci);
    bus_register_dev(&g->io_bus, &g->pci.pci_addr_dev);
    bus_register_dev(&g->io_bus, &g->pci.pci_data_dev);
}

void init_regs(guest* g)
{
    // set up the VCPU's initial state (program counter, etc.)
    struct kvm_sregs sregs;
    if (ioctl(g->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) 
    {
        perror("KVM_GET_SREGS");
        return;
    }
    printf("VCPU special registers retrieved successfully.\n");

    sregs.cs.base = 0;
    sregs.cs.limit = ~0;
    sregs.cs.g = 1;

    sregs.ds.base = 0;
    sregs.ds.limit = ~0;
    sregs.ds.g = 1;

    sregs.fs.base = 0;
    sregs.fs.limit = ~0;
    sregs.fs.g = 1;

    sregs.gs.base = 0;
    sregs.gs.limit = ~0;
    sregs.gs.g = 1;

    sregs.es.base = 0;
    sregs.es.limit = ~0;
    sregs.es.g = 1;

    sregs.ss.base = 0;
    sregs.ss.limit = ~0;
    sregs.ss.g = 1;

    sregs.cs.db = 1;
    sregs.ss.db = 1;
    sregs.cr0 |= 1; // enable protected mode

    if (ioctl(g->vcpu_fd, KVM_SET_SREGS, &sregs) < 0) 
    {
        perror("KVM_SET_SREGS");
        return;
    }
    printf("VCPU special registers set successfully.\n");

    struct kvm_regs regs = {
        .rflags = FLAGS_INIT, // required by x86
        .rip = KERNEL_START,
        .rsi = BOOT_PARAMS_START
    };

    if (ioctl(g->vcpu_fd, KVM_SET_REGS, &regs) < 0) 
    {
        perror("KVM_SET_REGS");
        return;
    }
    printf("VCPU general registers set successfully.\n");
}


// needed by the kernel to know if it runs on a vm or bare metal
void init_cpuid(guest* g)
{
    struct {
        uint32_t nent;
        uint32_t padding;
        struct kvm_cpuid_entry2 entries[100];
    } kvm_cpuid;

    kvm_cpuid.nent = sizeof(kvm_cpuid.entries) / sizeof(kvm_cpuid.entries[0]);
    ioctl(g->kvm_fd, KVM_GET_SUPPORTED_CPUID, &kvm_cpuid);
    
    for (uint32_t i = 0; i < kvm_cpuid.nent; i++) {
        struct kvm_cpuid_entry2 *entry = &kvm_cpuid.entries[i];

        // sets response to the string "KVMKVMKVM"
        if (entry->function == KVM_CPUID_SIGNATURE) 
        {
            entry->eax = KVM_CPUID_FEATURES;
            entry->ebx = 0x4b4d564b; // KVMK
            entry->ecx = 0x564b4d56; // VMKV
            entry->edx = 0x4d;       // M
        }
    }
    ioctl(g->vcpu_fd, KVM_SET_CPUID2, &kvm_cpuid);
}

#define MSR_IA32_MISC_ENABLE 0x000001a0
#define MSR_IA32_MISC_ENABLE_FAST_STRING_BIT 0
#define MSR_IA32_MISC_ENABLE_FAST_STRING \
    (1ULL << MSR_IA32_MISC_ENABLE_FAST_STRING_BIT)

#define KVM_MSR_ENTRY(_index, _data) \
    (struct kvm_msr_entry) { .index = _index, .data = _data }
void init_msrs(guest* g)
{
    int ndx = 0;
    struct kvm_msrs *msrs =
        calloc(1, sizeof(struct kvm_msrs) + (sizeof(struct kvm_msr_entry) * 1));

    msrs->entries[ndx++] =
        KVM_MSR_ENTRY(MSR_IA32_MISC_ENABLE, MSR_IA32_MISC_ENABLE_FAST_STRING);
    msrs->nmsrs = ndx;

    ioctl(g->vcpu_fd, KVM_SET_MSRS, msrs);

    free(msrs);
}

int load_image(struct guest *g, const char* image_path) 
{
    size_t datasz;
    void *data;
    int fd = open(image_path, O_RDONLY);
    if (fd < 0) 
    {
        return 1;
    }

    struct stat st;
    fstat(fd, &st);
    datasz = st.st_size;
    data = mmap(0, datasz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    struct boot_params* boot = (struct boot_params *)(((uint8_t *)g->mem) + BOOT_PARAMS_START);
    void* cmdline = (void *)(((uint8_t *)g->mem) + CMD_LINE_START);
    void* kernel = (void *)(((uint8_t *)g->mem) + KERNEL_START);

    memset(boot, 0, sizeof(struct boot_params));
    memmove((void *) ((uintptr_t) boot + offsetof(struct boot_params, hdr)),
            (void *) ((uintptr_t) data + offsetof(struct boot_params, hdr)),
            sizeof(struct setup_header));
    
    size_t setup_sectors = boot->hdr.setup_sects;
    size_t setupsz = (setup_sectors + 1) * 512;
    boot->hdr.vid_mode = 0xFFFF; // VGA
    boot->hdr.type_of_loader = 0xFF;
    boot->hdr.loadflags |= CAN_USE_HEAP | LOADED_HIGH | KEEP_SEGMENTS;
    boot->hdr.heap_end_ptr = 0xFE00;
    boot->hdr.ext_loader_ver = 0x0;
    boot->hdr.cmd_line_ptr = CMD_LINE_START;
    memset(cmdline, 0, boot->hdr.cmdline_size);
    memcpy(cmdline, KERNEL_OPTIONS, sizeof(KERNEL_OPTIONS));
    memmove(kernel, (char *)data + setupsz, datasz - setupsz);

    unsigned int idx = 0;
    boot->e820_table[idx++] = (struct boot_e820_entry){
        .addr = 0x0,
        .size = ISA_START_ADDRESS - 1,
        .type = E820_RAM,
    };
    boot->e820_table[idx++] = (struct boot_e820_entry){
        .addr = ISA_END_ADDRESS,
        .size = GUEST_MEMORY_SIZE - ISA_END_ADDRESS,
        .type = E820_RAM,
    };
    boot->e820_entries = idx;

    return 0;
}

void load_initrd(guest* g, const char* initrd_path)
{
    int fd = open(initrd_path, O_RDONLY);
    if (fd < 0)
        return;

    struct stat st;
    fstat(fd, &st);
    size_t datasz = st.st_size;
    void * data = mmap(0, datasz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    struct boot_params *boot = (struct boot_params *) ((uint8_t *) g->mem + BOOT_PARAMS_START);
    unsigned long addr = boot->hdr.initrd_addr_max & ~0xfffff;

    while (addr > GUEST_MEMORY_SIZE - datasz)
    {
        if (addr < KERNEL_START)
        {
            return perror("Not enough memory for initrd");
        }
        addr -= KERNEL_START;
    }

    void *initrd = ((uint8_t *) g->mem) + addr;

    memset(initrd, 0, datasz);
    memmove(initrd, data, datasz);

    boot->hdr.ramdisk_image = addr;
    boot->hdr.ramdisk_size = datasz;
    munmap(data, datasz);
}



void print_debug_info(guest* g)
{
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    
    if (ioctl(g->vcpu_fd, KVM_GET_REGS, &regs) < 0) {
        perror("KVM_GET_REGS");
        return;
    }
    
    if (ioctl(g->vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return;
    }

    printf("Debug info:\n");
    printf("RIP: 0x%llx, RAX: 0x%llx, RBX: 0x%llx\n", regs.rip, regs.rax, regs.rbx);
    printf("RCX: 0x%llx, RDX: 0x%llx\n", regs.rcx, regs.rdx);
    printf("RSI: 0x%llx, RDI: 0x%llx\n", regs.rsi, regs.rdi);
    printf("RBP: 0x%llx, RSP: 0x%llx\n", regs.rbp, regs.rsp);
    printf("RFLAGS: 0x%llx\n", regs.rflags);
    printf("CR0: 0x%llx, CR3: 0x%llx, CR4: 0x%llx\n", sregs.cr0, sregs.cr3, sregs.cr4);
    printf("EFER: 0x%llx\n", sregs.efer);

       // Check CPU mode
    if (sregs.cr0 & X86_CR0_PE) {
        printf("CPU is in Protected Mode\n");
        if (sregs.cr0 & X86_CR0_PG) {
            printf("Paging is enabled\n");
            if (sregs.cr4 & X86_CR4_PAE) {
                printf("Physical Address Extension (PAE) is enabled\n");
            }
            if (sregs.efer & X86_EFER_LME) {
                printf("Long Mode is enabled\n");
                if (sregs.efer & X86_EFER_LMA) {
                    printf("CPU is in 64-bit mode\n");
                } else {
                    printf("CPU is in compatibility mode\n");
                }
            } else {
                printf("CPU is in 32-bit mode\n");
            }
        } else {
            printf("CPU is in 32-bit protected mode without paging\n");
        }
    } else {
        printf("CPU is in Real Mode\n");
    }

    // Dump the first few bytes at the instruction pointer
    printf("Memory at RIP:\n");
    for (int i = 0; i < 16; i += 4) {
        uint32_t *mem = (uint32_t *)((char *)g->mem + regs.rip + i);
        printf("0x%llx: %08x\n", regs.rip + i, *mem);
    }
}