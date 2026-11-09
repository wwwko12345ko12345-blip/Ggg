EE_BIN = army_rts.elf
EE_OBJS = main.o
EE_LIBS = -lpad -lkernel
EE_CFLAGS = -O2 -G0
EE_CXXFLAGS = -O2 -G0 -fno-exceptions -fno-rtti
all: $(EE_BIN)
clean:
	rm -f $(EE_OBJS) $(EE_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
