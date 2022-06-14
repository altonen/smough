.section .trampoline
.code16

# APs start in 16-bit real mode and they must be switched to
# 32-bit protected mode before jumping to boot code.
#
# The switch to 32-bit address space is needed because the boot code
# is located at 0x100000 which is something a 16-bit pointer can't hold.

_trampoline_entry:
    cli

    movw %cs,  %ax
    movw %ax,  %ds
    movw %ax,  %es
    movw %ax,  %fs
    movw %ax,  %gs
    movw %ax,  %ss

    # enable a20
    inb $0x92, %al
    orb $0x2, %al
    andb $0xfe, %al
    outb %al, $0x92

    # get base from %cs, multiply it by 4 to get base address (0x55000), then add the relative
    # address of the temporary GDT table to that address to get the actual address of the GDT.
    # this is then loaded using lgdtl which allows the CPU to enter to protected mode
    mov %cs, %ebx
    shl $4, %ebx
    addl $0 + gdt_start - _trampoline_entry, %ebx
    movl %ebx, (gdt_ptr_ - _trampoline_entry + 2)

    lgdtl (gdt_ptr_ - _trampoline_entry)

    # enable protected mode and disable paging
    mov %cr0, %eax
    or $0x1, %eax
    and $0x7fffffff, %eax
    mov %eax, %cr0

    ljmpl $0x8, $_trampoline_32

error:
    hlt
    jmp error

# null, code and data
.align 0x10
gdt_start:
    .quad 0x0000000000000000

    # kernel code
    .word   0xffff
    .word   0x0000
    .byte   0x00
    .byte   0x9a
    .byte   0xcf
    .byte   0x00

    # kernel data
    .word   0xffff
    .word   0x0000
    .byte   0x00
    .byte   0x92
    .byte   0xcf
    .byte   0x00
gdt_ptr_:
    .short 23
    .long gdt_start

.code32
_trampoline_32:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    mov $0x13371338, %ebx
    call _start
