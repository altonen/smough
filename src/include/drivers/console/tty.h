#ifndef __TTY_H__
#define __TTY_H__

void tty_putc(char c);
void tty_puts(char *data);

void tty_install_putc(void (*tty_putc)(char c));
void tty_install_puts(void (*tty_puts)(char *s));

#endif /* __TTY_H__ */
