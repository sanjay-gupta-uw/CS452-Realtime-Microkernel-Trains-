## 1.0 Overview

- This program re-implements A0 but using the Kernel developed throughout CS452, and using interrupt handling to avoid spin waiting
- To run with IRQ enabled, set flag IRQEn to 1

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

In order to upload the compiled image to a rasberry pi and issue the following command:

`./upload.sh ../iotest.img <d8:3a:dd:1b:36:9e>`
(replace with desired MAC address).
