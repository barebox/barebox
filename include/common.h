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
#include <asm/ptrace.h>

#include <part.h>

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
#include <asm/global_data.h>	/* global data used for startup functions */

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
void do_reset(void);
void reginfo(void);

void	hang		(void) __attribute__ ((noreturn));

/* */
long int initdram (int);
int	display_options (void);
void	print_size (ulong, const char *);

/* common/main.c */
void	main_loop	(void);
int	run_command	(const char *cmd, int flag);
int	readline	(const char *prompt, char *buf, int len);
void	init_cmd_timeout(void);
void	reset_cmd_timeout(void);

/* lib_$(ARCH)/board.c */
void	board_init_f  (ulong);
//void	board_init_r  (gd_t *, ulong);
int	checkboard    (void);
int	checkflash    (void);
int	checkdram     (void);
char *	strmhz(char *buf, long hz);
int	last_stage_init(void);
extern ulong monitor_flash_len;

/* common/cmd_autoscript.c */
int	autoscript (ulong addr);


extern ulong load_addr;		/* Default Load Address */

#ifdef CONFIG_AUTO_COMPLETE
int env_complete(char *var, int maxv, char *cmdv[], int maxsz, char *buf);
#endif

int	misc_init_f   (void);
int	misc_init_r   (void);

/* common/exports.c */
void	jumptable_init(void);

/* common/memsize.c */
long	get_ram_size  (volatile long *, long);

/* $(BOARD)/eeprom.c */
void eeprom_init  (void);
#ifndef CONFIG_SPI
int  eeprom_probe (unsigned dev_addr, unsigned offset);
#endif
int  eeprom_read  (unsigned dev_addr, unsigned offset, uchar *buffer, unsigned cnt);
int  eeprom_write (unsigned dev_addr, unsigned offset, uchar *buffer, unsigned cnt);
#ifdef CONFIG_LWMON
extern uchar pic_read  (uchar reg);
extern void  pic_write (uchar reg, uchar val);
#endif

/*
 * Set this up regardless of board
 * type, to prevent errors.
 */
#if defined(CONFIG_SPI) || !defined(CFG_I2C_EEPROM_ADDR)
# define CFG_DEF_EEPROM_ADDR 0
#else
# define CFG_DEF_EEPROM_ADDR CFG_I2C_EEPROM_ADDR
#endif /* CONFIG_SPI || !defined(CFG_I2C_EEPROM_ADDR) */

#if defined(CONFIG_SPI)
extern void spi_init_f (void);
extern void spi_init_r (void);
extern ssize_t spi_read	 (uchar *, int, uchar *, int);
extern ssize_t spi_write (uchar *, int, uchar *, int);
#endif

#if defined(CFG_DRAM_TEST)
int testdram(void);
#endif /* CFG_DRAM_TEST */

/* $(CPU)/start.S */
#if defined(CONFIG_5xx) || \
    defined(CONFIG_8xx)
uint	get_immr      (uint);
#endif
uint	get_pir	      (void);
#if defined(CONFIG_MPC5xxx)
uint	get_svr       (void);
#endif
uint	get_pvr	      (void);
uint	get_svr	      (void);
uint	rd_ic_cst     (void);
void	wr_ic_cst     (uint);
void	wr_ic_adr     (uint);
uint	rd_dc_cst     (void);
void	wr_dc_cst     (uint);
void	wr_dc_adr     (uint);
int	icache_status (void);
void	icache_enable (void);
void	icache_disable(void);
int	dcache_status (void);
void	dcache_enable (void);
void	dcache_disable(void);
void	relocate_code (ulong, gd_t *, ulong);
ulong	get_endaddr   (void);
void	trap_init     (ulong);
#if defined (CONFIG_4xx)	|| \
    defined (CONFIG_MPC5xxx)	|| \
    defined (CONFIG_74xx_7xx)	|| \
    defined (CONFIG_74x)	|| \
    defined (CONFIG_75x)	|| \
    defined (CONFIG_74xx)	|| \
    defined (CONFIG_MPC8220)	|| \
    defined (CONFIG_MPC85xx)	|| \
    defined (CONFIG_MPC86xx)	|| \
    defined (CONFIG_MPC83XX)
unsigned char	in8(unsigned int);
void		out8(unsigned int, unsigned char);
unsigned short	in16(unsigned int);
unsigned short	in16r(unsigned int);
void		out16(unsigned int, unsigned short value);
void		out16r(unsigned int, unsigned short value);
unsigned long	in32(unsigned int);
unsigned long	in32r(unsigned int);
void		out32(unsigned int, unsigned long value);
void		out32r(unsigned int, unsigned long value);
void		ppcDcbf(unsigned long value);
void		ppcDcbi(unsigned long value);
void		ppcSync(void);
void		ppcDcbz(unsigned long value);
#endif

/* $(CPU)/cpu.c */
int	checkcpu      (void);
int	checkicache   (void);
int	checkdcache   (void);
void	upmconfig     (unsigned int, unsigned int *, unsigned int);
ulong	get_tbclk     (void);
void	reset_cpu     (ulong addr);

/* $(CPU)/speed.c */
int	get_clocks (void);
int	get_clocks_866 (void);
int	sdram_adjust_866 (void);
int	adjust_sdram_tbs_8xx (void);
#if defined(CONFIG_8260)
int	prt_8260_clks (void);
#elif defined(CONFIG_MPC83XX)
int print_clock_conf(void);
#elif defined(CONFIG_MPC5xxx)
int	prt_mpc5xxx_clks (void);
#endif
#if defined(CONFIG_MPC8220)
int	prt_mpc8220_clks (void);
#endif
#ifdef CONFIG_4xx
ulong	get_OPB_freq (void);
ulong	get_PCI_freq (void);
#endif
#if defined(CONFIG_S3C2400) || defined(CONFIG_S3C2410) || defined(CONFIG_LH7A40X)
ulong	get_FCLK (void);
ulong	get_HCLK (void);
ulong	get_PCLK (void);
ulong	get_UCLK (void);
#endif
#if defined(CONFIG_LH7A40X)
ulong	get_PLLCLK (void);
#endif
#if defined CONFIG_INCA_IP
uint	incaip_get_cpuclk (void);
#endif
ulong	get_bus_freq  (ulong);

#if defined(CONFIG_MPC85xx)
typedef MPC85xx_SYS_INFO sys_info_t;
void	get_sys_info  ( sys_info_t * );
#endif
#if defined(CONFIG_MPC86xx)
typedef MPC86xx_SYS_INFO sys_info_t;
void   get_sys_info  ( sys_info_t * );
#endif

#if defined(CONFIG_4xx) || defined(CONFIG_IOP480)
#  if defined(CONFIG_440)
    typedef PPC440_SYS_INFO sys_info_t;
#	if defined(CONFIG_440SPE)
	 unsigned long determine_sysper(void);
	 unsigned long determine_pci_clock_per(void);
	 int ppc440spe_revB(void);
#	endif
#  else
    typedef PPC405_SYS_INFO sys_info_t;
#  endif
void	get_sys_info  ( sys_info_t * );
#endif

/* $(CPU)/cpu_init.c */
#if defined(CONFIG_8xx) || defined(CONFIG_8260)
void	cpu_init_f    (volatile immap_t *immr);
#endif
#if defined(CONFIG_4xx) || defined(CONFIG_MPC85xx) || defined(CONFIG_MCF52x2) ||defined(CONFIG_MPC86xx)
void	cpu_init_f    (void);
#endif

int	cpu_init_r    (void);
#if defined(CONFIG_8260)
int	prt_8260_rsr  (void);
#endif

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

/* $(CPU)/.../commproc.c */
int	dpram_init (void);
uint	dpram_base(void);
uint	dpram_base_align(uint align);
uint	dpram_alloc(uint size);
uint	dpram_alloc_align(uint size,uint align);
void	post_word_store (ulong);
ulong	post_word_load (void);
void	bootcount_store (ulong);
ulong	bootcount_load (void);
#define BOOTCOUNT_MAGIC		0xB001C041

/* $(CPU)/.../<eth> */
void mii_init (void);

/* $(CPU)/.../lcd.c */
ulong	lcd_setmem (ulong);

/* $(CPU)/.../vfd.c */
ulong	vfd_setmem (ulong);

/* $(CPU)/.../video.c */
ulong	video_setmem (ulong);

/* lib_$(ARCH)/cache.c */
void	flush_cache   (unsigned long, unsigned long);


/* lib_$(ARCH)/ticks.S */
unsigned long long get_ticks(void);
void	wait_ticks    (unsigned long);

/* lib_$(ARCH)/time.c */
int	udelay	      (unsigned long);

/* lib_generic/vsprintf.c */
ulong	simple_strtoul(const char *cp,char **endp,unsigned int base);
#ifdef CFG_64BIT_VSPRINTF
unsigned long long	simple_strtoull(const char *cp,char **endp,unsigned int base);
#endif
long	simple_strtol(const char *cp,char **endp,unsigned int base);
void	panic(const char *fmt, ...);

/* lib_generic/crc32.c */
ulong crc32 (ulong, const unsigned char *, uint);
ulong crc32_no_comp (ulong, const unsigned char *, uint);

/* common/console.c */
int	ctrlc (void);

#ifdef CONFIG_SHOW_BOOT_PROGRESS
void	show_boot_progress (int status);
#endif

/* Reservoir for several functions in the code where
 * previously no header file existed
 */
int cmd_get_data_size(char* arg, int default_size);

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
