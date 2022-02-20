#ifndef __KASSERT_H__
#define __KASSERT_H__

#include <kernel/kprint.h>
#include <stdint.h>
#include <stddef.h>

/* register symbol found from multiboot2 info */
int ktrace_register_sym(uint8_t *sym, size_t sym_size, uint8_t *str, size_t str_size);

/* print call stack */
void ktrace(void);

#ifdef NDEBUG
#define kassert(cond)
#else
#define kassert(cond) __kernel_assert(cond)
#define __kernel_assert(cond) \
	if (!(cond)) { \
		kprint("\nassertion \"%s\" failed at %s:%d\n", #cond, __FILE__, __LINE__); \
		ktrace(); \
		while (1); \
	}
#endif

#endif /* __KASSERTH__ */
