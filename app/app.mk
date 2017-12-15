include ../../mkfiles/cflags.mk

CFLAGS += -I../../include -L../../lib

$(TARGET).eim : Makefile $(OBJS) ../crt0.o ../app.lds
	$(GCC) $(CFLAGS) -Wl,-T../app.lds,-Map=$(TARGET).map -o $(TARGET).eim $(OBJS) ../crt0.o $(LIBS)
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
	-$(DEL) *.map
	-$(DEL) ../../fs_files/*.eim


