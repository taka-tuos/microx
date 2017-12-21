MAKE     = make -r
GCC      = gcc
LD       = ld
DEL      = rm
COPY     = cp
GRUB     = grub-mkrescue
QEMU     = kvm
BZIP2    = bzip2

CFLAGS = -fno-builtin -g
CFLAGS += -fno-common -nostdlib -nostdinc -nostartfiles -nodefaultlibs 
CFLAGS += -m32 -march=i386 -fno-pie
CFLAGS += -mno-red-zone -ffreestanding -fno-stack-protector
