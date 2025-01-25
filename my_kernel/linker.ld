SECTIONS {
	. = 0x80000;            /* start text at this location */
    .text : {
        *(.text.boot)         /* Include .text.boot first */
        *(.text)              /* Include the rest of .text sections */
        *(.text.*)            /* Include sub-sections of .text */
    }
	
	.init_array : {
		__init_array_start = ADDR(.init_array); /* Mark the start of the .init_array section */
		__init_array_end = ADDR(.init_array) + SIZEOF(.init_array); /* Mark the end of the .init_array section */
	}

   .data : { *(.data) } /* Initialized variables */
   .bss : {
		*(.bss)
		. = ALIGN(4); /* Align .bss section to 4-byte boundary */
    }

	 /* Stack allocation */
    . = ALIGN(4);  /* Ensure stack is aligned to a 4-byte boundary */
    .stack : {
        __stack_start = .; /* Mark the start of the stack */
        . = . + 0x4000;    /* Allocate 16KB (0x4000) for the stack */
        __stack_end = .;   /* Mark the end of the stack */
    }
}

