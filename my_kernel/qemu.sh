#!/bin/bash
{
    read -r
    read -r line
    pts=$(echo $line | cut -d ' ' -f5)
    tmux splitw -h "screen $pts 115200"
    tmux resize-pane -L 50  # Increase pane width by 50 columns (adjust as needed)
    echo "Opening terminal on $pts"
    read -r line
    # echo "Opening terminal on $pts"
    # tmux splitw -v screen $pts 2400 # For some reason qemu only works with uart0
    cat
# } < <(qemu-system-aarch64 -kernel ../iotest.img -machine raspi4b -cpu cortex-a72 -nographic -serial pty -serial none -serial none -serial pty -s -S)
} < <(qemu-system-aarch64 -kernel iotest.img -machine raspi4b -cpu cortex-a72 -nographic -serial pty -serial none -serial none -serial pty)
