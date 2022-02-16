#ifndef __AMD64_ASM_H__
#define __AMD64_ASM_H__

#include <kernel/compiler.h>

typedef struct task task_t;

void gdt_flush(void *ptr);
void idt_flush(void *ptr);
void tss_flush(int index);

#endif /* __AMD64_ASM_H__ */
