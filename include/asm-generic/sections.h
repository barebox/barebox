#ifndef _ASM_GENERIC_SECTIONS_H_
#define _ASM_GENERIC_SECTIONS_H_

extern char _text[], _stext[], _etext[];
extern char __bss_start[], __bss_stop[];
extern char _end[];
extern void *_barebox_image_size;

#define barebox_image_size	(unsigned int)&_barebox_image_size

#endif /* _ASM_GENERIC_SECTIONS_H_ */
