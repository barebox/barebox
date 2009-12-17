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
#include <module.h>
#include <config.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/common.h>

#define pr_info(fmt, arg...)	printf(fmt, ##arg)
#define pr_notice(fmt, arg...)	printf(fmt, ##arg)
#define pr_err(fmt, arg...)	printf(fmt, ##arg)
#define pr_warning(fmt, arg...)	printf(fmt, ##arg)
#define pr_crit(fmt, arg...)	printf(fmt, ##arg)
#define pr_alert(fmt, arg...)	printf(fmt, ##arg)
#define pr_emerg(fmt, arg...)	printf(fmt, ##arg)

#ifdef DEBUG
#define pr_debug(fmt, arg...)	printf(fmt, ##arg)
#else
#define pr_debug(fmt, arg...) do {} while(0)
#endif

#define debug(fmt, arg...)	pr_debug(fmt, ##arg)

#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)

typedef void (interrupt_handler_t)(void *);

#include <asm/barebox.h> /* boot information for Linux kernel */

/*
 * Function Prototypes
 */
void reginfo(void);

void	hang (void) __attribute__ ((noreturn));
void	panic(const char *fmt, ...);

/* */
long int initdram (int);
char *size_human_readable(ulong size);

/* common/main.c */
int	run_command	(const char *cmd, int flag);
int	readline	(const char *prompt, char *buf, int len);

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

/* $(CPU)/cpu.c */
void	reset_cpu     (ulong addr);

/* $(CPU)/interrupts.c */
//void	timer_interrupt	   (struct pt_regs *);
//void	external_interrupt (struct pt_regs *);
void	irq_install_handler(int, interrupt_handler_t *, void *);
void	irq_free_handler   (int);
#ifdef CONFIG_USE_IRQ
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

void start_barebox(void);
void shutdown_barebox(void);

int arch_execute(void *, int argc, char *argv[]);

int run_shell(void);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define USHORT_MAX	((u16)(~0U))
#define SHORT_MAX	((s16)(USHORT_MAX>>1))
#define SHORT_MIN	(-SHORT_MAX - 1)
#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define LONG_MIN	(-LONG_MAX - 1)
#define ULONG_MAX	(~0UL)
#define LLONG_MAX	((long long)(~0ULL>>1))
#define LLONG_MIN	(-LLONG_MAX - 1)
#define ULLONG_MAX	(~0ULL)

#endif	/* __COMMON_H_ */
