#!/bin/bash
[ -f Makefile ] || {
	echo "Makefile not found"
	exit 1
}
echo "Building the application"
make

[ -f iotest.elf ] || {
    echo "Kernel not found"
    exit 1
}

echo "Running QEMU"

# change dir to scripts
# cd scripts

# Run the script
./qemu.sh


