# make --version
# XDIR:=/u/cs452/public/xdev
XDIR:=/opt/aarch64-none-elf
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CXX:=$(XBINDIR)/$(TRIPLE)-g++
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump

MMU?=on
OPT?=off

# COMPILE OPTIONS
WARNINGS=-Wall -Wextra -Wpedantic

ifeq ($(MMU),on)
MMUFLAGS:=-DMMU
else
MMUFLAGS:=-mstrict-align -mgeneral-regs-only
endif
ifeq ($(OPT),on)
OPTFLAGS=-O3
endif


CXXFLAGS:=-g -pipe -static -mstrict-align -ffreestanding -fno-exceptions -fno-unwind-tables \
			-mgeneral-regs-only -mcpu=$(ARCH) -march=armv8-a $(MMUFLAGS) $(WARNINGS) $(OPTFLAGS) -std=c++17 

# Linker options
LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld -Wl,--no-warn-rwx-segments -nostartfiles -nodefaultlibs -nostdlib -lc

# Source files and include dirs
SRC_DIR := src
SOURCES := $(shell find $(SRC_DIR) -name "*.cpp" -o -name "*.S")
OBJECTS := $(patsubst %.cpp, %.o, $(patsubst %.S, %.o, $(SOURCES)))
DEPENDS := $(patsubst %.cpp, %.d, $(patsubst %.S, %.d, $(SOURCES)))

# Default rule
all: iotest.img

clean:
	rm -f $(OBJECTS) $(DEPENDS) iotest.elf iotest.img

iotest.img: iotest.elf
	$(OBJCOPY) $< -O binary $@

iotest.elf: $(OBJECTS) linker.ld
	$(CXX) $(CXXFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)
ifneq ($(MMU),on)
	@$(OBJDUMP) -d $@ | grep -Fq q0 && printf "\n***** WARNING: SIMD DETECTED! *****\n\n" || true
endif

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%.o: %.S Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
