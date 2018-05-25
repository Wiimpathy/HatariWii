# Makefile for Hatari's UAE CPU.

# Include settings
include ../../Makefile-wii.cnf


CFLAGS += -I. -I../.. -I../includes -I../debug -I../gui-sdl -I../falcon $(CPPFLAGS) $(SDL_CFLAGS)
# disable several warnings for the CPU emu code as it comes from elsewhere
QUIETCFLAGS = $(CFLAGS) -Wno-unused -Wno-sign-compare -Wno-shadow


CPUCSRCS = hatari-glue.c memory.c newcpu.c readcpu.c fpp.c

CPUGENOBS = cpuemu_0.o cpuemu_11.o cpuemu_13.o cpuemu_20.o \
            cpuemu_21.o cpuemu_22.o cpuemu_23.o cpuemu_24.o cpuemu_31.o cpuemu_32.o cpuemu_33.o cpuemu_40.o \
            cpudefs.o cpustbl.o

CPU_OBS = $(CPUCSRCS:.c=.o) $(CPUGENOBS)


all: libUaeCpu.a

libUaeCpu.a: $(CPU_OBS)
	$(AR) cru $@ $^
	$(RANLIB) $@

build68k: build68k.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $<

cpudefs.c: build68k table68k
	./build68k <table68k >cpudefs.c

cpudefs-host.o: cpudefs.c
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

readcpu-host.o: readcpu.c
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

gencpu.o: gencpu.c
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

gencpu: gencpu.o readcpu-host.o cpudefs-host.o
	$(HOSTCC) $(HOSTLDFLAGS) -o $@ $^

cpuemu.c: gencpu
	./gencpu


cpustbl.c: cpuemu.c
cputbl.h: cpuemu.c


# this is for people with low memory (is there a way do do this with a single rule ?)
cpuemu_0.o: cpuemu.c
	$(CC) -DPART_1 -c $(QUIETCFLAGS) $< -o $@
cpuemu_11.o: cpuemu.c
	$(CC) -DPART_2 -c $(QUIETCFLAGS) $< -o $@
cpuemu_13.o: cpuemu.c
	$(CC) -DPART_3 -c $(QUIETCFLAGS) $< -o $@
cpuemu_20.o: cpuemu.c
	$(CC) -DPART_4 -c $(QUIETCFLAGS) $< -o $@
cpuemu_21.o: cpuemu.c
	$(CC) -DPART_5 -c $(QUIETCFLAGS) $< -o $@
cpuemu_22.o: cpuemu.c
	$(CC) -DPART_6 -c $(QUIETCFLAGS) $< -o $@
cpuemu_23.o: cpuemu.c
	$(CC) -DPART_7 -c $(QUIETCFLAGS) $< -o $@
cpuemu_24.o: cpuemu.c
	$(CC) -DPART_8 -c $(QUIETCFLAGS) $< -o $@
cpuemu_31.o: cpuemu.c
	$(CC) -DPART_9 -c $(QUIETCFLAGS) $< -o $@
cpuemu_32.o: cpuemu.c
	$(CC) -DPART_10 -c $(QUIETCFLAGS) $< -o $@
cpuemu_33.o: cpuemu.c
	$(CC) -DPART_11 -c $(QUIETCFLAGS) $< -o $@

newcpu.o: sysdeps.h hatari-glue.h maccess.h memory.h newcpu.h ../includes/dialog.h


clean:
	$(RM) *.o gencpu build68k libUaeCpu.a

distclean:	clean
	$(RM) cpudefs.c cpustbl.c cputbl.h cpuemu.c
	$(RM) *~ *.bak *.orig Makefile.dep


# Use "make depend" to generate file dependencies:
Makefile.dep: Makefile
	$(CC) -M $(CFLAGS) $(CPUCSRCS) > Makefile.dep

depend: Makefile.dep

-include Makefile.dep
