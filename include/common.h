/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

#include <stdio.h>
#include <compiler.h>

#include <config.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/common.h>

#ifdef	DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);
#else
#define debug(fmt,args...)
#define debugX(level,fmt,args...)
#endif	/* DEBUG */

#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)

typedef void (interrupt_handler_t)(void *);

#include <asm/u-boot.h> /* boot information for Linux kernel */

/*
 * General Purpose Utilities
 */
#define min(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x < __y) ? __x : __y; })

#define max(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
		(__x > __y) ? __x : __y; })


/*
 * Function Prototypes
 */
void reginfo(void);

void	hang (void) __attribute__ ((noreturn));
void	panic(const char *fmt, ...);

/* */
long int initdram (int);
int	display_options (void);
void	print_size (ulong, const char *);

/* common/main.c */
void	main_loop	(void);
int	run_command	(const char *cmd, int flag);
int	readline	(const char *prompt, char *buf, int len);
void	reset_cmd_timeout(void);

/* lib_$(ARCH)/board.c */
int	checkboard    (void);
int	checkflash    (void);
int	checkdram     (void);
int	checkcpu      (void);
char *	strmhz(char *buf, long hz);

#ifdef CONFIG_AUTO_COMPLETE
int env_complete(char *var, int maxv, char *cmdv[], int maxsz, char *buf);
#endif

/* common/exports.c */
void	jumptable_init(void);

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

#ifdef CONFIG_LWMON
extern uchar pic_read  (uchar reg);
extern void  pic_write (uchar reg, uchar val);
#endif

#if defined(CFG_DRAM_TEST)
int testdram(void);
#endif /* CFG_DRAM_TEST */

/* $(CPU)/cpu.c */
void	reset_cpu     (ulong addr);

/* $(CPU)/interrupts.c */
//void	timer_interrupt	   (struct pt_regs *);
//void	external_interrupt (struct pt_regs *);
void	irq_install_handler(int, interrupt_handler_t *, void *);
void	irq_free_handler   (int);
#ifdef CONFIG_INTERRUPTS
void	enable_interrupts  (void);
int	disable_interrupts (void);
#else
#define enable_interrupts() do {} while (0)
#define disable_interrupts() 0
#endif

/* lib_$(ARCH)/time.c */
void	udelay (unsigned long);
void	mdelay (unsigned long);

int gunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp);

/* lib_generic/vsprintf.c */
ulong	simple_strtoul(const char *cp,char **endp,unsigned int base);
#ifdef CFG_64BIT_VSPRINTF
unsigned long long	simple_strtoull(const char *cp,char **endp,unsigned int base);
#endif
long	simple_strtol(const char *cp,char **endp,unsigned int base);

/* lib_generic/crc32.c */
ulong crc32 (ulong, const unsigned char *, uint);
ulong crc32_no_comp (ulong, const unsigned char *, uint);

/* common/console.c */
int	ctrlc (void);

#ifdef CONFIG_SHOW_BOOT_PROGRESS
void	show_boot_progress (int status);
#endif

#define MEMAREA_SIZE_SPECIFIED 1

struct memarea_info {
        struct device_d *device;
	unsigned long start;
	unsigned long end;
	unsigned long size;
        unsigned long flags;
};

int spec_str_to_info(const char *str, struct memarea_info *info);
int parse_area_spec(const char *str, ulong *start, ulong *size);

/* Just like simple_strtoul(), but this one honors a K/M/G suffix */
unsigned long strtoul_suffix(const char *str, char **endp, int base);

extern void start_uboot(void);

#endif	/* __COMMON_H_ */
