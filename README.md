# smough

smough is a 64-bit monolithic kernel written in C

It's a rewrite of an older kernel started in 2017 which over the years has degraded into such a horrible mess that it's easier to write a new kernel from scratch rather than trying to fix the old one.

# (planned) features

* amd64 platform support
* symmetric multiprocessing
* preemptive, dynamic priority-based multitasking
* POSIX-compliant system call interface
* virtual file system
   * devfs
   * initramfs
* VESA VBE 3.0 GFX Driver
* network stack
   * Ethernet/ARP/IPv4/UDP/TCP/DHCP
