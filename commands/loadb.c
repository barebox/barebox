/**
 * @file
 * @brief LoadB and LoadY support.
 *
 * Provides loadb (over Kermit) and LoadY(over Y modem) support to download
 * images.
 *
 * FileName: commands/loadb.c
 */
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
 */

/*
 * Serial up- and download support
 */
#include <common.h>
#include <command.h>
#include <console.h>
#include <errno.h>
#include <environment.h>
#include <cache.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <malloc.h>

#define XON_CHAR	17
#define XOFF_CHAR	19
#define START_CHAR	0x01
#define ETX_CHAR	0x03
#define END_CHAR	0x0D
#define SPACE		0x20
#define K_ESCAPE	0x23
#define SEND_TYPE	'S'
#define DATA_TYPE	'D'
#define ACK_TYPE	'Y'
#define NACK_TYPE	'N'
#define BREAK_TYPE	'B'
#define tochar(x)	((char) (((x) + SPACE) & 0xff))
#define untochar(x)	((int) (((x) - SPACE) & 0xff))
#define DEF_FILE	"image.bin"

static int ofd;			/* output file descriptor */

/* Size of my buffer to write to o/p file */
#define MAX_WRITE_BUFFER 4096	/* Write size to o/p file */
static char *write_buffer;	/* buffer for finalized data to write */
static int write_idx;		/* index to the current location in buffer */

static char his_eol;		/* character he needs at end of packet */
static int his_pad_count;	/* number of pad chars he needs */
static char his_pad_char;	/* pad chars he needs */
static char his_quote;		/* quote chars he'll use */
static struct console_device *cdev; /* The console device we are using */

static void sendchar(char c)
{
	cdev->putc(cdev, c);
}

static int receivechar(void)
{
	return cdev->getc(cdev);
}

static void send_pad(void)
{
	int count = his_pad_count;

	while (count-- > 0)
		sendchar(his_pad_char);
}

/* converts escaped kermit char to binary char */
static char ktrans(char in)
{
	if ((in & 0x60) == 0x40) {
		return (char)(in & ~0x40);
	} else if ((in & 0x7f) == 0x3f) {
		return (char)(in | 0x40);
	} else
		return in;
}

static int chk1(char *buffer)
{
	int total = 0;

	while (*buffer) {
		total += *buffer++;
	}
	return (int)((total + ((total >> 6) & 0x03)) & 0x3f);
}

static void s1_sendpacket(char *packet)
{
	send_pad();
	while (*packet)
		sendchar(*packet++);
}

static char a_b[24];
static void send_ack(int n)
{
	a_b[0] = START_CHAR;
	a_b[1] = tochar(3);
	a_b[2] = tochar(n);
	a_b[3] = ACK_TYPE;
	a_b[4] = '\0';
	a_b[4] = tochar(chk1(&a_b[1]));
	a_b[5] = his_eol;
	a_b[6] = '\0';
	s1_sendpacket(a_b);
}

static void send_nack(int n)
{
	a_b[0] = START_CHAR;
	a_b[1] = tochar(3);
	a_b[2] = tochar(n);
	a_b[3] = NACK_TYPE;
	a_b[4] = '\0';
	a_b[4] = tochar(chk1(&a_b[1]));
	a_b[5] = his_eol;
	a_b[6] = '\0';
	s1_sendpacket(a_b);
}

/* os_data_* data buffer handling before flushing the data to the o/p device
 * we will only dump valid packets to the device.
 */
static int os_data_count, os_old_count, os_data_total;
static char *os_data_addr, *os_old_buffer;

static void os_data_init(void)
{
	os_data_count = 0;
	os_old_count = 0;
	os_data_addr = NULL;
	os_old_buffer = NULL;
}

static int os_data_alloc(int length)
{
	/* If already allocated, free the memory
	 */
	if (os_data_addr != NULL) {
		os_data_addr -= os_data_count;
		free(os_data_addr);
	}
	os_data_count = 0;
	os_data_addr = (char *)malloc(length);
	if (os_data_addr == NULL)
		return -1;
	return 0;
}

static int os_data_save(void)
{
	int ret = 0;
	int copy_size;
	/* if there was an old data, flush it */
	if (os_old_buffer) {
		char *ori_buf = os_old_buffer;
		do {
			if (write_idx == MAX_WRITE_BUFFER) {
				ret = write(ofd, write_buffer,
						MAX_WRITE_BUFFER);
				if (ret < 0) {
					eprintf("write to device failed\n");
					return ret;
				}
				write_idx = 0;
			}
			copy_size = min(os_old_count, (MAX_WRITE_BUFFER -
						write_idx));
			memcpy(write_buffer + write_idx, os_old_buffer,
					copy_size);
			write_idx += copy_size;
			os_data_total += copy_size;
			os_old_count -= copy_size;
			os_old_buffer += copy_size;
		} while (os_old_count);	/* if remaining bytes */
		free(ori_buf);
	}
	os_old_count = os_data_count;
	os_old_buffer = os_data_addr - os_data_count;
	os_data_count = 0;
	os_data_addr = NULL;
	return 0;
}

static void os_data_dump(void)
{
	if (os_old_buffer)
		free(os_old_buffer);
	if (os_data_addr) {
		os_data_addr -= os_data_count;
		free(os_data_addr);
	}
	os_data_init();
}
static void os_data_char(char new_char)
{
	*os_data_addr++ = new_char;
	os_data_count++;
}

/* k_data_* simply handles the kermit escape translations */
static int k_data_escape, k_data_escape_saved;
static void k_data_init(void)
{
	k_data_escape = 0;
}
static void k_data_save(void)
{
	k_data_escape_saved = k_data_escape;
}
static void k_data_restore(void)
{
	k_data_escape = k_data_escape_saved;
}
static void k_data_char(char new_char)
{
	if (k_data_escape) {
		/* last char was escape - translate this character */
		os_data_char(ktrans(new_char));
		k_data_escape = 0;
	} else {
		if (new_char == his_quote) {
			/* this char is escape - remember */
			k_data_escape = 1;
		} else {
			/* otherwise send this char as-is */
			os_data_char(new_char);
		}
	}
}

#define SEND_DATA_SIZE  20
static char send_parms[SEND_DATA_SIZE];
static char *send_ptr;

/**
 * @brief interprets the protocol info and builds and
 * sends an appropriate ack for what we can do
 *
 * @param n sequence
 *
 * @return void
 */
static void handle_send_packet(int n)
{
	int length = 3;
	int bytes;

	/* initialize some protocol parameters */
	his_eol = END_CHAR;	/* default end of line character */
	his_pad_count = 0;
	his_pad_char = '\0';
	his_quote = K_ESCAPE;

	/* ignore last character if it filled the buffer */
	if (send_ptr == &send_parms[SEND_DATA_SIZE - 1])
		--send_ptr;
	bytes = send_ptr - send_parms;	/* how many bytes we'll process */
	do {
		if (bytes-- <= 0)
			break;
		/* handle MAXL - max length */
		/* ignore what he says - most I'll take (here) is 94 */
		a_b[++length] = tochar(94);
		if (bytes-- <= 0)
			break;
		/* handle TIME - time you should wait for my packets */
		/* ignore what he says - don't wait for my ack longer than 1 second */
		a_b[++length] = tochar(1);
		if (bytes-- <= 0)
			break;
		/* handle NPAD - number of pad chars I need */
		/* remember what he says - I need none */
		his_pad_count = untochar(send_parms[2]);
		a_b[++length] = tochar(0);
		if (bytes-- <= 0)
			break;
		/* handle PADC - pad chars I need */
		/* remember what he says - I need none */
		his_pad_char = ktrans(send_parms[3]);
		a_b[++length] = 0x40;	/* He should ignore this */
		if (bytes-- <= 0)
			break;
		/* handle EOL - end of line he needs */
		/* remember what he says - I need CR */
		his_eol = untochar(send_parms[4]);
		a_b[++length] = tochar(END_CHAR);
		if (bytes-- <= 0)
			break;
		/* handle QCTL - quote control char he'll use */
		/* remember what he says - I'll use '#' */
		his_quote = send_parms[5];
		a_b[++length] = '#';
		if (bytes-- <= 0)
			break;
		/* handle QBIN - 8-th bit prefixing */
		/* ignore what he says - I refuse */
		a_b[++length] = 'N';
		if (bytes-- <= 0)
			break;
		/* handle CHKT - the clock check type */
		/* ignore what he says - I do type 1 (for now) */
		a_b[++length] = '1';
		if (bytes-- <= 0)
			break;
		/* handle REPT - the repeat prefix */
		/* ignore what he says - I refuse (for now) */
		a_b[++length] = 'N';
		if (bytes-- <= 0)
			break;
		/* handle CAPAS - the capabilities mask */
		/* ignore what he says - I only do long packets - I don't do windows */
		a_b[++length] = tochar(2);	/* only long packets */
		a_b[++length] = tochar(0);	/* no windows */
		a_b[++length] = tochar(94);	/* large packet msb */
		a_b[++length] = tochar(94);	/* large packet lsb */
	} while (0);

	a_b[0] = START_CHAR;
	a_b[1] = tochar(length);
	a_b[2] = tochar(n);
	a_b[3] = ACK_TYPE;
	a_b[++length] = '\0';
	a_b[length] = tochar(chk1(&a_b[1]));
	a_b[++length] = his_eol;
	a_b[++length] = '\0';
	s1_sendpacket(a_b);
}

/**
 * @brief receives a OS Open image file over kermit line
 *
 * @return bytes read
 */
static int k_recv(void)
{
	char new_char;
	char k_state, k_state_saved;
	int sum;
	int done;
	int length;
	int n, last_n;
	int z = 0;
	int len_lo, len_hi;

	/* initialize some protocol parameters */
	his_eol = END_CHAR;	/* default end of line character */
	his_pad_count = 0;
	his_pad_char = '\0';
	his_quote = K_ESCAPE;

	/* initialize the k_recv and k_data state machine */
	done = 0;
	k_state = 0;
	k_data_init();
	k_state_saved = k_state;
	k_data_save();
	os_data_init();
	os_data_total = 0;
	n = 0;			/* just to get rid of a warning */
	last_n = -1;

	/* expect this "type" sequence (but don't check):
	   S: send initiate
	   F: file header
	   D: data (multiple)
	   Z: end of file
	   B: break transmission
	 */

	/* enter main loop */
	while (!done) {
		/* set the send packet pointer to begining of send packet parms */
		send_ptr = send_parms;

		/* With each packet, start summing the bytes starting with the length.
		   Save the current sequence number.
		   Note the type of the packet.
		   If a character less than SPACE (0x20) is received - error.
		 */

		/* get a packet */
		/* wait for the starting character or ^C */
		for (;;) {
			switch (receivechar()) {
			case START_CHAR:	/* start packet */
				goto START;
			case ETX_CHAR:	/* ^C waiting for packet */
				/* Save last success buffer and quit */
				(void)os_data_save();
				return (0);
			default:
				;
			}
		}
START:
		/* get length of packet */
		sum = 0;
		new_char = receivechar();
		if ((new_char & 0xE0) == 0)
			goto packet_error;
		sum += new_char & 0xff;
		length = untochar(new_char);
		/* get sequence number */
		new_char = receivechar();
		if ((new_char & 0xE0) == 0)
			goto packet_error;
		sum += new_char & 0xff;
		n = untochar(new_char);
		--length;

		/* NEW CODE - check sequence numbers for retried packets */
		/* Note - this new code assumes that the sequence number is correctly
		 * received.  Handling an invalid sequence number adds another layer
		 * of complexity that may not be needed - yet!  At this time, I'm hoping
		 * that I don't need to buffer the incoming data packets and can write
		 * the data into memory in real time.
		 */
		if (n == last_n) {
			/* same sequence number, restore the previous state */
			k_state = k_state_saved;
			k_data_restore();
			/* Dump any previously allocated data including previous
			 *  buffer host wishes to retry.. */
			os_data_dump();
		} else {
			/* new sequence number, checkpoint the download */
			last_n = n;
			k_state_saved = k_state;
			k_data_save();
		}
		/* END NEW CODE */

		/* get packet type */
		new_char = receivechar();
		if ((new_char & 0xE0) == 0)
			goto packet_error;
		sum += new_char & 0xff;
		k_state = new_char;
		--length;
		/* check for extended length */
		if (length == -2) {
			/* (length byte was 0, decremented twice) */
			/* get the two length bytes */
			new_char = receivechar();
			if ((new_char & 0xE0) == 0)
				goto packet_error;
			sum += new_char & 0xff;
			len_hi = untochar(new_char);
			new_char = receivechar();
			if ((new_char & 0xE0) == 0)
				goto packet_error;
			sum += new_char & 0xff;
			len_lo = untochar(new_char);
			length = len_hi * 95 + len_lo;
			/* check header checksum */
			new_char = receivechar();
			if ((new_char & 0xE0) == 0)
				goto packet_error;
			if (new_char !=
			    tochar((sum + ((sum >> 6) & 0x03)) & 0x3f))
				goto packet_error;
			sum += new_char & 0xff;
			/* --length;
			 * new length includes only data and block check to come
			 */
		}
		/* Try to allocate data chunk */
		if (length > 1) {
			if (os_data_alloc(length) < 0) {
				/* too large.. NACK the data back */
				goto packet_error;
			}
		}
		while (length > 1) {
			new_char = receivechar();
			if ((new_char & 0xE0) == 0)
				goto packet_error;
			sum += new_char & 0xff;
			--length;
			if (k_state == DATA_TYPE) {
				/* pass on the data if this is a data packet */
				k_data_char(new_char);
			} else if (k_state == SEND_TYPE) {
				/* save send pack in buffer as is */
				*send_ptr++ = new_char;
				/* if too much data, back off the pointer */
				if (send_ptr >= &send_parms[SEND_DATA_SIZE])
					--send_ptr;
			}
		}
		/* get and validate checksum character */
		new_char = receivechar();
		if ((new_char & 0xE0) == 0)
			goto packet_error;
		if (new_char != tochar((sum + ((sum >> 6) & 0x03)) & 0x3f))
			goto packet_error;
		/* get END_CHAR */
		new_char = receivechar();
		if (new_char != END_CHAR) {
packet_error:
			/* restore state machines */
			k_state = k_state_saved;
			k_data_restore();
			/* send a negative acknowledge packet in */
			send_nack(n);
		} else if (k_state == SEND_TYPE) {
			/* crack the protocol parms, build an appropriate ack packet */
			handle_send_packet(n);
		} else {
			/* send simple acknowledge packet in */
			send_ack(n);
			/* Flush old buffer,
			 * Store current buffer as old buffer */
			os_data_save();
			/* quit if end of transmission */
			if (k_state == BREAK_TYPE) {
				/* Flush remaining buffer */
				os_data_save();
				done = 1;
			}
		}
		++z;
	}
	return ((ulong) os_data_total);
}

/**
 * @brief loadb Support over kermit protocol
 *
 * @return download size
 */
static ulong load_serial_bin(void)
{
	int size, i;
	char buf[32];

	/* Try to allocate the buffer we shall write to files */
	write_buffer = malloc(MAX_WRITE_BUFFER);
	if (write_buffer == NULL) {
		eprintf("could not allocate file i/o buffer\n");
		return -ENOMEM;
	}

	/* Lets get the image from host */
	size = k_recv();

	/*
	 * Gather any trailing characters (for instance, the ^D which
	 * is sent by 'cu' after sending a file), and give the
	 * box some time (100 * 1 ms)
	 */
	for (i = 0; i < 100; ++i) {
		if (cdev->tstc(cdev))
			(void)receivechar();;
		udelay(1000);
	}

	/* Flush out the remaining data if any */
	if (write_idx > 0) {
		i = write(ofd, write_buffer, write_idx);
		if (i < 0) {
			eprintf("write to device failed\n");
			size = i;
			goto err_quit;
		}
		write_idx = 0;
	}
	printf("## Total Size      = 0x%08x = %d Bytes\n", size, size);
	sprintf(buf, "%X", size);
	setenv("filesize", buf);

err_quit:
	free(write_buffer);
	return size;
}

/**
 * @brief provide the loadb(Kermit) or loadY mode support
 *
 * @param cmdtp
 * @param argc
 * @param argv
 *
 * @return success or failure
 */
static int do_load_serial_bin(int argc, char *argv[])
{
	ulong offset = 0;
	ulong addr;
	int load_baudrate = 0, current_baudrate;
	int rcode = 0, ret;
	int opt;
	char *output_file = NULL;
	int current_active;
	char *console_dev_name = NULL;

	while ((opt = getopt(argc, argv, "f:b:o:c:")) > 0) {
		switch (opt) {
		case 'f':
			output_file = optarg;
			break;
		case 'b':
			load_baudrate = (int)simple_strtoul(optarg, NULL, 10);
			break;
		case 'o':
			offset = (int)simple_strtoul(optarg, NULL, 10);
			break;
		case 'c':
			console_dev_name = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (console_dev_name) {
		cdev = console_get_by_name(console_dev_name);
		if (!cdev) {
			printf("Console %s not found\n", console_dev_name);
			return -ENODEV;
		}
	} else {
		cdev = console_get_first_active();
		if (!cdev) {
			printf("No console device with STDIN and STDOUT\n");
			return -ENODEV;
		}
	}
	current_baudrate = console_get_baudrate(cdev);
	current_active = console_get_active(cdev);

	/* Load Defaults */
	if (load_baudrate == 0)
		load_baudrate = current_baudrate;
	if (NULL == output_file)
		output_file = DEF_FILE;

	/* File should exist */
	ofd = open(output_file, O_WRONLY | O_CREAT);
	if (ofd < 0) {
		perror(argv[0]);
		return 3;
	}
	/* Seek to the right offset */
	if (offset) {
		int seek = lseek(ofd, offset, SEEK_SET);
		if (seek != offset) {
			close(ofd);
			ofd = 0;
			perror(argv[0]);
			return 4;
		}
	}

	printf("## Ready for binary (kermit) download "
	       "to 0x%08lX offset on %s device at %d bps...\n", offset,
	       output_file, load_baudrate);

	console_set_active(cdev, 0);
	ret = console_set_baudrate(cdev, load_baudrate);
	if (ret)
		return ret;

	addr = load_serial_bin();
	if (addr == 0) {
		printf("## Binary (kermit) download aborted\n");
		rcode = 1;
	}

	ret = console_set_baudrate(cdev, current_baudrate);
	if (ret)
		return ret;

	console_set_active(cdev, current_active);

	close(ofd);
	ofd = 0;
	return rcode;
}

BAREBOX_CMD_HELP_START(loadb)
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f FILE", "download to FILE (default image.bin)")
BAREBOX_CMD_HELP_OPT("-o OFFS", "destination file OFFSet (default 0)")
BAREBOX_CMD_HELP_OPT("-b BAUD", "baudrate for download (default: console baudrate)")
BAREBOX_CMD_HELP_OPT("-c CONSOLE", "Specify console (default: first active console")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(loadb)
	.cmd = do_load_serial_bin,
	BAREBOX_CMD_DESC("load binary file over serial line (Kermit)")
	BAREBOX_CMD_OPTS(" FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_loadb_help)
BAREBOX_CMD_END
