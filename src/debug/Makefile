# Makefile for Hatari's debugging & tracing support.

CPUDIR ?= uae-cpu
ENABLE_DSP_EMU ?= 0

# Include settings
include ../../Makefile-wii.cnf

# Additional include directories:
INCFLAGS = -I. -I../.. -I../includes -I../$(CPUDIR) -I../falcon $(CPPFLAGS)

SRCS = log.c debugui.c breakcond.c debugcpu.c debugInfo.c evaluate.c history.c symbols.c profile.c profilecpu.c profiledsp.c natfeats.c console.c 68kDisass.c
ifeq ($(ENABLE_DSP_EMU),1)
SRCS += debugdsp.c
INCFLAGS += -DENABLE_DSP_EMU=1 
endif

# Set extra flags passed to the compiler
CFLAGS += $(INCFLAGS) $(SDL_CFLAGS)


OBJS = $(SRCS:.c=.o)

all: debug.a

debug.a: $(OBJS)
	$(AR) cru $@ $^
	$(RANLIB) $@


clean:
	$(RM) *.o debug.a

distclean: clean
	$(RM) Makefile.dep *~ *.bak *.orig


# Use "make depend" to generate file dependencies:
Makefile.dep: Makefile ../Makefile
	$(CC) -M $(CFLAGS) $(SRCS) > Makefile.dep

depend: Makefile.dep

-include Makefile.dep
