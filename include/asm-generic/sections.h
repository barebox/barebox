#ifndef _ASM_GENERIC_SECTIONS_H_
#define _ASM_GENERIC_SECTIONS_H_

extern char _text[], _stext[], _etext[];
extern char __bss_start[], __bss_stop[];
extern char _sdata[], _edata[];
extern char __bare_init_start[], __bare_init_end[];
extern char _end[];
extern char __image_start[];
extern char __image_end[];
extern char __piggydata_start[];
extern void *_barebox_image_size;
extern void *_barebox_bare_init_size;
extern void *_barebox_pbl_size;

/* Start and end of .ctors section - used for constructor calls. */
extern char __ctors_start[], __ctors_end[];

#define barebox_image_size	(__image_end - __image_start)
#define barebox_bare_init_size	(unsigned int)&_barebox_bare_init_size
#define barebox_pbl_size	(__piggydata_start - __image_start)
#endif /* _ASM_GENERIC_SECTIONS_H_ */
