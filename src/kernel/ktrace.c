#include <fs/elf.h>
#include <kernel/kassert.h>
#include <kernel/util.h>

#define CALL_STACK_MAX_DEPTH 8

static uint8_t *__sym;
static uint8_t *__str;
static size_t __sym_size;
static size_t __str_size;

/* Printing a stack trace for kernel asserts
 *
 * Assert in itself is a very useful tool for checking the logic of the program
 * but as the kernel grows in size it may be hard to reason about how this assert
 * was triggered without a proper stack trace.
 *
 * When smough is booting, it parses the ELF symbol table provided by multiboot
 * which contains (address, symbol name) combinations. These are then saved to
 * a local storage and when kassert() is called, it calls ktrace() which walks
 * the base pointer until it finds kmain() which means that the call trace has
 * traced back to the boot and all relevant information has been found.
 *
 * As the base pointer unwound, its current value is checked against the ELF
 * symbol table and if a corresponding function name is found, its name is printed.
 * Otherwise only the address is printed. */

int ktrace_register_sym(uint8_t *sym, size_t sym_size, uint8_t *str, size_t str_size)
{
    __sym      = sym;
    __str      = str;
    __sym_size = sym_size;
    __str_size = str_size;

    return 0;
}

static int __trace(uint64_t rbp)
{
    Elf64_Sym *iter = (Elf64_Sym *)__sym;

    for (size_t i = 0; i < __sym_size; ) {
        iter = (Elf64_Sym *)((uint8_t *) __sym + i);

        if (iter->st_value <= rbp && rbp <= iter->st_value + iter->st_size) {
            if (ELF64_ST_TYPE(iter->st_info) == STT_FUNC) {
                kprint("--> 0x%x %s\n", rbp, &__str[iter->st_name]);

                if (!kstrcmp((const char *)&__str[iter->st_name], "kmain"))
                    return 1;
                return 0;
            } else
                kprint("--> 0x%x\n", rbp);
        }

        i += sizeof(Elf64_Sym);
    }

    return 0;
}

void ktrace(void)
{
    uint64_t *rbp = 0;
    asm volatile ("mov %%rbp, %0": "=g"(rbp));

    for (int i = 0; i < CALL_STACK_MAX_DEPTH; ++i) {
        if (__trace(rbp[1]))
            return;
        rbp = (uint64_t *)rbp[0];
    }
}
