.PHONY: all lib usb clean

# DMODE_ROOT := $(abspath ../dmode)
DMODE_ROOT := /home/unics/Desktop/cs140e/cs140e/dmode


# MEMMAP := $(abspath memmap.ld)

CFLAGS := -mfpu=vfp -mfloat-abi=softfp -I$(abspath src)


# Common library sources + USB objects (USB must come first for linking)
COMMON_SRCS := $(shell find $(DMODE_ROOT)/lib -name '*.c')
COMMON_OBJS := $(USB_OBJS)
COMMON_OBJS += $(COMMON_SRCS:.c=.o)
COMMON_OBJS += $(DMODE_ROOT)/lib/src/start.o
COMMON_OBJS += ./src/instrument.o
COMMON_OBJS += ./src/helpers.o
COMMON_OBJS += ./src/midi.o
COMMON_OBJS += ./src/intable.o
COMMON_OBJS += ./src/minispi.o
COMMON_OBJS += ./src/osc.o
COMMON_OBJS += ./src/w5500.o
COMMON_OBJS += ./src/p10.o
COMMON_OBJS += ./src/spi.o
COMMON_OBJS += ./src/timer.o
COMMON_OBJS += ./src/vm.o
COMMON_OBJS += ./src/vms.o
COMMON_OBJS += ./src/sched.o
COMMON_OBJS += ./src/dma.o
COMMON_OBJS += ./src/m32.o

# USB include path - MUST come before library includes

# Set CFLAGS with USB includes FIRST before including Makefile.gcc


TARGETS := main.bin badapple.bin

include $(DMODE_ROOT)/lib/scripts/Makefile.gcc
include $(DMODE_ROOT)/lib/scripts/Makefile.ld

all: lib  $(TARGETS)

lib:
	$(MAKE) -C $(DMODE_ROOT)/lib

badapple.bin: badapple.mp4
	python3 convert.py

src/dma.c: src/p10.ass ass.py
	python3 ass.py src/p10.ass > src/dma.c


clean:
	$(MAKE) -C $(DMODE_ROOT)/lib clean
	rm -f *.o *.bin *.elf *.list
	rm -f $(USB_DIR)/src/*.o
