
#include <common.h>
#include <watchdog.h>

#ifdef CONFIG_OF_FLAT_TREE
#include <ft_build.h>
#endif

static void  __attribute__((noinline))
do_bootm_linux (cmd_tbl_t *cmdtp, int flag,
		int	argc, char *argv[],
		ulong	addr,
		ulong	*len_ptr,
		int	verify)
{
	ulong	sp;
	ulong	len, checksum;
	ulong	initrd_start, initrd_end;
	ulong	cmd_start, cmd_end;
	ulong	initrd_high;
	ulong	data;
	int	initrd_copy_to_ram = 1;
	char    *cmdline;
	char	*s;
	bd_t	*kbd;
	void	(*kernel)(bd_t *, ulong, ulong, ulong, ulong);
	image_header_t *hdr = &header;
#ifdef CONFIG_OF_FLAT_TREE
	char	*of_flat_tree = NULL;
	ulong	of_data = 0;
#endif

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

	cmdline = (char *)((sp - CFG_BARGSIZE) & ~0xF);
	kbd = (bd_t *)(((ulong)cmdline - sizeof(bd_t)) & ~0xF);

	if ((s = getenv("bootargs")) == NULL)
		s = "";

	strcpy (cmdline, s);

	cmd_start    = (ulong)&cmdline[0];
	cmd_end      = cmd_start + strlen(cmdline);

	*kbd = *(gd->bd);

#ifdef	DEBUG
	printf ("## cmdline at 0x%08lX ... 0x%08lX\n", cmd_start, cmd_end);

	do_bdinfo (NULL, 0, 0, NULL);
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

	kernel = (void (*)(bd_t *, ulong, ulong, ulong, ulong)) ntohl(hdr->ih_ep);

	/*
	 * Check if there is an initrd image
	 */

#ifdef CONFIG_OF_FLAT_TREE
	/* Look for a '-' which indicates to ignore the ramdisk argument */
	if (argc >= 3 && strcmp(argv[2], "-") ==  0) {
			debug ("Skipping initrd\n");
			len = data = 0;
		}
	else
#endif
	if (argc >= 3) {
		debug ("Not skipping initrd\n");
		SHOW_BOOT_PROGRESS (9);

		addr = simple_strtoul(argv[2], NULL, 16);

		printf ("## Loading RAMDisk Image at %08lx ...\n", addr);

		/* Copy header so we can blank CRC field for re-calculation */
		memmove (&header, (char *)addr, sizeof(image_header_t));

		if (ntohl(hdr->ih_magic)  != IH_MAGIC) {
			puts ("Bad Magic Number\n");
			SHOW_BOOT_PROGRESS (-10);
			do_reset (cmdtp, flag, argc, argv);
		}

		data = (ulong)&header;
		len  = sizeof(image_header_t);

		checksum = ntohl(hdr->ih_hcrc);
		hdr->ih_hcrc = 0;

		if (crc32 (0, (uchar *)data, len) != checksum) {
			puts ("Bad Header Checksum\n");
			SHOW_BOOT_PROGRESS (-11);
			do_reset (cmdtp, flag, argc, argv);
		}

		SHOW_BOOT_PROGRESS (10);

		print_image_hdr (hdr);

		data = addr + sizeof(image_header_t);
		len  = ntohl(hdr->ih_size);

		if (verify) {
			ulong csum = 0;
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
			ulong cdata = data, edata = cdata + len;
#endif	/* CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG */

			puts ("   Verifying Checksum ... ");

#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)

			while (cdata < edata) {
				ulong chunk = edata - cdata;

				if (chunk > CHUNKSZ)
					chunk = CHUNKSZ;
				csum = crc32 (csum, (uchar *)cdata, chunk);
				cdata += chunk;

				WATCHDOG_RESET();
			}
#else	/* !(CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG) */
			csum = crc32 (0, (uchar *)data, len);
#endif	/* CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG */

			if (csum != ntohl(hdr->ih_dcrc)) {
				puts ("Bad Data CRC\n");
				SHOW_BOOT_PROGRESS (-12);
				do_reset (cmdtp, flag, argc, argv);
			}
			puts ("OK\n");
		}

		SHOW_BOOT_PROGRESS (11);

		if ((hdr->ih_os   != IH_OS_LINUX)	||
		    (hdr->ih_arch != IH_CPU_PPC)	||
		    (hdr->ih_type != IH_TYPE_RAMDISK)	) {
			puts ("No Linux PPC Ramdisk Image\n");
			SHOW_BOOT_PROGRESS (-13);
			do_reset (cmdtp, flag, argc, argv);
		}

		/*
		 * Now check if we have a multifile image
		 */
	} else if ((hdr->ih_type==IH_TYPE_MULTI) && (len_ptr[1])) {
		u_long tail    = ntohl(len_ptr[0]) % 4;
		int i;

		SHOW_BOOT_PROGRESS (13);

		/* skip kernel length and terminator */
		data = (ulong)(&len_ptr[2]);
		/* skip any additional image length fields */
		for (i=1; len_ptr[i]; ++i)
			data += 4;
		/* add kernel length, and align */
		data += ntohl(len_ptr[0]);
		if (tail) {
			data += 4 - tail;
		}

		len   = ntohl(len_ptr[1]);

	} else {
		/*
		 * no initrd image
		 */
		SHOW_BOOT_PROGRESS (14);

		len = data = 0;
	}

#ifdef CONFIG_OF_FLAT_TREE
	if(argc > 3) {
		of_flat_tree = (char *) simple_strtoul(argv[3], NULL, 16);
		hdr = (image_header_t *)of_flat_tree;

		if  (*(ulong *)of_flat_tree == OF_DT_HEADER) {
#ifndef CFG_NO_FLASH
			if (addr2info((ulong)of_flat_tree) != NULL)
				of_data = (ulong)of_flat_tree;
#endif
		} else if (ntohl(hdr->ih_magic) == IH_MAGIC) {
			printf("## Flat Device Tree Image at %08lX\n", hdr);
			print_image_hdr(hdr);

			if ((ntohl(hdr->ih_load) <  ((unsigned long)hdr + ntohl(hdr->ih_size) + sizeof(hdr))) &&
			   ((ntohl(hdr->ih_load) + ntohl(hdr->ih_size)) > (unsigned long)hdr)) {
				printf ("ERROR: Load address overwrites Flat Device Tree uImage\n");
				return;
			}

			printf("   Verifying Checksum ... ");
			memmove (&header, (char *)hdr, sizeof(image_header_t));
			checksum = ntohl(header.ih_hcrc);
			header.ih_hcrc = 0;

			if(checksum != crc32(0, (uchar *)&header, sizeof(image_header_t))) {
				printf("ERROR: Flat Device Tree header checksum is invalid\n");
				return;
			}

			checksum = ntohl(hdr->ih_dcrc);
			addr = (ulong)((uchar *)(hdr) + sizeof(image_header_t));
			len = ntohl(hdr->ih_size);

			if(checksum != crc32(0, (uchar *)addr, len)) {
				printf("ERROR: Flat Device Tree checksum is invalid\n");
				return;
			}
			printf("OK\n");

			if (ntohl(hdr->ih_type) != IH_TYPE_FLATDT) {
				printf ("ERROR: uImage not Flat Device Tree type\n");
				return;
			}
			if (ntohl(hdr->ih_comp) != IH_COMP_NONE) {
				printf("ERROR: uImage is not uncompressed\n");
				return;
			}
			if (*((ulong *)(of_flat_tree + sizeof(image_header_t))) != OF_DT_HEADER) {
				printf ("ERROR: uImage data is not a flat device tree\n");
				return;
			}

			memmove((void *)ntohl(hdr->ih_load),
		       		(void *)(of_flat_tree + sizeof(image_header_t)),
				ntohl(hdr->ih_size));
			of_flat_tree = (char *)ntohl(hdr->ih_load);
		} else {
			printf ("Did not find a flat flat device tree at address %08lX\n", of_flat_tree);
			return;
		}
		printf ("   Booting using flat device tree at 0x%x\n",
				of_flat_tree);
	} else if ((hdr->ih_type==IH_TYPE_MULTI) && (len_ptr[1]) && (len_ptr[2])) {
		u_long tail    = ntohl(len_ptr[0]) % 4;
		int i;

		/* skip kernel length, initrd length, and terminator */
		of_data = (ulong)(&len_ptr[3]);
		/* skip any additional image length fields */
		for (i=2; len_ptr[i]; ++i)
			of_data += 4;
		/* add kernel length, and align */
		of_data += ntohl(len_ptr[0]);
		if (tail) {
			of_data += 4 - tail;
		}

		/* add initrd length, and align */
		tail = ntohl(len_ptr[1]) % 4;
		of_data += ntohl(len_ptr[1]);
		if (tail) {
			of_data += 4 - tail;
		}

		if (((struct boot_param_header *)of_data)->magic != OF_DT_HEADER) {
			printf ("ERROR: image is not a flat device tree\n");
			return;
		}

		if (((struct boot_param_header *)of_data)->totalsize != ntohl(len_ptr[2])) {
			printf ("ERROR: flat device tree size does not agree with image\n");
			return;
		}
	}
#endif
	if (!data) {
		debug ("No initrd\n");
	}

	if (data) {
	    if (!initrd_copy_to_ram) {	/* zero-copy ramdisk support */
		initrd_start = data;
		initrd_end = initrd_start + len;
	    } else {
		initrd_start  = (ulong)kbd - len;
		initrd_start &= ~(4096 - 1);	/* align on page */

		if (initrd_high) {
			ulong nsp;

			/*
			 * the inital ramdisk does not need to be within
			 * CFG_BOOTMAPSZ as it is not accessed until after
			 * the mm system is initialised.
			 *
			 * do the stack bottom calculation again and see if
			 * the initrd will fit just below the monitor stack
			 * bottom without overwriting the area allocated
			 * above for command line args and board info.
			 */
			asm( "mr %0,1": "=r"(nsp) : );
			nsp -= 2048;		/* just to be sure */
			nsp &= ~0xF;
			if (nsp > initrd_high)	/* limit as specified */
				nsp = initrd_high;
			nsp -= len;
			nsp &= ~(4096 - 1);	/* align on page */
			if (nsp >= sp)
				initrd_start = nsp;
		}

		SHOW_BOOT_PROGRESS (12);

		debug ("## initrd at 0x%08lX ... 0x%08lX (len=%ld=0x%lX)\n",
			data, data + len - 1, len, len);

		initrd_end    = initrd_start + len;
		printf ("   Loading Ramdisk to %08lx, end %08lx ... ",
			initrd_start, initrd_end);
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
		{
			size_t l = len;
			void *to = (void *)initrd_start;
			void *from = (void *)data;

			while (l > 0) {
				size_t tail = (l > CHUNKSZ) ? CHUNKSZ : l;
				WATCHDOG_RESET();
				memmove (to, from, tail);
				to += tail;
				from += tail;
				l -= tail;
			}
		}
#else	/* !(CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG) */
		memmove ((void *)initrd_start, (void *)data, len);
#endif	/* CONFIG_HW_WATCHDOG || CONFIG_WATCHDOG */
		puts ("OK\n");
	    }
	} else {
		initrd_start = 0;
		initrd_end = 0;
	}

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
		(ulong)kernel);

	SHOW_BOOT_PROGRESS (15);

#if defined(CFG_INIT_RAM_LOCK) && !defined(CONFIG_E500)
	unlock_ram_in_cache();
#endif

#ifdef CONFIG_OF_FLAT_TREE
	/* move of_flat_tree if needed */
	if (of_data) {
		ulong of_start, of_len;
		of_len = ((struct boot_param_header *)of_data)->totalsize;
		/* provide extra 8k pad */
		if (initrd_start)
			of_start = initrd_start - of_len - 8192;
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
		(*kernel) (kbd, initrd_start, initrd_end, cmd_start, cmd_end);
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
	ft_setup(of_flat_tree, kbd, initrd_start, initrd_end);
	/* ft_dump_blob(of_flat_tree); */

	(*kernel) ((bd_t *)of_flat_tree, (ulong)kernel, 0, 0, 0);
#endif
}

