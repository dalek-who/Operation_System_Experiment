# C compiler and linker
CROSS_COMPILE = mipsel-linux-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
SIZE		= $(CROSS_COMPILE)size


# Add debug flag
CFLAGS += -g

## Linker flags
# Don't use the standard startup files or libraries
LDFLAGS += -nostdlib
#
# Specify a location on a per-target basis
LDFLAGS += -Ttext

all: bootblock image

bootblock: bootblock.o
	${CC} -G 0 -O2  -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800000 -N -o bootblock bootblock.s -nostdlib -e main -Wl,-m -Wl,elf32ltsmip -T ld.script

image: bootblock
	./createimage	--extended bootblock

clean:
	rm -fr bootblock image *.o
