## 1.0 Overview

This program is written for K3 and implements the desired functionality:

- FirstUserTask (FUT) runs and creates a name server, clock server, clock-notifier and client tasks that requests paramaters from FUT and delays according to the paramaters passed back.

## 2.0 Program Operation

### 2.1 Compilation

The following sequence of steps compiles this source code into an elf image file -- note that the code has been written for the Arm Cortex A72.

To compile the code, navigate from the root directory to my_kernel

`cd my_kernel`
followed by
`make`

This should produce `"iotest.elf"` and `"iotest.img"`. 

Now navigate to the scripts directory
`cd scripts`

In order to upload the compiled image to a rasberry pi, issue the following command:

`./upload.sh ../iotest.img <d8:3a:dd:1b:36:9e>`
(replace with desired MAC address).
