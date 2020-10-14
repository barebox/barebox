/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

#include <stdio.h>
#include <module.h>
#include <config.h>
#include <clock.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <asm/common.h>
#include <asm/io.h>
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

/*
 * Function Prototypes
 */
void reginfo(void);

void __noreturn hang (void);

char *size_human_readable(unsigned long long size);

int	readline	(const char *prompt, char *buf, int len);

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

/* common/console.c */
int ctrlc(void);
int arch_ctrlc(void);

#ifdef CONFIG_CONSOLE_FULL
void ctrlc_handled(void);
#else
static inline void ctrlc_handled(void) { }
#endif

#ifdef CONFIG_ARCH_HAS_STACK_DUMP
void dump_stack(void);
#else
static inline void dump_stack(void)
{
	printf("no stack data available\n");
}
#endif

int parse_area_spec(const char *str, loff_t *start, loff_t *size);

/* Just like simple_strtoul(), but this one honors a K/M/G suffix */
unsigned long strtoul_suffix(const char *str, char **endp, int base);
unsigned long long strtoull_suffix(const char *str, char **endp, int base);

/*
 * Function pointer to the main barebox function. Defaults
 * to run_shell() when a shell is enabled.
 */
extern int (*barebox_main)(void);

enum autoboot_state {
	AUTOBOOT_COUNTDOWN,
	AUTOBOOT_ABORT,
	AUTOBOOT_MENU,
	AUTOBOOT_BOOT,
	AUTOBOOT_UNKNOWN,
};

void set_autoboot_state(enum autoboot_state autoboot);
enum autoboot_state do_autoboot_countdown(void);

void __noreturn start_barebox(void);
void shutdown_barebox(void);

#define ARRAY_AND_SIZE(x)	(x), ARRAY_SIZE(x)

/*
 * The STACK_ALIGN_ARRAY macro is used to allocate a buffer on the stack that
 * meets a minimum alignment requirement.
 *
 * Note that the size parameter is the number of array elements to allocate,
 * not the number of bytes.
 */
#define STACK_ALIGN_ARRAY(type, name, size, align)		\
	char __##name[sizeof(type) * (size) + (align) - 1];	\
	type *name = (type *)ALIGN((uintptr_t)__##name, align)

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12
#define PAGE_ALIGN(s)	ALIGN(s, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(x) ALIGN_DOWN(x, PAGE_SIZE)

int mem_parse_options(int argc, char *argv[], char *optstr, int *mode,
		char **sourcefile, char **destfile, int *swab);
int memcpy_parse_options(int argc, char *argv[], int *sourcefd,
			 int *destfd, loff_t *count,
			 int rwsize, int destmode);
#define RW_BUF_SIZE	(unsigned)4096

extern const char version_string[];
extern const char release_string[];
extern const char buildsystem_version_string[];
#ifdef CONFIG_BANNER
void barebox_banner(void);
#else
static inline void barebox_banner(void) {}
#endif

const char *barebox_get_model(void);
void barebox_set_model(const char *);
const char *barebox_get_hostname(void);
void barebox_set_hostname(const char *);
void barebox_set_hostname_no_overwrite(const char *);

#endif	/* __COMMON_H_ */
