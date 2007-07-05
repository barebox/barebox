#define DEBUG

#include <common.h>
#include <command.h>
#include <watchdog.h>
#include <image.h>
#include <environment.h>
#include <asm/global_data.h>
#include <asm/bitops.h>
#include <errno.h>
#include <fs.h>

#ifdef CONFIG_OF_FLAT_TREE
#include <ft_build.h>
#endif
extern bd_t *bd;
#define SHOW_BOOT_PROGRESS(x)

int
do_bootm_linux(image_header_t *os_header, image_header_t *initrd_header, const char *oftree)
{
	ulong	sp;
	ulong	len, checksum;
	ulong	initrd_start, initrd_end = 0;
	ulong	cmd_start, cmd_end;
	ulong	initrd_high;
	ulong	data;
	int	initrd_copy_to_ram = 1;
	char    *cmdline;
	char	*s;
	bd_t	*kbd;
	void	(*kernel)(bd_t *, ulong, ulong, ulong, ulong);
#ifdef CONFIG_OF_FLAT_TREE
	char	*of_flat_tree = NULL;
//	ulong	of_data = 0;
#endif
	void    *os_data = NULL;
	void    *initrd_data = NULL;
	void    *of_data = NULL;
	unsigned long os_len, initrd_len;
	unsigned long *lenp;

	printf("entering %s: os_header: %p initrd_header: %p oftree: %p\n",
			__FUNCTION__, os_header, initrd_header, oftree);

	if (os_header->ih_type == IH_TYPE_MULTI) {
		unsigned long *data = (unsigned long *)(os_header + 1);
		unsigned long len1 = 0, len2 = 0;

		if (!*data) {
			printf("multifile image with 0 entries???\n");
			return -1;
		}
		os_len = ntohl(*data);	/* first one is the kernel */
		data++;

		if (*data) {
			len1 = ntohl(*data);	/* could be initrd or oftree */
			data++;
		}

		if (*data) {
			len2 = ntohl(*data);	/* could be oftree */
			data++;
		}

		while (*data)		/* skip all remaining files */
			data++;

		data++;			/* skip terminating zero */

		os_data = (void *)ntohl(data);

		if (len2) {
			initrd_data = (void *)
				((unsigned long)data + ((os_len + 3) & ~3));
			initrd_len  = len1;
			of_data     = (void *)
				((unsigned long)initrd_data + ((initrd_len + 3) & ~3));
		} else if (len1) {
			/* We could check here if this is a multifile image
			 * with only a kernel and an oftree. Original U-Boot
			 * did not do this, so leave it out for now.
			 */
			initrd_data = (void *)((unsigned long)data + ((os_len + 3) & ~3));
			initrd_len  = len1;
		}
	} else {
		os_data = (void *)(os_header + 1);
		printf("set os_data to %p\n", os_data);
	}

	if (initrd_header)
		initrd_data = (void *)(initrd_header + 1);

#ifdef CONFIG_OF_FLAT_TREE
	if (oftree) {
		image_header_t *oftree_header;
		/* The oftree can be given either as an uboot image or as a
		 * binary blob. First try to read it as an image.
		 */
		oftree_header = map_image(oftree, 1);
		if (oftree_header) {
			of_data = (void *)(oftree_header + 1);
		} else {
			of_data = read_file(oftree);
			if (!of_data) {
				printf("could not read %s: %s\n", oftree, errno_str());
				return -1;
			}
		}
		/* FIXME: check if this is really an oftree */
	}
#endif
	printf("loading kernel.\n");

	if ((s = getenv ("initrd_high")) != NULL) {
		/* a value of "no" or a similar string will act like 0,
		 * turning the "load high" feature off. This is intentional.
		 */
		initrd_high = simple_strtoul(s, NULL, 16);
		if (initrd_high == ~0)
			initrd_copy_to_ram = 0;
	} else {	/* not set, no restrictions to load high */
		initrd_high = ~0;
	}

#ifdef CONFIG_LOGBUFFER
	kbd=gd->bd;
	/* Prevent initrd from overwriting logbuffer */
	if (initrd_high < (kbd->bi_memsize-LOGBUFF_LEN-LOGBUFF_OVERHEAD))
		initrd_high = kbd->bi_memsize-LOGBUFF_LEN-LOGBUFF_OVERHEAD;
	debug ("## Logbuffer at 0x%08lX ", kbd->bi_memsize-LOGBUFF_LEN);
#endif

	/*
	 * Booting a (Linux) kernel image
	 *
	 * Allocate space for command line and board info - the
	 * address should be as high as possible within the reach of
	 * the kernel (see CFG_BOOTMAPSZ settings), but in unused
	 * memory, which means far enough below the current stack
	 * pointer.
	 */

	asm( "mr %0,1": "=r"(sp) : );

	debug ("## Current stack ends at 0x%08lX ", sp);

	sp -= 2048;		/* just to be sure */
	if (sp > CFG_BOOTMAPSZ)
		sp = CFG_BOOTMAPSZ;
	sp &= ~0xF;

	debug ("=> set upper limit to 0x%08lX\n", sp);

	cmdline = (char *)((sp - CONFIG_CBSIZE) & ~0xF);
	kbd = (bd_t *)(((ulong)cmdline - sizeof(bd_t)) & ~0xF);

	printf("cmdline: %p kbd: %p\n", cmdline, kbd);

	if ((s = getenv("bootargs")) == NULL)
		s = "";

	strcpy (cmdline, s);

	cmd_start    = (ulong)cmdline;
	cmd_end      = cmd_start + strlen(cmdline);

	init_board_data(kbd);

#ifdef	DEBUG
	printf ("## cmdline at 0x%08lX ... 0x%08lX\n", cmd_start, cmd_end);

//	do_bdinfo (NULL, 0, 0, NULL);
#endif

	if ((s = getenv ("clocks_in_mhz")) != NULL) {
		/* convert all clock information to MHz */
		kbd->bi_intfreq /= 1000000L;
		kbd->bi_busfreq /= 1000000L;
#if defined(CONFIG_MPC8220)
		kbd->bi_inpfreq /= 1000000L;
		kbd->bi_pcifreq /= 1000000L;
		kbd->bi_pevfreq /= 1000000L;
		kbd->bi_flbfreq /= 1000000L;
		kbd->bi_vcofreq /= 1000000L;
#endif
#if defined(CONFIG_CPM2)
		kbd->bi_cpmfreq /= 1000000L;
		kbd->bi_brgfreq /= 1000000L;
		kbd->bi_sccfreq /= 1000000L;
		kbd->bi_vco     /= 1000000L;
#endif
#if defined(CONFIG_MPC5xxx)
		kbd->bi_ipbfreq /= 1000000L;
		kbd->bi_pcifreq /= 1000000L;
#endif /* CONFIG_MPC5xxx */
	}

	kernel = (void (*)(bd_t *, ulong, ulong, ulong, ulong))(32 * 1024 * 1024); /* FIXME */

	printf("uncompressing\n");
	if (relocate_image(os_header, (void *)(32 * 1024 * 1024)))
		return -1;
	printf("done\n");


#if defined(CFG_INIT_RAM_LOCK) && !defined(CONFIG_E500)
	unlock_ram_in_cache();
#endif

#ifdef CONFIG_OF_FLAT_TREE
	/* move of_flat_tree if needed */
	if (of_data) {
		ulong of_start, of_len;
		of_len = ((struct boot_param_header *)of_data)->totalsize;
		/* provide extra 8k pad */
		if (initrd_data)
			of_start = (unsigned long)initrd_data - of_len - 8192;
		else
			of_start  = (ulong)kbd - of_len - 8192;
		of_start &= ~(4096 - 1);	/* align on page */
		debug ("## device tree at 0x%08lX ... 0x%08lX (len=%ld=0x%lX)\n",
			of_data, of_data + of_len - 1, of_len, of_len);

		of_flat_tree = (char *)of_start;
		printf ("   Loading Device Tree to %08lx, end %08lx ... ",
			of_start, of_start + of_len - 1);
		memmove ((void *)of_start, (void *)of_data, of_len);
	}
#endif

	/*
	 * Linux Kernel Parameters (passing board info data):
	 *   r3: ptr to board info data
	 *   r4: initrd_start or 0 if no initrd
	 *   r5: initrd_end - unused if r4 is 0
	 *   r6: Start of command line string
	 *   r7: End   of command line string
	 */
#ifdef CONFIG_OF_FLAT_TREE
	if (!of_flat_tree)	/* no device tree; boot old style */
#endif
		(*kernel) (kbd, initrd_data, initrd_end, cmd_start, cmd_end);
		/* does not return */

#ifdef CONFIG_OF_FLAT_TREE
	/*
	 * Linux Kernel Parameters (passing device tree):
	 *   r3: ptr to OF flat tree, followed by the board info data
	 *   r4: physical pointer to the kernel itself
	 *   r5: NULL
	 *   r6: NULL
	 *   r7: NULL
	 */
	ft_setup(of_flat_tree, kbd, initrd_data, initrd_end);
	ft_dump_blob(of_flat_tree);

	(*kernel) ((bd_t *)of_flat_tree, (ulong)kernel, 0, 0, 0);
#endif
}

