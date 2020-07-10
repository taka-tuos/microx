include ../../mkfiles/cflags.mk

CFLAGS += -I. -I../../include

# default

default :
	$(MAKE) $(TARGET).a

# rules

$(TARGET).a : $(OBJS) Makefile
	$(AR) rcs $(TARGET).a $(OBJS)
	cp $(TARGET).a ../

# normal rules

%.o : %.c Makefile
	$(GCC) $(CFLAGS) -I. -c -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -I. -c -o $*.o $*.S

# commands

clean :
	-$(RM) *.o
	-$(RM) $(TARGET).a
	-$(RM) ../$(TARGET).a
