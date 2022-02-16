ROOTDIR?=$(shell pwd)
include $(ROOTDIR)/Makefile.config

.PHONY: all kernel iso clean headers

all: headers kernel iso

headers:
	@mkdir -p $(SYSROOT)
	$(MAKE) --directory=src headers

kernel:
	$(MAKE) --directory=src install

iso:
	@mkdir -p isodir/boot/grub
	@cp sysroot/boot/smough.kernel isodir/boot/smough.kernel
	@echo "set timeout=0"                     > isodir/boot/grub/grub.cfg
	@echo "set default=0"                    >> isodir/boot/grub/grub.cfg
	@echo "menuentry smough {"               >> isodir/boot/grub/grub.cfg
	@echo "multiboot2 /boot/smough.kernel }" >> isodir/boot/grub/grub.cfg
	@grub-mkrescue -o smough.iso isodir

run:
	qemu-system-x86_64 -cdrom smough.iso $(QEMUFLAGS) &> /dev/null

debug:
	objcopy --only-keep-debug src/smough.kernel kernel.sym
	qemu-system-x86_64 -cdrom smough.iso $(QEMUFLAGS) -gdb tcp::1337 -S

clean:
	$(MAKE) --directory=src clean
	@rm -rf sysroot isodir smough.iso kernel.sym

clean-all:
	$(MAKE) --directory=src clean-all
	@rm -rf sysroot isodir smough.iso kernel.sym
