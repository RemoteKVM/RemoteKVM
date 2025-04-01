#include "guest.h"

int vm_irq_line(guest* v, int irq, int level)
{
    struct kvm_irq_level irq_level = {
        {.irq = irq},
        .level = level,
    };

    if (ioctl(v->vm_fd, KVM_IRQ_LINE, &irq_level) < 0)
    {
        printf("Failed to set the status of an IRQ line");
        return -1;
    }

    return 0;
}
