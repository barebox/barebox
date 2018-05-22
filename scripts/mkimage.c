/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "compiler.h"

#include "../include/image.h"
#include "../common/image.c"

char *cmdname;

#include "../crypto/crc32.c"

//extern unsigned long crc32 (unsigned long crc, const char *buf, unsigned int len);

static	void	copy_file (int, const char *, int);
static	void	usage	(void);
static	int	get_table_entry (table_entry_t *, char *, char *);
static	int	get_arch(char *);
static	int	get_comp(char *);
static	int	get_os  (char *);
static	int	get_type(char *);


char	*datafile;
char	*imagefile;

int dflag    = 0;
int eflag    = 0;
int lflag    = 0;
int vflag    = 0;
int xflag    = 0;
int opt_os   = IH_OS_LINUX;
int opt_arch = IH_ARCH_PPC;
int opt_type = IH_TYPE_KERNEL;
int opt_comp = IH_COMP_GZIP;

image_header_t header;
image_header_t *hdr = &header;

static inline uint32_t image_get_header_size(void)
{
	return sizeof(image_header_t);
}

#define image_get_hdr_u32(x)					\
static inline uint32_t image_get_##x(const image_header_t *hdr)	\
{								\
	return uimage_to_cpu(hdr->ih_##x);			\
}

image_get_hdr_u32(magic);	/* image_get_magic */
image_get_hdr_u32(hcrc);	/* image_get_hcrc */
image_get_hdr_u32(time);	/* image_get_time */
image_get_hdr_u32(size);	/* image_get_size */
image_get_hdr_u32(load);	/* image_get_load */
image_get_hdr_u32(ep);		/* image_get_ep */
image_get_hdr_u32(dcrc);	/* image_get_dcrc */

#define image_get_hdr_u8(x)					\
static inline uint8_t image_get_##x(const image_header_t *hdr)	\
{								\
	return hdr->ih_##x;					\
}
image_get_hdr_u8(os);		/* image_get_os */
image_get_hdr_u8(arch);		/* image_get_arch */
image_get_hdr_u8(type);		/* image_get_type */
image_get_hdr_u8(comp);		/* image_get_comp */

static inline char *image_get_name(const image_header_t *hdr)
{
	return (char*)hdr->ih_name;
}

static inline uint32_t image_get_data_size(const image_header_t *hdr)
{
	return image_get_size(hdr);
}

/**
 * image_get_data - get image payload start address
 * @hdr: image header
 *
 * image_get_data() returns address of the image payload. For single
 * component images it is image data start. For multi component
 * images it points to the null terminated table of sub-images sizes.
 *
 * returns:
 *     image payload data start address
 */
static inline ulong image_get_data(const image_header_t *hdr)
{
	return ((ulong)hdr + image_get_header_size());
}

static inline uint32_t image_get_image_size(const image_header_t *hdr)
{
	return (image_get_size(hdr) + image_get_header_size());
}

static inline ulong image_get_image_end(const image_header_t *hdr)
{
	return ((ulong)hdr + image_get_image_size(hdr));
}

#define image_set_hdr_u32(x)						\
static inline void image_set_##x(image_header_t *hdr, uint32_t val)	\
{									\
	hdr->ih_##x = cpu_to_uimage(val);				\
}

image_set_hdr_u32(magic);	/* image_set_magic */
image_set_hdr_u32(hcrc);	/* image_set_hcrc */
image_set_hdr_u32(time);	/* image_set_time */
image_set_hdr_u32(size);	/* image_set_size */
image_set_hdr_u32(load);	/* image_set_load */
image_set_hdr_u32(ep);		/* image_set_ep */
image_set_hdr_u32(dcrc);	/* image_set_dcrc */

#define image_set_hdr_u8(x)						\
static inline void image_set_##x(image_header_t *hdr, uint8_t val)	\
{									\
	hdr->ih_##x = val;						\
}

image_set_hdr_u8(os);		/* image_set_os */
image_set_hdr_u8(arch);		/* image_set_arch */
image_set_hdr_u8(type);		/* image_set_type */
image_set_hdr_u8(comp);		/* image_set_comp */

static inline void image_set_name(image_header_t *hdr, const char *name)
{
	strncpy(image_get_name(hdr), name, IH_NMLEN - 1);
}

/**
 * image_multi_count - get component (sub-image) count
 * @hdr: pointer to the header of the multi component image
 *
 * image_multi_count() returns number of components in a multi
 * component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     number of components
 */
static ulong image_multi_count(void *data)
{
	ulong i, count = 0;
	uint32_t *size;

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)data;

	/* count non empty slots */
	for (i = 0; size[i]; ++i)
		count++;

	return count;
}

/**
 * image_multi_getimg - get component data address and size
 * @hdr: pointer to the header of the multi component image
 * @idx: index of the requested component
 * @data: pointer to a ulong variable, will hold component data address
 * @len: pointer to a ulong variable, will hold component size
 *
 * image_multi_getimg() returns size and data address for the requested
 * component in a multi component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     data address and size of the component, if idx is valid
 *     0 in data and len, if idx is out of range
 */
static void image_multi_getimg(void *data, ulong idx,
			ulong *img_data, ulong *len)
{
	int i;
	uint32_t *size;
	ulong offset, count, tmp_img_data;

	/* get number of component */
	count = image_multi_count(data);

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)data;

	/* get address of the proper component data start, which means
	 * skipping sizes table (add 1 for last, null entry) */
	tmp_img_data = (ulong)data + (count + 1) * sizeof (uint32_t);

	if (idx < count) {
		*len = uimage_to_cpu(size[idx]);
		offset = 0;

		/* go over all indices preceding requested component idx */
		for (i = 0; i < idx; i++) {
			/* add up i-th component size, rounding up to 4 bytes */
			offset += (uimage_to_cpu(size[i]) + 3) & ~3 ;
		}

		/* calculate idx-th component data address */
		*img_data = tmp_img_data + offset;
	} else {
		*len = 0;
		*img_data = 0;
	}
}

static void image_print_type(const image_header_t *hdr)
{
	const char *os, *arch, *type, *comp;

	os = image_get_os_name(image_get_os(hdr));
	arch = image_get_arch_name(image_get_arch(hdr));
	type = image_get_type_name(image_get_type(hdr));
	comp = image_get_comp_name(image_get_comp(hdr));

	printf ("%s %s %s (%s)\n", arch, os, type, comp);
}

static void image_print_time(time_t timestamp)
{
	printf("%s", ctime(&timestamp));
}

static void image_print_size(uint32_t size)
{
	printf("%d Bytes = %.2f kB = %.2f MB\n",
			size, (double)size / 1.024e3,
			(double)size / 1.048576e6);
}

static void image_print_contents(const image_header_t *hdr, void *data)
{
	int type;

	printf("Image Name:   %.*s\n", IH_NMLEN, image_get_name(hdr));
	printf("Created:      ");
	image_print_time((time_t)image_get_time(hdr));
	printf ("Image Type:   ");
	image_print_type(hdr);
	printf ("Data Size:    ");
	image_print_size(image_get_data_size(hdr));
	printf ("Load Address: %08x\n", image_get_load(hdr));
	printf ("Entry Point:  %08x\n", image_get_ep(hdr));

	type = image_get_type(hdr);
	if (data && (type == IH_TYPE_MULTI || type == IH_TYPE_SCRIPT)) {
		int i;
		ulong img_data, len;
		ulong count = image_multi_count(data);

		printf ("Contents:\n");
		for (i = 0; i < count; i++) {
			image_multi_getimg(data, i, &img_data, &len);

			printf("   Image %d: ", i);
			image_print_size(len);

			if (image_get_type(hdr) != IH_TYPE_SCRIPT && i > 0) {
				/*
				 * the user may need to know offsets
				 * if planning to do something with
				 * multiple files
				 */
				printf("    Offset = 0x%08lx\n", img_data);
			}
		}
	}
}

int
main (int argc, char **argv)
{
	int ifd;
	uint32_t checksum;
	uint32_t addr;
	uint32_t ep;
	struct stat sbuf;
	char *ptr;
	char *name = "";

	cmdname = *argv;

	addr = ep = 0;

	while (--argc > 0 && **++argv == '-') {
		while (*++*argv) {
			switch (**argv) {
			case 'l':
				lflag = 1;
				break;
			case 'A':
				if ((--argc <= 0) ||
				    (opt_arch = get_arch(*++argv)) < 0)
					usage ();
				goto NXTARG;
			case 'C':
				if ((--argc <= 0) ||
				    (opt_comp = get_comp(*++argv)) < 0)
					usage ();
				goto NXTARG;
			case 'O':
				if ((--argc <= 0) ||
				    (opt_os = get_os(*++argv)) < 0)
					usage ();
				goto NXTARG;
			case 'T':
				if ((--argc <= 0) ||
				    (opt_type = get_type(*++argv)) < 0)
					usage ();
				goto NXTARG;

			case 'a':
				if (--argc <= 0)
					usage ();
				addr = strtoul (*++argv, (char **)&ptr, 16);
				if (*ptr) {
					fprintf (stderr,
						"%s: invalid load address %s\n",
						cmdname, *argv);
					exit (EXIT_FAILURE);
				}
				goto NXTARG;
			case 'd':
				if (--argc <= 0)
					usage ();
				datafile = *++argv;
				dflag = 1;
				goto NXTARG;
			case 'e':
				if (--argc <= 0)
					usage ();
				ep = strtoul (*++argv, (char **)&ptr, 16);
				if (*ptr) {
					fprintf (stderr,
						"%s: invalid entry point %s\n",
						cmdname, *argv);
					exit (EXIT_FAILURE);
				}
				eflag = 1;
				goto NXTARG;
			case 'n':
				if (--argc <= 0)
					usage ();
				name = *++argv;
				goto NXTARG;
			case 'v':
				vflag++;
				break;
			case 'x':
				xflag++;
				break;
			default:
				usage ();
			}
		}
NXTARG:		;
	}

	if ((argc != 1) || ((lflag ^ dflag) == 0))
		usage();

	if (!eflag) {
		ep = addr;
		/* If XIP, entry point must be after the barebox header */
		if (xflag)
			ep += sizeof(image_header_t);
	}

	/*
	 * If XIP, ensure the entry point is equal to the load address plus
	 * the size of the barebox header.
	 */
	if (xflag) {
		if (ep != addr + sizeof(image_header_t)) {
			fprintf (stderr,
				"%s: For XIP, the entry point must be the load addr + %lu\n",
				cmdname,
				(unsigned long)sizeof(image_header_t));
			exit (EXIT_FAILURE);
		}
	}

	imagefile = *argv;

	if (lflag) {
		ifd = open(imagefile, O_RDONLY|O_BINARY);
	} else {
		ifd = open(imagefile, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666);
	}

	if (ifd < 0) {
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (lflag) {
		int len;
		char *data;
		/*
		 * list header information of existing image
		 */
		if (fstat(ifd, &sbuf) < 0) {
			fprintf (stderr, "%s: Can't stat %s: %s\n",
				cmdname, imagefile, strerror(errno));
			exit (EXIT_FAILURE);
		}

		if ((unsigned)sbuf.st_size < sizeof(image_header_t)) {
			fprintf (stderr,
				"%s: Bad size: \"%s\" is no valid image\n",
				cmdname, imagefile);
			exit (EXIT_FAILURE);
		}

		ptr = mmap(0, sbuf.st_size,
					    PROT_READ, MAP_SHARED, ifd, 0);
		if ((caddr_t)ptr == (caddr_t)-1) {
			fprintf (stderr, "%s: Can't read %s: %s\n",
				cmdname, imagefile, strerror(errno));
			exit (EXIT_FAILURE);
		}

		/*
		 * create copy of header so that we can blank out the
		 * checksum field for checking - this can't be done
		 * on the PROT_READ mapped data.
		 */
		memcpy (hdr, ptr, sizeof(image_header_t));

		if (image_get_magic(hdr) != IH_MAGIC) {
			fprintf (stderr,
				"%s: Bad Magic Number: \"%s\" is no valid image\n",
				cmdname, imagefile);
			exit (EXIT_FAILURE);
		}

		data = (char *)hdr;
		len  = image_get_header_size();

		checksum = image_get_hcrc(hdr);
		image_set_hcrc(hdr, 0);	/* clear for re-calculation */

		if (crc32 (0, (unsigned char *)data, len) != checksum) {
			fprintf (stderr,
				"%s: ERROR: \"%s\" has bad header checksum!\n",
				cmdname, imagefile);
			exit (EXIT_FAILURE);
		}

		data = (char *)(ptr + image_get_header_size());
		len  = sbuf.st_size - image_get_header_size() ;

		if (crc32 (0, (unsigned char *)data, len) != image_get_dcrc(hdr)) {
			fprintf (stderr,
				"%s: ERROR: \"%s\" has corrupted data!\n",
				cmdname, imagefile);
			exit (EXIT_FAILURE);
		}

		/* for multi-file images we need the data part, too */
		image_print_contents((image_header_t *)ptr,
				     (void*)image_get_data((image_header_t *)ptr));

		(void) munmap((void *)ptr, sbuf.st_size);
		(void) close (ifd);

		exit (EXIT_SUCCESS);
	}

	/*
	 * Must be -w then:
	 *
	 * write dummy header, to be fixed later
	 */
	memset (hdr, 0, image_get_header_size());

	if (write(ifd, hdr, image_get_header_size()) != image_get_header_size()) {
		fprintf (stderr, "%s: Write error on %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if ((opt_type == IH_TYPE_MULTI) ||
	    (opt_type == IH_TYPE_SCRIPT)) {
		char *file = datafile;
		uint32_t size;

		for (;;) {
			char *sep = NULL;

			if (file) {
				if ((sep = strchr(file, ':')) != NULL) {
					*sep = '\0';
				}

				if (stat (file, &sbuf) < 0) {
					fprintf (stderr, "%s: Can't stat %s: %s\n",
						cmdname, file, strerror(errno));
					exit (EXIT_FAILURE);
				}
				size = htonl(sbuf.st_size);
			} else {
				size = 0;
			}

			if (write(ifd, (char *)&size, sizeof(size)) != sizeof(size)) {
				fprintf (stderr, "%s: Write error on %s: %s\n",
					cmdname, imagefile, strerror(errno));
				exit (EXIT_FAILURE);
			}

			if (!file) {
				break;
			}

			if (sep) {
				*sep = ':';
				file = sep + 1;
			} else {
				file = NULL;
			}
		}

		file = datafile;

		for (;;) {
			char *sep = strchr(file, ':');
			if (sep) {
				*sep = '\0';
				copy_file (ifd, file, 1);
				*sep++ = ':';
				file = sep;
			} else {
				copy_file (ifd, file, 0);
				break;
			}
		}
	} else {
		copy_file (ifd, datafile, 0);
	}

	/* We're a bit of paranoid */
#if defined(_POSIX_SYNCHRONIZED_IO) && !defined(__sun__) && !defined(__FreeBSD__)
	(void) fdatasync (ifd);
#else
	(void) fsync (ifd);
#endif

	if (fstat(ifd, &sbuf) < 0) {
		fprintf (stderr, "%s: Can't stat %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	ptr = mmap(0, sbuf.st_size,
				    PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
	if (ptr == MAP_FAILED) {
		fprintf (stderr, "%s: Can't map %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	hdr = (image_header_t *)ptr;

	checksum = crc32 (0,
			  (unsigned char *)(ptr + image_get_header_size()),
			  sbuf.st_size - image_get_header_size()
			 );

	/* Build new header */
	image_set_magic(hdr, IH_MAGIC);
	image_set_time(hdr, sbuf.st_mtime);
	image_set_size(hdr, sbuf.st_size - image_get_header_size());
	image_set_load(hdr, addr);
	image_set_ep(hdr, ep);
	image_set_dcrc(hdr, checksum);
	image_set_os(hdr, opt_os);
	image_set_arch(hdr, opt_arch);
	image_set_type(hdr, opt_type);
	image_set_comp(hdr, opt_comp);

	image_set_name(hdr, name);

	checksum = crc32(0,(unsigned char *)hdr, image_get_header_size());

	image_set_hcrc(hdr, checksum);

	image_print_contents(hdr, (void*)image_get_data(hdr));

	(void) munmap((void *)ptr, sbuf.st_size);

	/* We're a bit of paranoid */
#if defined(_POSIX_SYNCHRONIZED_IO) && !defined(__sun__) && !defined(__FreeBSD__)
	(void) fdatasync (ifd);
#else
	(void) fsync (ifd);
#endif

	if (close(ifd)) {
		fprintf (stderr, "%s: Write error on %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	exit (EXIT_SUCCESS);
}

static void
copy_file (int ifd, const char *datafile, int pad)
{
	int dfd;
	struct stat sbuf;
	unsigned char *ptr;
	int tail;
	int zero = 0;
	int offset = 0;
	int size;

	if (vflag) {
		fprintf (stderr, "Adding Image %s\n", datafile);
	}

	if ((dfd = open(datafile, O_RDONLY|O_BINARY)) < 0) {
		fprintf (stderr, "%s: Can't open %s: %s\n",
			cmdname, datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf (stderr, "%s: Can't stat %s: %s\n",
			cmdname, datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	ptr = (unsigned char *)mmap(0, sbuf.st_size,
				    PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == (unsigned char *)MAP_FAILED) {
		fprintf (stderr, "%s: Can't read %s: %s\n",
			cmdname, datafile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (xflag) {
		unsigned char *p = NULL;
		/*
		 * XIP: do not append the image_header_t at the
		 * beginning of the file, but consume the space
		 * reserved for it.
		 */

		if ((unsigned)sbuf.st_size < image_get_header_size()) {
			fprintf (stderr,
				"%s: Bad size: \"%s\" is too small for XIP\n",
				cmdname, datafile);
			exit (EXIT_FAILURE);
		}

		for (p = ptr; p < ptr + image_get_header_size(); p++) {
			if ( *p != 0xff ) {
				fprintf (stderr,
					"%s: Bad file: \"%s\" has invalid buffer for XIP\n",
					cmdname, datafile);
				exit (EXIT_FAILURE);
			}
		}

		offset = image_get_header_size();
	}

	size = sbuf.st_size - offset;
	if (write(ifd, ptr + offset, size) != size) {
		fprintf (stderr, "%s: Write error on %s: %s\n",
			cmdname, imagefile, strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (pad && ((tail = size % 4) != 0)) {

		if (write(ifd, (char *)&zero, 4-tail) != 4-tail) {
			fprintf (stderr, "%s: Write error on %s: %s\n",
				cmdname, imagefile, strerror(errno));
			exit (EXIT_FAILURE);
		}
	}

	(void) munmap((void *)ptr, sbuf.st_size);
	(void) close (dfd);
}

void
usage ()
{
	fprintf (stderr, "Usage: %s -l image\n"
			 "          -l ==> list image header information\n"
			 "       %s [-x] -A arch -O os -T type -C comp "
			 "-a addr -e ep -n name -d data_file[:data_file...] image\n",
		cmdname, cmdname);
	fprintf (stderr, "          -A ==> set architecture to 'arch'\n"
			 "          -O ==> set operating system to 'os'\n"
			 "          -T ==> set image type to 'type'\n"
			 "          -C ==> set compression type 'comp'\n"
			 "          -a ==> set load address to 'addr' (hex)\n"
			 "          -e ==> set entry point to 'ep' (hex)\n"
			 "          -n ==> set image name to 'name'\n"
			 "          -d ==> use image data from 'datafile'\n"
			 "          -x ==> set XIP (execute in place)\n"
		);
	exit (EXIT_FAILURE);
}

static int get_arch(char *name)
{
	return (get_table_entry(arch_name, "CPU", name));
}


static int get_comp(char *name)
{
	return (get_table_entry(comp_name, "Compression", name));
}


static int get_os (char *name)
{
	return (get_table_entry(os_name, "OS", name));
}


static int get_type(char *name)
{
	return (get_table_entry(type_name, "Image", name));
}

static int get_table_entry (table_entry_t *table, char *msg, char *name)
{
	table_entry_t *t;
	int first = 1;

	for (t=table; t->id>=0; ++t) {
		if (t->sname && strcasecmp(t->sname, name)==0)
			return (t->id);
	}
	fprintf (stderr, "\nInvalid %s Type - valid names are", msg);
	for (t=table; t->id>=0; ++t) {
		if (t->sname == NULL)
			continue;
		fprintf (stderr, "%c %s", (first) ? ':' : ',', t->sname);
		first = 0;
	}
	fprintf (stderr, "\n");
	return (-1);
}
