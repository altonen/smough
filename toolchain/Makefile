.PHONY: all clean

all: mkinitrd initrd

mkinitrd:
	$(CC) -o mkinitrd util/mkinitrd.c $(CFLAGS) -D__DEBUG_

initrd: mkinitrd
	./mkinitrd -f Makefile "/sbin/init" -f util/mkinitrd.c "/sbin/sh"

clean:
	$(MAKE) --directory=programs clean
	rm -f initrd.bin mkinitrd
