OBJS_LIB = libmicrox.o

include ../cflags.mk

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
