#include <drivers/gfx/vga.h>

/* Use the VGA routines by default.
 * When VBE is initialized, new routines will be installed */
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
