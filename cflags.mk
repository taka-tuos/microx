MAKE     = make -r
GCC      = gcc
LD       = ld
DEL      = rm
COPY     = cp
GRUB     = grub-mkrescue
QEMU     = qemu-system-i386
BZIP2    = bzip2

CFLAGS = -I. -Igolibc/ -Ilibmicrox/ -fno-builtin
CFLAGS += -fno-common -nostdlib -nostdinc -nostartfiles -nodefaultlibs 
CFLAGS += -m32 -march=i386 -fno-pie
CFLAGS += -mno-red-zone -ffreestanding -fno-stack-protector
