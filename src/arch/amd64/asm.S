.code64
.section .text
.global gdt_flush
.global tss_flush
.global idt_flush

gdt_flush:
    lgdt (%rdi)
    pushq $8
    pushq $flush
    lretq
flush:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    ret

idt_flush:
    lidt (%rdi)
    ret

tss_flush:
    movq %rdi, %rax
    ltr %ax
    ret
