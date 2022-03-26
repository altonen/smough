#ifndef __PIT_H__
#define __PIT_H__

#include <stddef.h>

#define PIT_DATA_0  0x40
#define PIT_DATA_1  0x41
#define PIT_DATA_2  0x42
#define PIT_CMD     0x43
#define PIT_CHNL_2  0x61

void pit_phase(size_t hz);
void pit_wait(size_t ticks);
void pit_install(void);

#endif /* __PIT_H__ */
