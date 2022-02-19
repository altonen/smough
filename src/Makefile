ROOTDIR?=$(shell pwd)/..

include $(ROOTDIR)/Makefile.config
include $(ARCHDIR)/make.config
include $(DRIVERDIR)/make.config
include $(LIBRARYDIR)/make.config

DESTDIR=$(realpath $(shell pwd)/../sysroot)

CFLAGS  := $(CFLAGS) --sysroot=$(DESTDIR) $(KERNEL_ARCH_CFLAGS)
LDFLAGS := $(LDFLAGS) -nostdlib -lgcc $(KERNEL_ARCH_LDFLAGS)

KERNEL_OBJS= \
	$(KERNEL_ARCH_OBJS) \
	$(KERNEL_DRIVER_OBJS) \
	$(KERNEL_LIB_OBJS) \
	kernel/kmain.o \
	kernel/irq.o \
	kernel/kprint.o \
	kernel/kpanic.o \
	kernel/util.o

OBJS = $(KERNEL_OBJS)
CLEAN_OBJS = $(KERNEL_OBJS)
LINK_LIST = $(OBJS) $(LDFLAGS)

.PHONY: all clean install headers install-kernel
.SUFFIXES: .o .c .S

all: smough.kernel
install: headers install-kernel

headers:
	echo $(DESTDIR)
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: smough.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp smough.kernel $(DESTDIR)$(BOOTDIR)

smough.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot2 smough.kernel

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS)

clean:
	rm -f smough.kernel kernel.map
	rm -f $(CLEAN_OBJS)

clean-all:
	rm -f smough.kernel kernel.map
	rm -f $(OBJS)
