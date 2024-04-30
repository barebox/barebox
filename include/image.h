/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2000-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 ********************************************************************
 * NOTE: This header file defines an interface to barebox. Including
 * this (unmodified) header file in another file is considered normal
 * use of barebox, and does *not* fall under the heading of "derived
 * work".
 ********************************************************************
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#ifdef __BAREBOX__
#include <linux/types.h>
#include <asm/byteorder.h>
#include <stdio.h>
#include <string.h>
#endif

/*
 * Operating System Codes
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum {
	IH_OS_INVALID		= 0,	/* Invalid OS	*/
	IH_OS_OPENBSD,			/* OpenBSD	*/
	IH_OS_NETBSD,			/* NetBSD	*/
	IH_OS_FREEBSD,			/* FreeBSD	*/
	IH_OS_4_4BSD,			/* 4.4BSD	*/
	IH_OS_LINUX,			/* Linux	*/
	IH_OS_SVR4,			/* SVR4		*/
	IH_OS_ESIX,			/* Esix		*/
	IH_OS_SOLARIS,			/* Solaris	*/
	IH_OS_IRIX,			/* Irix		*/
	IH_OS_SCO,			/* SCO		*/
	IH_OS_DELL,			/* Dell		*/
	IH_OS_NCR,			/* NCR		*/
	IH_OS_LYNXOS,			/* LynxOS	*/
	IH_OS_VXWORKS,			/* VxWorks	*/
	IH_OS_PSOS,			/* pSOS		*/
	IH_OS_QNX,			/* QNX		*/
	IH_OS_BAREBOX,			/* Firmware	*/
	IH_OS_RTEMS,			/* RTEMS	*/
	IH_OS_ARTOS,			/* ARTOS	*/
	IH_OS_UNITY,			/* Unity OS	*/
	IH_OS_INTEGRITY,		/* INTEGRITY	*/
	IH_OS_OSE,			/* OSE		*/
	IH_OS_PLAN9,			/* Plan 9	*/
	IH_OS_OPENRTOS,		/* OpenRTOS	*/
	IH_OS_ARM_TRUSTED_FIRMWARE,     /* ARM Trusted Firmware */
	IH_OS_TEE,			/* Trusted Execution Environment */
	IH_OS_OPENSBI,			/* RISC-V OpenSBI */
	IH_OS_EFI,			/* EFI Firmware (e.g. GRUB2) */

	IH_OS_COUNT,
};

/*
 * CPU Architecture Codes (supported by Linux)
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum {
	IH_ARCH_INVALID		= 0,	/* Invalid CPU	*/
	IH_ARCH_ALPHA,			/* Alpha	*/
	IH_ARCH_ARM,			/* ARM		*/
	IH_ARCH_I386,			/* Intel x86	*/
	IH_ARCH_IA64,			/* IA64		*/
	IH_ARCH_MIPS,			/* MIPS		*/
	IH_ARCH_MIPS64,			/* MIPS	 64 Bit */
	IH_ARCH_PPC,			/* PowerPC	*/
	IH_ARCH_S390,			/* IBM S390	*/
	IH_ARCH_SH,			/* SuperH	*/
	IH_ARCH_SPARC,			/* Sparc	*/
	IH_ARCH_SPARC64,		/* Sparc 64 Bit */
	IH_ARCH_M68K,			/* M68K		*/
	IH_ARCH_NIOS,			/* Nios-32	*/
	IH_ARCH_MICROBLAZE,		/* MicroBlaze   */
	IH_ARCH_NIOS2,			/* Nios-II	*/
	IH_ARCH_BLACKFIN,		/* Blackfin	*/
	IH_ARCH_AVR32,			/* AVR32	*/
	IH_ARCH_ST200,			/* STMicroelectronics ST200  */
	IH_ARCH_SANDBOX,		/* Sandbox architecture (test only) */
	IH_ARCH_NDS32,			/* ANDES Technology - NDS32  */
	IH_ARCH_OPENRISC,		/* OpenRISC 1000  */
	IH_ARCH_ARM64,			/* ARM64	*/
	IH_ARCH_ARC,			/* Synopsys DesignWare ARC */
	IH_ARCH_X86_64,			/* AMD x86_64, Intel and Via */
	IH_ARCH_XTENSA,			/* Xtensa	*/
	IH_ARCH_RISCV,			/* RISC-V */

	IH_ARCH_COUNT,
};

#if defined(__PPC__)
#define IH_ARCH IH_ARCH_PPC
#elif defined(__ARM__)
#ifdef CONFIG_CPU_64v8
#define IH_ARCH IH_ARCH_ARM64
#else
#define IH_ARCH IH_ARCH_ARM
#endif
#elif defined(__I386__) || defined(__x86_64__) || defined(__i386__)
#define IH_ARCH IH_ARCH_I386
#elif defined(__mips__)
#define IH_ARCH IH_ARCH_MIPS
#elif defined(__nios__)
#define IH_ARCH IH_ARCH_NIOS
#elif defined(__m68k__)
#define IH_ARCH IH_ARCH_M68K
#elif defined(__microblaze__)
#define IH_ARCH IH_ARCH_MICROBLAZE
#elif defined(__nios2__)
#define IH_ARCH IH_ARCH_NIOS2
#elif defined(__OR1K__)
#define IH_ARCH IH_ARCH_OPENRISC
#elif defined(__blackfin__)
#define IH_ARCH IH_ARCH_BLACKFIN
#elif defined(__avr32__)
#define IH_ARCH IH_ARCH_AVR32
#elif defined(CONFIG_SANDBOX)
#define IH_ARCH IH_ARCH_SANDBOX
#elif defined(__riscv)
#define IH_ARCH IH_ARCH_RISCV
#else
#define IH_ARCH IH_ARCH_INVALID
#endif

/*
 * Image Types
 *
 * "Standalone Programs" are directly runnable in the environment
 *	provided by barebox; it is expected that (if they behave
 *	well) you can continue to work in barebox after return from
 *	the Standalone Program.
 * "OS Kernel Images" are usually images of some Embedded OS which
 *	will take over control completely. Usually these programs
 *	will install their own set of exception handlers, device
 *	drivers, set up the MMU, etc. - this means, that you cannot
 *	expect to re-enter barebox except by resetting the CPU.
 * "RAMDisk Images" are more or less just data blocks, and their
 *	parameters (address, size) are passed to an OS kernel that is
 *	being started.
 * "Multi-File Images" contain several images, typically an OS
 *	(Linux) kernel image and one or more data images like
 *	RAMDisks. This construct is useful for instance when you want
 *	to boot over the network using BOOTP etc., where the boot
 *	server provides just a single image file, but you want to get
 *	for instance an OS kernel and a RAMDisk image.
 *
 *	"Multi-File Images" start with a list of image sizes, each
 *	image size (in bytes) specified by an "uint32_t" in network
 *	byte order. This list is terminated by an "(uint32_t)0".
 *	Immediately after the terminating 0 follow the images, one by
 *	one, all aligned on "uint32_t" boundaries (size rounded up to
 *	a multiple of 4 bytes - except for the last file).
 *
 * "Firmware Images" are binary images containing firmware (like
 *	barebox or FPGA images) which usually will be programmed to
 *	flash memory.
 *
 * "Script files" are command sequences that will be executed by
 *	barebox's command interpreter; this feature is especially
 *	useful when you configure barebox to use a real shell (hush)
 *	as command interpreter (=> Shell Scripts).
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */

enum {
	IH_TYPE_INVALID		= 0,	/* Invalid Image		*/
	IH_TYPE_STANDALONE,		/* Standalone Program		*/
	IH_TYPE_KERNEL,			/* OS Kernel Image		*/
	IH_TYPE_RAMDISK,		/* RAMDisk Image		*/
	IH_TYPE_MULTI,			/* Multi-File Image		*/
	IH_TYPE_FIRMWARE,		/* Firmware Image		*/
	IH_TYPE_SCRIPT,			/* Script file			*/
	IH_TYPE_FILESYSTEM,		/* Filesystem Image (any type)	*/
	IH_TYPE_FLATDT,			/* Binary Flat Device Tree Blob	*/
	IH_TYPE_KWBIMAGE,		/* Kirkwood Boot Image		*/
	IH_TYPE_IMXIMAGE,		/* Freescale IMXBoot Image	*/
	IH_TYPE_UBLIMAGE,		/* Davinci UBL Image		*/
	IH_TYPE_OMAPIMAGE,		/* TI OMAP Config Header Image	*/
	IH_TYPE_AISIMAGE,		/* TI Davinci AIS Image		*/
	/* OS Kernel Image, can run from any load address */
	IH_TYPE_KERNEL_NOLOAD,
	IH_TYPE_PBLIMAGE,		/* Freescale PBL Boot Image	*/
	IH_TYPE_MXSIMAGE,		/* Freescale MXSBoot Image	*/
	IH_TYPE_GPIMAGE,		/* TI Keystone GPHeader Image	*/
	IH_TYPE_ATMELIMAGE,		/* ATMEL ROM bootable Image	*/
	IH_TYPE_SOCFPGAIMAGE,		/* Altera SOCFPGA CV/AV Preloader */
	IH_TYPE_X86_SETUP,		/* x86 setup.bin Image		*/
	IH_TYPE_LPC32XXIMAGE,		/* x86 setup.bin Image		*/
	IH_TYPE_LOADABLE,		/* A list of typeless images	*/
	IH_TYPE_RKIMAGE,		/* Rockchip Boot Image		*/
	IH_TYPE_RKSD,			/* Rockchip SD card		*/
	IH_TYPE_RKSPI,			/* Rockchip SPI image		*/
	IH_TYPE_ZYNQIMAGE,		/* Xilinx Zynq Boot Image */
	IH_TYPE_ZYNQMPIMAGE,		/* Xilinx ZynqMP Boot Image */
	IH_TYPE_ZYNQMPBIF,		/* Xilinx ZynqMP Boot Image (bif) */
	IH_TYPE_FPGA,			/* FPGA Image */
	IH_TYPE_VYBRIDIMAGE,	/* VYBRID .vyb Image */
	IH_TYPE_TEE,            /* Trusted Execution Environment OS Image */
	IH_TYPE_FIRMWARE_IVT,		/* Firmware Image with HABv4 IVT */
	IH_TYPE_PMMC,            /* TI Power Management Micro-Controller Firmware */
	IH_TYPE_STM32IMAGE,		/* STMicroelectronics STM32 Image */
	IH_TYPE_SOCFPGAIMAGE_V1,	/* Altera SOCFPGA A10 Preloader	*/
	IH_TYPE_MTKIMAGE,		/* MediaTek BootROM loadable Image */
	IH_TYPE_IMX8MIMAGE,		/* Freescale IMX8MBoot Image	*/
	IH_TYPE_IMX8IMAGE,		/* Freescale IMX8Boot Image	*/
	IH_TYPE_COPRO,			/* Coprocessor Image for remoteproc*/

	IH_TYPE_COUNT,			/* Number of image types */
};

/*
 * Compression Types
 *
 * The following are exposed to uImage header.
 * New IDs *MUST* be appended at the end of the list and *NEVER*
 * inserted for backward compatibility.
 */
enum {
	IH_COMP_NONE		= 0,	/*  No	 Compression Used	*/
	IH_COMP_GZIP,			/* gzip	 Compression Used	*/
	IH_COMP_BZIP2,			/* bzip2 Compression Used	*/

	IH_COMP_COUNT,
};

#define IH_MAGIC	0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32	/* Image Name Length		*/

/*
 * all data in network byte order (aka natural aka bigendian)
 */

typedef struct image_header {
	uint32_t	ih_magic;	/* Image Header Magic Number	*/
	uint32_t	ih_hcrc;	/* Image Header CRC Checksum	*/
	uint32_t	ih_time;	/* Image Creation Timestamp	*/
	uint32_t	ih_size;	/* Image Data Size		*/
	uint32_t	ih_load;	/* Data	 Load  Address		*/
	uint32_t	ih_ep;		/* Entry Point Address		*/
	uint32_t	ih_dcrc;	/* Image Data CRC Checksum	*/
	uint8_t		ih_os;		/* Operating System		*/
	uint8_t		ih_arch;	/* CPU architecture		*/
	uint8_t		ih_type;	/* Image Type			*/
	uint8_t		ih_comp;	/* Compression Type		*/
	uint8_t		ih_name[IH_NMLEN];	/* Image Name		*/
} image_header_t;

#if defined(CONFIG_BOOTM_SHOW_TYPE) || !defined(__BAREBOX__)
const char *image_get_os_name(uint8_t os);
const char *image_get_arch_name(uint8_t arch);
const char *image_get_type_name(uint8_t type);
const char *image_get_comp_name(uint8_t comp);
#else
static inline const char *image_get_os_name(uint8_t os)
{
	return NULL;
}

static inline const char *image_get_arch_name(uint8_t arch)
{
	return NULL;
}

static inline const char *image_get_type_name(uint8_t type)
{
	return NULL;
}

static inline const char *image_get_comp_name(uint8_t comp)
{
	return NULL;
}
#endif

#define uimage_to_cpu(x)		be32_to_cpu(x)
#define cpu_to_uimage(x)		cpu_to_be32(x)

struct uimage_handle_data {
	size_t offset; /* offset in the image */
	ulong len;
};

struct uimage_handle *uimage_open(const char *filename);
void uimage_close(struct uimage_handle *handle);
int uimage_verify(struct uimage_handle *handle);
int uimage_load(struct uimage_handle *handle, unsigned int image_no,
		long(*flush)(void*, unsigned long));
void uimage_print_contents(struct uimage_handle *handle);
ssize_t uimage_get_size(struct uimage_handle *handle, unsigned int image_no);
struct resource *uimage_load_to_sdram(struct uimage_handle *handle,
		int image_no, unsigned long load_address);
void *uimage_load_to_buf(struct uimage_handle *handle, int image_no,
		size_t *size);
#define MAX_MULTI_IMAGE_COUNT 16

struct uimage_handle {
	struct image_header header;
	char *name;
	char *copy;
	struct uimage_handle_data ihd[MAX_MULTI_IMAGE_COUNT];
	int nb_data_entries;
	size_t data_offset;
	int fd;
};

#define UIMAGE_INVALID_ADDRESS	(~0)

#endif	/* __IMAGE_H__ */
