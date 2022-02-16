#include <kernel/io.h>

inline void outb(uint32_t addr, uint8_t v)
{
    asm volatile("outb %%al, %%dx" :: "d" (addr), "a" (v));
}

inline void outw(uint32_t addr, uint16_t v)
{
    asm volatile("outw %%ax, %%dx" :: "d" (addr), "a" (v));
}

inline void outl(uint32_t addr, uint32_t v)
{
    asm volatile("outl %%eax, %%dx" :: "d" (addr), "a" (v));
}

inline uint8_t inb(uint32_t addr)
{
    uint8_t v;
    asm volatile("inb %%dx, %%al" : "=a" (v) : "d" (addr));
    return v;
}

inline uint16_t inw(uint32_t addr)
{
    uint16_t v;
    asm volatile("inw %%dx, %%ax" : "=a" (v) : "d" (addr));
    return v;
}

inline uint32_t inl(uint32_t addr)
{
    uint32_t v;
    asm volatile("inl %%dx, %%eax" : "=a" (v) : "d" (addr));
    return v;
}

inline void io_wait(void)
{
    asm volatile("jmp 1f\n"
                 "1:jmp 2f\n"
                 "2:");
}
