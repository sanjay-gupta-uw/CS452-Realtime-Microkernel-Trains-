# make --version
XDIR:=/u/cs452/public/xdev
# XDIR:=/Applications/ArmGNUToolchain/14.2.rel1/aarch64-none-elf
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CXX:=$(XBINDIR)/$(TRIPLE)-g++
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump

# COMPILE OPTIONS
WARNINGS=-Wall -Wextra -Wpedantic
CXXFLAGS:=-g -pipe -static -mstrict-align -ffreestanding -fno-exceptions -fno-unwind-tables \
			-mgeneral-regs-only -mcpu=$(ARCH) -march=armv8-a $(WARNINGS) -std=c++17 

# Linker options
LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld -Wl,--no-warn-rwx-segments -nostartfiles -nodefaultlibs -nostdlib

# Source files and include dirs
SOURCES := $(wildcard *.cpp) $(wildcard *.S) # Only look for .cpp and .S files
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

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%.o: %.S Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
