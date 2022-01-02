# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX = mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

# uMPS2-related paths

# Simplistic search for the umps2 install. prefix. If you have umps2
# installed on some weird location, set UMPS2_DIR_PREFIX by hand.
ifneq ($(wildcard /usr/bin/umps2),)
    UMPS2_DIR_PREFIX = /usr
else
    UMPS2_DIR_PREFIX = /usr/local
endif

UMPS2_DATA_DIR = $(UMPS2_DIR_PREFIX)/share/umps2
UMPS2_INCLUDE_DIR = $(UMPS2_DIR_PREFIX)/include/umps2

# Compiler options
CFLAGS_LANG = -ansi
CFLAGS_MIPS = -mips1 -mfp32 -std=gnu11
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS2_INCLUDE_DIR) -I. -Wall

# Linker options
LDFLAGS = -nostdlib -T $(UMPS2_DATA_DIR)/umpscore.ldscript

# Add the location of crt*.S to the search path
VPATH = $(UMPS2_DATA_DIR)

.PHONY : all clean

all : kernel.core.umps

kernel.core.umps : kernel
	umps2-elf2umps -k $<

kernel : p1test_rikaya_v0.o pcb.o asl.o crtso.o libumps.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	find . -name "*.o" -o -name "kernel" -o -name "kernel.*.umps" -o -name "term*.umps" -type f|xargs rm -f

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<
