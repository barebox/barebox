
#ifdef CONFIG_ARCH_IMX_INTERNAL_BOOT

#ifdef CONFIG_ARCH_IMX_INTERNAL_BOOT_SERIAL
#define PRE_IMAGE \
	.pre_image : {					\
		KEEP(*(.flash_header_0x0*))		\
		KEEP(*(.dcd_entry_0x0*))		\
		KEEP(*(.image_len_0x0*))		\
		. = 0x400;				\
	}
#else

#define PRE_IMAGE \
	.pre_image : {					\
		KEEP(*(.flash_header_start*))		\
		. = 0x100;				\
		KEEP(*(.flash_header_0x0100*))		\
		KEEP(*(.dcd_entry_0x0100*))		\
		KEEP(*(.image_len_0x0100*))		\
		. = 0x400;				\
		KEEP(*(.flash_header_0x0400*))		\
		KEEP(*(.dcd_entry_0x0400*))		\
		KEEP(*(.image_len_0x0400*))		\
		. = 0x1000;				\
		KEEP(*(.flash_header_0x1000*))		\
		KEEP(*(.dcd_entry_0x1000*))		\
		KEEP(*(.image_len_0x1000*))		\
		. = 0x2000;				\
	}
#endif
#endif
