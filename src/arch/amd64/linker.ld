OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS {
	boot       PT_LOAD;
	trampoline PT_LOAD;
	percpu     PT_LOAD;
	ramfs      PT_LOAD;
	kernel     PT_LOAD;
}

P_START = 0x0000000001000000;
V_START = 0xffffffff81000000;

SECTIONS
{
	. = 1M;
	_boot_start = .;

	.boot : AT(ADDR(.boot))
	{
		*(.boot.multiboot)
		*(.boot.text)
		*(.boot.data)
		*(.boot.bss)
		*boot.o(COMMON)
	} :boot

	_boot_end = ALIGN(4K) + V_START - P_START;

	.trampoline 0x55000 : AT(_boot_end - V_START + P_START)
	{
		_trampoline_start = LOADADDR(.trampoline) + V_START - P_START;
		*(.trampoline)
		_trampoline_end = _trampoline_start + (. - 0x55000);
		. = ALIGN(4K);
	} :trampoline

	. = _boot_end + SIZEOF(.trampoline);

	.percpu ALIGN(64) : AT(ADDR(.percpu) - V_START + P_START)
	{
		_percpu_start = LOADADDR(.percpu) + V_START - P_START;
		*(.percpu)
		_percpu_end = .;
	}:percpu

	.ramfs ALIGN(4K) : AT(ADDR(.ramfs) - V_START + P_START)
	{
		_ramfs_start = LOADADDR(.ramfs) + V_START - P_START;
		*(.ramfs)
		_ramfs_end = .;
	}:ramfs

	.text ALIGN(4K) : AT(ADDR(.text) - V_START + P_START)
	{
		*(.text)
	}:kernel

	.rodata ALIGN(4K) : AT (ADDR(.rodata) - V_START + P_START)
	{
		*(.rodata)
		*(.rodata.*)
	}:kernel

	.data ALIGN(4K) : AT (ADDR(.data) - V_START + P_START)
	{
		*(.data)
	}:kernel

	.bss ALIGN(4K) : AT (ADDR(.bss) - V_START + P_START)
	{
		*(.bss)
		*(COMMON)
	}:kernel

	_kernel_physical_end = LOADADDR(.bss) + SIZEOF(.bss);
}
