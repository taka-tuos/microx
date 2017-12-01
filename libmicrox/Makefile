OBJS_LIB = libmicrox.o

MAKE     = make -r
GCC      = gcc
AR       = ar
DEL      = rm
COPY     = cp
GRUB     = grub-mkrescue

CFLAGS = -I. -fno-builtin
CFLAGS += -fno-common -nostdlib -nostdinc -nostartfiles -nodefaultlibs 
CFLAGS += -m32 -march=i386 -fno-pie
CFLAGS += -mno-red-zone -ffreestanding -fno-stack-protector

# default

default :
	$(MAKE) libmicrox.a

# rules

libmicrox.a : $(OBJS_LIB) Makefile
	$(AR) rcs libmicrox.a $(OBJS_LIB)

# normal rules

%.o : %.c Makefile
	$(GCC) $(CFLAGS) -I. -c -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -I. -c -o $*.o $*.S

# commands

clean :
	-$(DEL) *.o
	-$(DEL) libmicrox.a
