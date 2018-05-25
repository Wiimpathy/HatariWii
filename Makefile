all: 
	cd src; $(MAKE) -f Makefile.wii
clean: 
	cd src; $(MAKE) -f Makefile.wii clean
run: 
	cd src; wiiload hatari.dol
