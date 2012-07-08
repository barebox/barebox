
/* the EP93xx expects to find the pattern 'CRUS' at 0x1000 */

#define PRE_IMAGE				\
	.pre_image : {				\
		KEEP(*(.flash_header_start*))	\
		. = 0x1000;			\
		LONG(0x53555243) /* 'CRUS' */	\
	}
