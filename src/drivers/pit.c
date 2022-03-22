#include <kernel/io.h>
#include <kernel/pic.h>
#include <drivers/pit.h>
#include <kernel/kprint.h>

static size_t ticks = 0;

void pit_phase(size_t hz)
{
	size_t div = 1193180 / hz;
	outb(PIT_CMD, 0x36);
	outb(PIT_DATA_0, div & 0xff);
	outb(PIT_DATA_0, div >> 8);
}

void pit_handler()
{
	ticks++;
}

void pit_wait(size_t wait)
{
	size_t diff = ticks + wait;
	while (ticks < diff);
}

void timer_install(void)
{
	pic_install_handler(pit_handler, 0);
}
