#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>

void outb(uint32_t addr, uint8_t v);  /* byte */
void outw(uint32_t addr, uint16_t v); /* word */
void outl(uint32_t addr, uint32_t v); /* dword */

uint8_t inb(uint32_t addr);
uint16_t inw(uint32_t addr);
uint32_t inl(uint32_t addr);

#endif /* end of include guard: __IO_H__ */
