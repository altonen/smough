#ifndef __IRQ_H__
#define __IRQ_H__

#include <kernel/common.h>

#define VECNUM_SPURIOUS   0xff
#define VECNUM_IRQ_START  0x20
#define VECNUM_TIMER      0x20
#define VECNUM_KEYBOARD   0x21
#define VECNUM_SYSCALL    0x80
#define VECNUM_PAGE_FAULT 0x0e
#define VECNUM_GPF        0x0d

enum {
    IRQ_HANDLED   =  0,
    IRQ_UNHANDLED = -1,
};

void irq_init(void);
void irq_install_handler(int num, uint32_t (*handler)(void *), void *ctx);
void irq_uninstall_handler(int num, uint32_t (*handler)(void *));

#endif /* __IRQ_H__ */
