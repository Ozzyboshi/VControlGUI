AS=vasmm68k_mot
ASMFLAGS=-Fhunk -devpac -m68000 -quiet
OUTFILE=VControlGUI

OBJECTS = \
V_CPU_CACR.o\
V_CPU_PCR.o\
V_FPU.o\

all: $(OUTFILE)

clean:
	rm $(OBJECTS)
	rm VControlGUI

V_CPU_CACR.o: $(BUILDDEPS)
	$(AS) $(ASMFLAGS) -I /opt/amiga/m68k-amigaos/ndk-include -o $@ $*.asm

V_CPU_PCR.o: $(BUILDDEPS)
	$(AS) $(ASMFLAGS) -I /opt/amiga/m68k-amigaos/ndk-include -o $@ $*.asm

V_FPU.o: $(BUILDDEPS)
	$(AS) $(ASMFLAGS) -I /opt/amiga/m68k-amigaos/ndk-include -o $@ $*.asm

VControlGUI: $(OBJECTS) VControlGUI.c
	m68k-amigaos-gcc VControlGUI.c $(OBJECTS) -o VControlGUI


