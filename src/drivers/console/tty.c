#include <drivers/gfx/vga.h>
#include <kernel/kassert.h>

void (*__tty_putc)(char)   = vga_put_char;
void (*__tty_puts)(char *) = vga_put_str;

void tty_putc(char c)
{
    __tty_putc(c);
}

void tty_puts(char *data)
{
    __tty_puts(data);
}

void tty_install_putc(void (*_tty_putc)(char c))
{
    kassert(_tty_putc != NULL);
    __tty_putc = _tty_putc;
}

void tty_install_puts(void (*_tty_puts)(char *s))
{
    kassert(_tty_puts != NULL);
    __tty_puts = _tty_puts;
}
