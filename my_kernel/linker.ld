SECTIONS {
	. = 0x80000;            /* start text at this location */
	.text.boot : {          /* boot code must come first */
		KEEP(*(.text.boot))
	}                       /* rest: follow defaults */
   .init_array : {
		__init_array_start = ADDR(.init_array); /* Mark the start of the .init_array section */
		__init_array_end = ADDR(.init_array) + SIZEOF(.init_array); /* Mark the end of the .init_array section */
	}
}
