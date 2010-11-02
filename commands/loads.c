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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Serial up- and download support
 */
#include <common.h>
#include <command.h>
#include <s_record.h>
#include <net.h>
#include <exports.h>
#include <xyzModem.h>

DECLARE_GLOBAL_DATA_PTR;

static ulong load_serial (ulong offset);
static int read_record (char *buf, ulong len);
static int do_echo = 1;

# if (CONFIG_COMMANDS & CFG_CMD_SAVES)
static int save_serial (ulong offset, ulong size);
static int write_record (char *buf);
# endif /* CFG_CMD_SAVES */

int do_load_serial(struct command *cmdtp, int argc, char *argv[])
{
	ulong offset = 0;
	ulong addr;
	int i;
	char *env_echo;
	int rcode = 0;
#ifdef	CFG_LOADS_BAUD_CHANGE
	int load_baudrate, current_baudrate;

	load_baudrate = current_baudrate = gd->baudrate;
#endif

	if (((env_echo = getenv("loads_echo")) != NULL) && (*env_echo == '1')) {
		do_echo = 1;
	} else {
		do_echo = 0;
	}

#ifdef	CFG_LOADS_BAUD_CHANGE
	if (argc >= 2) {
		offset = simple_strtoul(argv[1], NULL, 16);
	}
	if (argc == 3) {
		load_baudrate = (int)simple_strtoul(argv[2], NULL, 10);

		/* default to current baudrate */
		if (load_baudrate == 0)
			load_baudrate = current_baudrate;
	}
	if (load_baudrate != current_baudrate) {
		printf ("## Switch baudrate to %d bps and press ENTER ...\n",
			load_baudrate);
		udelay(50000);
		gd->baudrate = load_baudrate;
		serial_setbrg ();
		udelay(50000);
		for (;;) {
			if (getc() == '\r')
				break;
		}
	}
#else	/* ! CFG_LOADS_BAUD_CHANGE */
	if (argc == 2) {
		offset = simple_strtoul(argv[1], NULL, 16);
	}
#endif	/* CFG_LOADS_BAUD_CHANGE */

	printf ("## Ready for S-Record download ...\n");

	addr = load_serial (offset);

	/*
	 * Gather any trailing characters (for instance, the ^D which
	 * is sent by 'cu' after sending a file), and give the
	 * box some time (100 * 1 ms)
	 */
	for (i=0; i<100; ++i) {
		if (tstc()) {
			(void) getc();
		}
		udelay(1000);
	}

	if (addr == ~0) {
		printf ("## S-Record download aborted\n");
		rcode = 1;
	} else {
		printf ("## Start Addr      = 0x%08lX\n", addr);
		load_addr = addr;
	}

#ifdef	CFG_LOADS_BAUD_CHANGE
	if (load_baudrate != current_baudrate) {
		printf ("## Switch baudrate to %d bps and press ESC ...\n",
			current_baudrate);
		udelay (50000);
		gd->baudrate = current_baudrate;
		serial_setbrg ();
		udelay (50000);
		for (;;) {
			if (getc() == 0x1B) /* ESC */
				break;
		}
	}
#endif
	return rcode;
}

static ulong
load_serial (ulong offset)
{
	char	record[SREC_MAXRECLEN + 1];	/* buffer for one S-Record	*/
	char	binbuf[SREC_MAXBINLEN];		/* buffer for binary data	*/
	int	binlen;				/* no. of data bytes in S-Rec.	*/
	int	type;				/* return code for record type	*/
	ulong	addr;				/* load address from S-Record	*/
	ulong	size;				/* number of bytes transferred	*/
	char	buf[32];
	ulong	store_addr;
	ulong	start_addr = ~0;
	ulong	end_addr   =  0;
	int	line_count =  0;

	while (read_record(record, SREC_MAXRECLEN + 1) >= 0) {
		type = srec_decode (record, &binlen, &addr, binbuf);

		if (type < 0) {
			return (~0);		/* Invalid S-Record		*/
		}

		switch (type) {
		case SREC_DATA2:
		case SREC_DATA3:
		case SREC_DATA4:
		    store_addr = addr + offset;
		    memcpy ((char *)(store_addr), binbuf, binlen);
		    if ((store_addr) < start_addr)
			start_addr = store_addr;
		    if ((store_addr + binlen - 1) > end_addr)
			end_addr = store_addr + binlen - 1;
		    break;
		case SREC_END2:
		case SREC_END3:
		case SREC_END4:
		    udelay (10000);
		    size = end_addr - start_addr + 1;
		    printf ("\n"
			    "## First Load Addr = 0x%08lX\n"
			    "## Last  Load Addr = 0x%08lX\n"
			    "## Total Size      = 0x%08lX = %ld Bytes\n",
			    start_addr, end_addr, size, size
		    );
		    sprintf(buf, "%lX", size);
		    setenv("filesize", buf);
		    return (addr);
		case SREC_START:
		    break;
		default:
		    break;
		}
		if (!do_echo) {	/* print a '.' every 100 lines */
			if ((++line_count % 100) == 0)
				putc ('.');
		}
	}

	return (~0);			/* Download aborted		*/
}

static int
read_record (char *buf, ulong len)
{
	char *p;
	char c;

	--len;	/* always leave room for terminating '\0' byte */

	for (p=buf; p < buf+len; ++p) {
		c = getc();		/* read character		*/
		if (do_echo)
			putc (c);	/* ... and echo it		*/

		switch (c) {
		case '\r':
		case '\n':
			*p = '\0';
			return (p - buf);
		case '\0':
		case 0x03:			/* ^C - Control C		*/
			return (-1);
		default:
			*p = c;
		}

	    /* Check for the console hangup (if any different from serial) */
	    if (gd->jt[XF_getc] != getc) {
		if (ctrlc()) {
		    return (-1);
		}
	    }
	}

	/* line too long - truncate */
	*p = '\0';
	return (p - buf);
}

#if (CONFIG_COMMANDS & CFG_CMD_SAVES)

int do_save_serial(struct command *cmdtp, int flag, int argc, char *argv[])
{
	ulong offset = 0;
	ulong size   = 0;
#ifdef	CFG_LOADS_BAUD_CHANGE
	int save_baudrate, current_baudrate;

	save_baudrate = current_baudrate = gd->baudrate;
#endif

	if (argc >= 2) {
		offset = simple_strtoul(argv[1], NULL, 16);
	}
#ifdef	CFG_LOADS_BAUD_CHANGE
	if (argc >= 3) {
		size = simple_strtoul(argv[2], NULL, 16);
	}
	if (argc == 4) {
		save_baudrate = (int)simple_strtoul(argv[3], NULL, 10);

		/* default to current baudrate */
		if (save_baudrate == 0)
			save_baudrate = current_baudrate;
	}
	if (save_baudrate != current_baudrate) {
		printf ("## Switch baudrate to %d bps and press ENTER ...\n",
			save_baudrate);
		udelay(50000);
		gd->baudrate = save_baudrate;
		serial_setbrg ();
		udelay(50000);
		for (;;) {
			if (getc() == '\r')
				break;
		}
	}
#else	/* ! CFG_LOADS_BAUD_CHANGE */
	if (argc == 3) {
		size = simple_strtoul(argv[2], NULL, 16);
	}
#endif	/* CFG_LOADS_BAUD_CHANGE */

	printf ("## Ready for S-Record upload, press ENTER to proceed ...\n");
	for (;;) {
		if (getc() == '\r')
			break;
	}
	if(save_serial (offset, size)) {
		printf ("## S-Record upload aborted\n");
	} else {
		printf ("## S-Record upload complete\n");
	}
#ifdef	CFG_LOADS_BAUD_CHANGE
	if (save_baudrate != current_baudrate) {
		printf ("## Switch baudrate to %d bps and press ESC ...\n",
			(int)current_baudrate);
		udelay (50000);
		gd->baudrate = current_baudrate;
		serial_setbrg ();
		udelay (50000);
		for (;;) {
			if (getc() == 0x1B) /* ESC */
				break;
		}
	}
#endif
	return 0;
}

#define SREC3_START				"S0030000FC\n"
#define SREC3_FORMAT			"S3%02X%08lX%s%02X\n"
#define SREC3_END				"S70500000000FA\n"
#define SREC_BYTES_PER_RECORD	16

static int save_serial (ulong address, ulong count)
{
	int i, c, reclen, checksum, length;
	char *hex = "0123456789ABCDEF";
	char	record[2*SREC_BYTES_PER_RECORD+16];	/* buffer for one S-Record	*/
	char	data[2*SREC_BYTES_PER_RECORD+1];	/* buffer for hex data	*/

	reclen = 0;
	checksum  = 0;

	if(write_record(SREC3_START))			/* write the header */
		return (-1);
	do {
		if(count) {						/* collect hex data in the buffer  */
			c = *(volatile uchar*)(address + reclen);	/* get one byte    */
			checksum += c;							/* accumulate checksum */
			data[2*reclen]   = hex[(c>>4)&0x0f];
			data[2*reclen+1] = hex[c & 0x0f];
			data[2*reclen+2] = '\0';
			++reclen;
			--count;
		}
		if(reclen == SREC_BYTES_PER_RECORD || count == 0) {
			/* enough data collected for one record: dump it */
			if(reclen) {	/* build & write a data record: */
				/* address + data + checksum */
				length = 4 + reclen + 1;

				/* accumulate length bytes into checksum */
				for(i = 0; i < 2; i++)
					checksum += (length >> (8*i)) & 0xff;

				/* accumulate address bytes into checksum: */
				for(i = 0; i < 4; i++)
					checksum += (address >> (8*i)) & 0xff;

				/* make proper checksum byte: */
				checksum = ~checksum & 0xff;

				/* output one record: */
				sprintf(record, SREC3_FORMAT, length, address, data, checksum);
				if(write_record(record))
					return (-1);
			}
			address  += reclen;  /* increment address */
			checksum  = 0;
			reclen    = 0;
		}
	}
	while(count);
	if(write_record(SREC3_END))	/* write the final record */
		return (-1);
	return(0);
}

static int
write_record (char *buf)
{
	char c;

	while((c = *buf++))
		putc(c);

	/* Check for the console hangup (if any different from serial) */

	if (ctrlc()) {
	    return (-1);
	}
	return (0);
}
# endif /* CFG_CMD_SAVES */

#ifdef	CFG_LOADS_BAUD_CHANGE
BAREBOX_CMD(
	loads, 3, 0,	do_load_serial,
	"loads   - load S-Record file over serial line\n",
	"[ off ] [ baud ]\n"
	"    - load S-Record file over serial line"
	" with offset 'off' and baudrate 'baud'\n"
);

#else	/* ! CFG_LOADS_BAUD_CHANGE */
BAREBOX_CMD(
	loads, 2, 0,	do_load_serial,
	"loads   - load S-Record file over serial line\n",
	"[ off ]\n"
	"    - load S-Record file over serial line with offset 'off'\n"
);
#endif	/* CFG_LOADS_BAUD_CHANGE */

/*
 * SAVES always requires LOADS support, but not vice versa
 */


#if (CONFIG_COMMANDS & CFG_CMD_SAVES)
#ifdef	CFG_LOADS_BAUD_CHANGE
BAREBOX_CMD(
	saves, 4, 0,	do_save_serial,
	"saves   - save S-Record file over serial line\n",
	"[ off ] [size] [ baud ]\n"
	"    - save S-Record file over serial line"
	" with offset 'off', size 'size' and baudrate 'baud'\n"
);
#else	/* ! CFG_LOADS_BAUD_CHANGE */
BAREBOX_CMD(
	saves, 3, 0,	do_save_serial,
	"saves   - save S-Record file over serial line\n",
	"[ off ] [size]\n"
	"    - save S-Record file over serial line with offset 'off' and size 'size'\n"
);
#endif	/* CFG_LOADS_BAUD_CHANGE */
#endif	/* CFG_CMD_SAVES */

