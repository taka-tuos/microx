include ../../cflags.mk

CFLAGS += -I../../libmicrox -I../../golibc

$(TARGET).eim : Makefile $(OBJS) ../app.lds
	$(MAKE) -C ../../libmicrox
	$(GCC) $(CFLAGS) -Wl,-T../app.lds,-Map=$(TARGET).map -o $(TARGET).eim $(OBJS) ../../libmicrox/libmicrox.a ../../golibc/golibc.a
	$(COPY) $(TARGET).eim ../../fs_files/

%.o : %.c Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.c

%.o : %.S Makefile
	$(GCC) -c $(CFLAGS) -o $*.o $*.S

default :
	$(MAKE) $(TARGET).eim

clean :
	-$(DEL) *.o
	-$(DEL) *.eim
	-$(DEL) ../../fs_files/*.eim


