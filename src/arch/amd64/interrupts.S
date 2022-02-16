.section .text
.code64

# Interrupt service routines

.global isr0  # division by zero
.global isr1  # debug
.global isr2  # non-maskable interrupt
.global isr3  # breakpoint
.global isr4  # into detected overflow
.global isr5  # out of bounds
.global isr6  # invalid opcode
.global isr7  # no coprocessor
.global isr8  # double fault
.global isr9  # coprocessor segment overrun
.global isr10 # bad tss
.global isr11 # segment not present
.global isr12 # stack fault
.global isr13 # general protection fault
.global isr14 # page faul
.global isr15 # unknown interrupt
.global isr16 # coprocessor fault
.global isr17 # alignment check exception
.global isr18 # machine check exception
.global isr19 # simd floating point exception
.global isr20 # virtualization exception
.global isr128 # system call

.global irq0  # timer, Local APIC is configured during initialization
.global irq1  # keyboard
.global irq2  # cascade
.global irq3  # com2
.global irq4  # com1
.global irq5  # lpt2
.global irq6  # floppy disk
.global irq7  # lpt1
.global irq8  # cmos real-time clock
.global irq9  # legacy scsi / nic
.global irq10 # scsi / nic
.global irq11 # scsi / nic
.global irq12 # ps2 mouse
.global irq13 # fpu / coprocessor / inter-processor
.global irq14 # primary ata hard disk
.global irq15 # secondary ata hard disk

# how everything works:
# 1. clear interrupt flag
# 2. push "dummy" error code (not for every interrupt)
# 3. push ISR number
# 4. jump to isr_common, save cpu state and call interrupt handler

# divide by zero
isr0:
    cli
    pushq $0 # error code 0 == no error code
    pushq $0 # interrupt number
    jmp isr_common

# debug
isr1:
    cli
    pushq $0
    pushq $1
    jmp isr_common

# non-maskable interrupt
isr2:
    cli
    pushq $0
    pushq $2
    jmp isr_common

# breakpoint
isr3:
    cli
    pushq $0
    pushq $3
    jmp isr_common

# into detected overflow
isr4:
    cli
    pushq $0
    pushq $4
    jmp isr_common

# out of bounds
isr5:
    cli
    pushq $0
    pushq $5
    jmp isr_common

# invalid opcode
isr6:
    cli
    pushq $0
    pushq $6
    jmp isr_common

# no coprocessor
isr7:
    cli
    pushq $0
    pushq $7
    jmp isr_common

# double fault
isr8:
    cli
    pushq $8 # NOTE! no error code pushed
    jmp isr_common

# coprocessor segment overrun
isr9:
    cli
    pushq $0
    pushq $9
    jmp isr_common

# bad tss
isr10:
    cli
    pushq $10
    jmp isr_common

# segment not present
isr11:
    cli
    pushq $11
    jmp isr_common

# stack fault
isr12:
    cli
    pushq $12
    jmp isr_common

# general protection fault
isr13:
    cli
    pushq $13
    jmp isr_common

# page fault
isr14:
    cli
    pushq $14
    jmp isr_common

## unknown interrupt
isr15:
    cli
    pushq $0
    pushq $15
    jmp isr_common

# coprocessor fault
isr16:
    cli
    pushq $0
    pushq $16
    jmp isr_common

isr17:
    cli
    pushq $17
    jmp isr_common

isr18:
    cli
    pushq $0
    pushq $18
    jmp isr_common

isr19:
    cli
    pushq $0
    pushq $19
    jmp isr_common

isr20:
    cli
    pushq $0
    pushq $20
    jmp isr_common

isr128:
    cli
    pushq $0
    pushq $0x80
    jmp isr_common

irq0:
    cli
    pushq $0
    pushq $32
    jmp isr_common

irq1:
    cli
    pushq $0
    pushq $33
    jmp isr_common

irq2:
    cli
    pushq $0
    pushq $34
    jmp isr_common

irq3:
    cli
    pushq $0
    pushq $35
    jmp isr_common

irq4:
    cli
    pushq $0
    pushq $36
    jmp isr_common

irq5:
    cli
    pushq $0
    pushq $37
    jmp isr_common

irq6:
    cli
    pushq $0
    pushq $38
    jmp isr_common

irq7:
    cli
    pushq $0
    pushq $39
    jmp isr_common

irq8:
    cli
    pushq $0
    pushq $40
    jmp isr_common

irq9:
    cli
    pushq $0
    pushq $41
    jmp isr_common

irq10:
    cli
    pushq $0
    pushq $42
    jmp isr_common

irq11:
    cli
    pushq $0
    pushq $43
    jmp isr_common

irq12:
    cli
    pushq $0
    pushq $44
    jmp isr_common

irq13:
    cli
    pushq $0
    pushq $45
    jmp isr_common

irq14:
    cli
    pushq $0
    pushq $46
    jmp isr_common

irq15:
    cli
    pushq $0
    pushq $47
    jmp isr_common

# save cpu state, call interrupt handler
# and then restore state
isr_common:
    cmp $0x8, 24(%rsp)
    je 1f
    swapgs
1:
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rdi
    pushq %rsi
    pushq %rbp
    pushq %rbx
    pushq %rdx
    pushq %rcx
    pushq %rax
    movq %rsp, %rdi
    mov $interrupt_handler, %rax
    call *%rax
    popq %rax
    popq %rcx
    popq %rdx
    popq %rbx
    popq %rbp
    popq %rsi
    popq %rdi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
    addq $16, %rsp # discard error code and isr number

    cmp $0x8, 8(%rsp)
    je 2f
    swapgs
2:
    iretq
