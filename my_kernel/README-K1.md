## 1.0 Overview
This program is written for K1 and implements the desired functionality: 
- FirstUserTask runs and spawns four other tasks, two of lower priority, and two of higher than the FirstUserTask.
- after spawning each task, FirstUserTask prints the task it created, and calls exit() upon completion
- the spawned tasks print their id and parent id, yield, print again and then exit

Note that this is done by issuing system calls to change EL and context switching.
## 2.0 Program Operation

### 2.1 Compilation

The following sequence of steps compiles this source code into an elf image file -- note that the code has been written for the Arm Cortex A72.

To compile the code, navigate from the root directory to my_kernel

`cd my_kernel`
followed by 
`make`

This should produce `"iotest.elf"` and `"iotest.img"`. 

In order to upload the compiled image to a rasberry pi and issue the following command:

`./upload.sh iotest.img <d8:3a:dd:1b:36:9e>`
 (replace with desired MAC address).
