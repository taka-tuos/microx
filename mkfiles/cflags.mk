MAKE     = make -r
GCC      = gcc
LD       = ld
DEL      = rm
COPY     = cp
GRUB     = grub-mkrescue
QEMU     = qemu-system-i386
BZIP2    = bzip2

CFLAGS = -fno-builtin -Os
CFLAGS += -fno-common -nostdlib -nostdinc -nostartfiles -nodefaultlibs 
CFLAGS += -m32 -march=i586 -fno-pie
CFLAGS += -mno-red-zone -ffreestanding -fno-stack-protector
