# Makefile for Hatari's Falcon support.

CPUDIR ?= uae-cpu

# Include settings
include ../../Makefile-wii.cnf

# Additional include directories:
INCFLAGS = -I../.. -I../includes -I../debug -I../$(CPUDIR) $(CPPFLAGS)


# Set flags passed to the compiler (e.g. optimization flags)
#CFLAGS := -g -O2 -Wall


FALCSRCS = videl.c hostscreen.c nvram.c dsp.c crossbar.c microphone.c dsp_core.c dsp_cpu.c dsp_disasm.c
ifeq ($(ENABLE_DSP_EMU),1)
FALCSRCS += dsp_core.c dsp_cpu.c dsp_disasm.c
INCFLAGS += -DENABLE_DSP_EMU=1
endif

# Set flags passed to the compiler (e.g. optimization flags)
#CFLAGS := -g -O2 -Wall

CFLAGS += $(INCFLAGS) $(SDL_CFLAGS)

FALCOBJS = $(FALCSRCS:.c=.o)


all: falcon.a

falcon.a: $(FALCOBJS)
	$(AR) cru $@ $^
	$(RANLIB) $@


clean:
	$(RM) *.o falcon.a

distclean: clean
	$(RM) Makefile.dep *~ *.bak *.orig


# Use "make depend" to generate file dependencies:
Makefile.dep: Makefile ../Makefile
	$(CC) -M $(CFLAGS) $(FALCSRCS) > Makefile.dep

depend: Makefile.dep

-include Makefile.dep
