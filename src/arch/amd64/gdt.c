#include <arch/amd64/asm.h>
#include <drivers/lapic.h>
#include <kernel/compiler.h>
#include <kernel/gdt.h>
#include <kernel/percpu.h>

static uint64_t GDT[5 + 2 * 64] = { 0 };

static          struct gdt_ptr_t gdt_ptr;
static __percpu struct tss_ptr_t tss_ptr;

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

void tss_init(void)
{
    struct tss_ptr_t *tss = get_thiscpu_ptr(tss_ptr);

    uint64_t tss_base = (uint64_t)tss;
    uint64_t tss_size = sizeof(struct tss_ptr_t);
    unsigned cpu_id   = lapic_get_init_cpu_count() - 1; // TODO: `get_thiscpu_id()`?

    // in 64-bit mode, the TSS descriptors are 16 bytes instead of 8.
    // (see Intel manual chapter 7.2.3 for more details)
    GDT[5 + 2 * cpu_id + 1] = (tss_base >> 32) & 0xffffffff; // base     32 - 63
    GDT[5 + 2 * cpu_id + 0] = 0x0000e90000000000   |         // options
        (tss_size         &   0x000000000000ffff)  |         // limit    0  - 16
        ((tss_base << 16) &   0x000000ffffff0000)  |         // base     0  - 23
        ((tss_size << 32) &   0x000f000000000000)  |         // limit    16 - 19
        ((tss_base << 32) &   0xff00000000000000);           // base     24 - 31

    tss_flush(40 + cpu_id * 2 * 8);
    put_thiscpu_ptr(tss);
}
