TARGETS = addacs_in.pd_linux addacs_out.pd_linux

pd_linux: $(TARGETS) 


.SUFFIXES: .pd_linux


LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch -fPIC

LINUXINCLUDE =  -I../../src -I..

clean:
	rm $(TARGETS)

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld -shared -o $*.pd_linux $*.o -lc -lm
	strip --strip-unneeded $*.pd_linux
	rm $*.o


