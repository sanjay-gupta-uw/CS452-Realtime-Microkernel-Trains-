#!/bin/bash
{
    read -r
    read -r line
    pts=$(echo $line | cut -d ' ' -f5)
    
    # Open tmux window with specific number of cols/rows
    tmux splitw -h "screen $pts 115200"  # Adjust the pts and baud rate if needed
    tmux resize-pane -x 132 -y 100       # Set tmux pane to 120 columns and 40 rows

    echo "Opening terminal on $pts"
    
    read -r line
    
    cat  # Keep the output for QEMU to show up
} < <(qemu-system-aarch64 -kernel iotest.img -machine raspi4b -cpu cortex-a72 -nographic -serial pty -serial none -serial none -serial pty)
