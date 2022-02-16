#include <arch/amd64/asm.h>
#include <kernel/compiler.h>
#include <kernel/gdt.h>

static uint64_t GDT[5 + 2 * 64] = { 0 };
static struct gdt_ptr_t gdt_ptr;

void gdt_init(void)
{
    static int once;

    if (!once) {
        GDT[0] = 0;
        GDT[1] = 0x00a0980000000000; /* kernel code */
        GDT[2] = 0x00c0920000000000; /* kernel data */
        GDT[3] = 0x00a0f80000000000; /* user code */
        GDT[4] = 0x00c0f20000000000; /* user data */

        gdt_ptr.limit = sizeof(GDT) - 1;
        gdt_ptr.base  = (uint64_t)GDT;
        once 		  = 1;
    }

    gdt_flush(&gdt_ptr);
}
