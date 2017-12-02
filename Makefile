OBJS_KERNEL = multiboot.o kernel.o ata.o pci.o ff.o diskio.o memory.o console.o timer.o dsctbl.o int.o mtask.o keyboard.o fifo.o

include cflags.mk

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

vfat.img : Makefile $(shell find fs_files/ -type f) apps
	-$(DEL) vfat.img
	$(BZIP2) -d -k vfat.img.bz2
	find fs_files/ -type d | cut -d/ -f2-8 | xargs -I{} mmd -i vfat.img {}
	find fs_files/ -type f | cut -d/ -f2-8 | xargs -I{} mcopy -i vfat.img fs_files/{} ::{}

# normal rules

%.o : %.c Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.S

# commands

apps :
	$(MAKE) -C app

bootcd :
	$(MAKE) bootcd.iso

hdimage :
	$(MAKE) vfat.img

run : bootcd.iso vfat.img
	$(QEMU) -hda vfat.img -cdrom bootcd.iso -boot d
	
clean :
	$(MAKE) -C golibc clean
	$(MAKE) -C libmicrox clean
	$(MAKE) -C app clean
	-$(DEL) *.o
