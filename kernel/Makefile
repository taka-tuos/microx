OBJS_KERNEL = multiboot.o kernel.o ata.o pci.o ff.o diskio.o memory.o console.o timer.o dsctbl.o int.o mtask.o keyboard.o fifo.o mutex.o ffsystem.o sheet.o nvbdflib.o mouse.o graphic.o

include ../mkfiles/cflags.mk

CFLAGS += -I. -I../include/

# default

default :
	$(MAKE) kernel.elf

# rules

kernel.elf : $(OBJS_KERNEL) Makefile
	$(MAKE) -C ../lib
	$(LD) -m elf_i386 -Ttext=0x100000 -L../lib/ -Map kernel.map $(OBJS_KERNEL) -lc -o kernel.elf

# normal rules

%.o : %.c Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.S

# commands
	
clean :
	-$(DEL) *.o
	-$(DEL) *.map
	-$(DEL) *.elf
