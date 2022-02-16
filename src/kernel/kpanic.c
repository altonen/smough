#include <kernel/compiler.h>
#include <arch/amd64/cpu.h>
#include <kernel/kpanic.h>

void __noreturn kpanic(const char *error)
{
    disable_irq();

    while (1) { }
    __builtin_unreachable();
}
