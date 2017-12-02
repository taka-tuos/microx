include mkfiles/cflags.mk

# default

default :
	$(MAKE) core

# rules

bootcd.iso : core Makefile
	$(COPY) kernel/kernel.elf grub/
	$(GRUB)  --output=bootcd.iso grub

vfat.img : apps Makefile $(shell find fs_files/ -type f)
	-$(DEL) vfat.img
	$(BZIP2) -d -k vfat.img.bz2
	find fs_files/ -type d | cut -d/ -f2-8 | xargs -I{} mmd -i vfat.img {}
	find fs_files/ -type f | cut -d/ -f2-8 | xargs -I{} mcopy -i vfat.img fs_files/{} ::{}

# commands

apps :
	$(MAKE) -C app

core :
	$(MAKE) -C kernel

bootcd :
	$(MAKE) bootcd.iso

hdimage :
	$(MAKE) vfat.img

run : kernel bootcd.iso vfat.img
	$(QEMU) -hda vfat.img -cdrom bootcd.iso -boot d
	
clean :
	$(MAKE) -C lib clean
	$(MAKE) -C app clean
	$(MAKE) -C kernel clean
