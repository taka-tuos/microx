OBJS_GOLIBC = vsprintf.o abs.o atoi.o errno.o memchr.o memcmp.o memcpy.o memmove.o \
			memset.o qsort.o rand.o sprintf.o strcat.o strchr.o strcmp.o strcpy.o \
			strcspn.o strlen.o strncat.o strncmp.o strncpy.o strpbrk.o strrchr.o \
			strspn.o strstr.o strtol.o strtoul.o strtoul0.o 

include ../cflags.mk

# default

default :
	$(MAKE) golibc.a

# rules

golibc.a : $(OBJS_GOLIBC) Makefile
	$(AR) rcs golibc.a $(OBJS_GOLIBC)

# normal rules

%.o : %.c Makefile
	$(GCC) $(CFLAGS) -I. -c -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -I. -c -o $*.o $*.S

# commands

clean :
	-$(DEL) *.o
	-$(DEL) golibc.a
