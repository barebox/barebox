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
#include <linux/stddef.h>
#include <asm/common.h>
#include <printk.h>

/*
 * sanity check. The Linux Kernel defines only one of __LITTLE_ENDIAN and
 * __BIG_ENDIAN. Endianess can then be tested with #ifdef __xx_ENDIAN. Userspace
 * always defined both __LITTLE_ENDIAN and __BIG_ENDIAN and byteorder can then
 * be tested with #if __BYTE_ORDER == __xx_ENDIAN.
 *
 * As we tend to use a lot of Kernel code in barebox we use the kernel way of
 * determing the byte order. Make sure here that architecture code properly
 * defines it.
 */
#include <asm/byteorder.h>
#if defined __LITTLE_ENDIAN && defined __BIG_ENDIAN
#error "both __LITTLE_ENDIAN and __BIG_ENDIAN are defined"
#endif
#if !defined __LITTLE_ENDIAN && !defined __BIG_ENDIAN
#error "None of __LITTLE_ENDIAN and __BIG_ENDIAN are defined"
#endif

#define BUG() do { \
	printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)


#define __WARN() do { 								\
	printf("WARNING: at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__);	\
} while (0)

#ifndef WARN_ON
#define WARN_ON(condition) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN();						\
	unlikely(__ret_warn_on);					\
})
#endif

#ifndef WARN
#define WARN(condition, format...) ({					\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN();						\
		puts("WARNING: ");					\
		printf(format);						\
	unlikely(__ret_warn_on);					\
})
#endif

#include <asm/barebox.h> /* boot information for Linux kernel */

/*
 * Function Prototypes
 */
void reginfo(void);

void __noreturn hang (void);
void __noreturn panic(const char *fmt, ...);

char *size_human_readable(unsigned long long size);

/* common/main.c */
int	run_command	(const char *cmd, int flag);
int	readline	(const char *prompt, char *buf, int len);

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

/* $(CPU)/cpu.c */
void __noreturn reset_cpu(unsigned long addr);
void __noreturn poweroff(void);

/* lib_$(ARCH)/time.c */
void	udelay (unsigned long);
void	mdelay (unsigned long);

/* lib_generic/vsprintf.c */
ulong	simple_strtoul(const char *cp,char **endp,unsigned int base);
unsigned long long	simple_strtoull(const char *cp,char **endp,unsigned int base);
long	simple_strtol(const char *cp,char **endp,unsigned int base);

/* lib_generic/crc32.c */
uint32_t crc32(uint32_t, const void*, unsigned int);
uint32_t crc32_no_comp(uint32_t, const void*, unsigned int);

/* common/console.c */
int	ctrlc (void);

#ifdef ARCH_HAS_STACK_DUMP
void dump_stack(void);
#else
static inline void dump_stack(void)
{
	printf("no stack data available\n");
}
#endif

#define MEMAREA_SIZE_SPECIFIED 1

struct memarea_info {
	struct device_d *device;
	unsigned long start;
	unsigned long end;
	unsigned long size;
	unsigned long flags;
};

int parse_area_spec(const char *str, loff_t *start, loff_t *size);

/* Just like simple_strtoul(), but this one honors a K/M/G suffix */
unsigned long strtoul_suffix(const char *str, char **endp, int base);
unsigned long long strtoull_suffix(const char *str, char **endp, int base);

/*
 * Function pointer to the main barebox function. Defaults
 * to run_shell() when a shell is enabled.
 */
extern int (*barebox_main)(void);

void __noreturn start_barebox(void);
void shutdown_barebox(void);
extern void (*board_shutdown)(void);

/*
 * architectures which have special calling conventions for
 * executing programs should set this. Used by the 'go' command
 */
extern void (*do_execute)(void *func, int argc, char *argv[]);

void arch_shutdown(void);

int run_shell(void);

/* Force a compilation error if condition is true */
#define BUILD_BUG_ON(condition) ((void)BUILD_BUG_ON_ZERO(condition))

/* Force a compilation error if condition is constant and true */
#define MAYBE_BUILD_BUG_ON(cond) ((void)sizeof(char[1 - 2 * !!(cond)]))

/* Force a compilation error if a constant expression is not a power of 2 */
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)			\
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/*
 * Force a compilation error if condition is true, but also produce a
 * result (of value 0 and type size_t), so the expression can be used
 * e.g. in a structure initializer (or where-ever else comma
 * expressions aren't permitted).
 */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))

#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN_DOWN(x, a)	((x) & ~((typeof(x))(a) - 1))
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#define ARRAY_AND_SIZE(x)	(x), ARRAY_SIZE(x)

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

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12
#define PAGE_ALIGN(s) (((s) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(x) ((x) & ~(PAGE_SIZE - 1))

int memory_display(const void *addr, loff_t offs, unsigned nbytes, int size, int swab);

#define DUMP_PREFIX_OFFSET 0
static inline void print_hex_dump(const char *level, const char *prefix_str,
		int prefix_type, int rowsize, int groupsize,
		const void *buf, size_t len, bool ascii)
{
	memory_display(buf, 0, len, 4, 0);
}

int mem_parse_options(int argc, char *argv[], char *optstr, int *mode,
		char **sourcefile, char **destfile, int *swab);
int open_and_lseek(const char *filename, int mode, loff_t pos);
#define RW_BUF_SIZE	(unsigned)4096

extern const char version_string[];
#ifdef CONFIG_BANNER
void barebox_banner(void);
#else
static inline void barebox_banner(void) {}
#endif

const char *barebox_get_model(void);
void barebox_set_model(const char *);
const char *barebox_get_hostname(void);
void barebox_set_hostname(const char *);

#define IOMEM(addr)	((void __force __iomem *)(addr))

#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))

#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)

#define abs(x) ({                               \
		long __x = (x);                 \
		(__x < 0) ? -__x : __x;         \
	})

#define abs64(x) ({                             \
		s64 __x = (x);                  \
		(__x < 0) ? -__x : __x;         \
	})

/*
 * Check if two regions overlap. returns true if they do, false otherwise
 */
static inline bool region_overlap(unsigned long starta, unsigned long lena,
		unsigned long startb, unsigned long lenb)
{
	if (starta + lena <= startb)
		return 0;
	if (startb + lenb <= starta)
		return 0;
	return 1;
}

#endif	/* __COMMON_H_ */
