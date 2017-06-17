OBJS_KERNEL = multiboot.o kernel.o ata.o pci.o ff.o diskio.o memory.o console.o timer.o dsctbl.o int.o mtask.o keyboard.o fifo.o

MAKE     = make -r
GCC      = gcc
LD       = ld
DEL      = rm
COPY     = cp
GRUB     = grub-mkrescue
QEMU     = qemu-system-i386
BZIP2    = bzip2

CFLAGS = -I. -Igolibc/ -fno-builtin
CFLAGS += -fno-common -nostdlib -nostdinc -nostartfiles -nodefaultlibs 
CFLAGS += -m32 -march=i386
CFLAGS += -mno-red-zone -ffreestanding -fno-stack-protector

# default

default :
	$(MAKE) kernel.elf

# rules

kernel.elf : $(OBJS_KERNEL) Makefile
	$(MAKE) -C golibc
	$(LD) -m elf_i386 -Ttext=0x100000 -Map kernel.map $(OBJS_KERNEL) golibc/golibc.a -o kernel.elf

bootcd.iso : kernel.elf Makefile
	$(COPY) kernel.elf grub/
	$(GRUB)  --output=bootcd.iso grub

vfat.img : Makefile
	-$(DEL) vfat.img
	$(BZIP2) -d -k vfat.img.bz2
	ls fs_files | xargs -I{} mcopy -i vfat.img fs_files/{} ::{}

# normal rules

%.o : %.c Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.S

# commands

bootcd :
	$(MAKE) bootcd.iso

hdimage :
	$(MAKE) vfat.img

run : bootcd.iso vfat.img
	$(QEMU) -hda vfat.img -cdrom bootcd.iso -boot d
	
clean :
	$(MAKE) -C golibc clean
	-$(DEL) *.o
