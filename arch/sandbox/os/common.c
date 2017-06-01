
/*
 * common.c - common wrapper functions between barebox and the host
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * These are host includes. Never include any barebox header
 * files here...
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
/*
 * ...except the ones needed to connect with barebox
 */
#include <mach/linux.h>
#include <mach/hostfile.h>

int sdl_xres;
int sdl_yres;

static struct termios term_orig, term_vi;
static char erase_char;	/* the users erase character */

static void rawmode(void)
{
	tcgetattr(0, &term_orig);
	term_vi = term_orig;
	term_vi.c_lflag &= (~ICANON & ~ECHO & ~ISIG);
	term_vi.c_iflag &= (~IXON & ~ICRNL);
	term_vi.c_oflag |= (ONLCR);
	term_vi.c_cc[VMIN] = 1;
	term_vi.c_cc[VTIME] = 0;
	erase_char = term_vi.c_cc[VERASE];
	tcsetattr(0, TCSANOW, &term_vi);
}

static void cookmode(void)
{
	fflush(stdout);
	tcsetattr(0, TCSANOW, &term_orig);
}

int linux_tstc(int fd)
{
	struct timeval tv = {
		.tv_usec = 100,
	};
	fd_set rfds;
	int ret;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	/*
	 * We set the timeout here to 100us, because otherwise
	 * barebox would eat all cpu resources while waiting
	 * for input.
	 */
	ret = select(fd + 1, &rfds, NULL, NULL, &tv);

	if (ret)
		return 1;

	return 0;
}

int ctrlc(void)
{
	char chr;

	if (linux_read_nonblock(0, &chr, 1) == 1 && chr == 3)
		return 1;
	return 0;
}

uint64_t linux_get_time(void)
{
	struct timespec ts;
	uint64_t now;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	now = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;

	return now;
}

void __attribute__((noreturn)) linux_exit(void)
{
	cookmode();
	exit(0);
}

int linux_read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	if (count == 0)
		return 0;

	do {
		ret = read(fd, buf, count);

		if (ret == 0) {
			printf("read on fd %d returned 0, device gone? - exiting\n", fd);
			linux_exit();
		} else if (ret == -1) {
			if (errno == EAGAIN)
				return -errno;
			else if (errno == EINTR)
				continue;
			else {
				printf("read on fd %d returned -1, errno %d - exiting\n", fd, errno);
				linux_exit();
			}
		}
	} while (ret <= 0);

	return (int)ret;
}

int linux_read_nonblock(int fd, void *buf, size_t count)
{
	int oldflags, ret;

	oldflags = fcntl(fd, F_GETFL);
	if (oldflags == -1)
		goto err_out;

	if (fcntl(fd, F_SETFL, oldflags | O_NONBLOCK) == -1)
		goto err_out;

	ret = linux_read(fd, buf, count);

	if (fcntl(fd, F_SETFL, oldflags) == -1)
		goto err_out;

	return ret;

err_out:
	perror("fcntl");
	return -1;
}

ssize_t linux_write(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}

off_t linux_lseek(int fd, off_t offset)
{
	return lseek(fd, offset, SEEK_SET);
}

int linux_execve(const char * filename, char *const argv[], char *const envp[])
{
	pid_t pid, tpid;
	int execve_status;

	pid = fork();

	if (pid == -1) {
		perror("linux_execve");
		return pid;
	} else if (pid == 0) {
		exit(execve(filename, argv, envp));
	} else {
		do {
			tpid = wait(&execve_status);
		} while(tpid != pid);

		return execve_status;
	}
}

extern void start_barebox(void);
extern void mem_malloc_init(void *start, void *end);

static int add_image(char *str, char *devname_template, int *devname_number)
{
	struct hf_info *hf = malloc(sizeof(struct hf_info));
	char *filename, *devname;
	char tmp[16];
	int readonly = 0;
	struct stat s;
	char *opt;
	int fd, ret;

	if (!hf)
		return -1;

	filename = strtok(str, ",");
	while ((opt = strtok(NULL, ","))) {
		if (!strcmp(opt, "ro"))
			readonly = 1;
	}

	/* parses: "devname=filename" */
	devname = strtok(filename, "=");
	filename = strtok(NULL, "=");
	if (!filename) {
		filename = devname;
		snprintf(tmp, sizeof(tmp),
			 devname_template, (*devname_number)++);
		devname = strdup(tmp);
	}

	printf("add %s backed by file %s%s\n", devname,
	       filename, readonly ? "(ro)" : "");

	fd = open(filename, readonly ? O_RDONLY : O_RDWR);
	hf->fd = fd;
	hf->filename = filename;

	if (fd < 0) {
		perror("open");
		goto err_out;
	}

	if (fstat(fd, &s)) {
		perror("fstat");
		goto err_out;
	}

	hf->size = s.st_size;
	hf->devname = strdup(devname);

	hf->base = (unsigned long)mmap(NULL, hf->size,
			PROT_READ | (readonly ? 0 : PROT_WRITE),
			MAP_SHARED, fd, 0);
	if ((void *)hf->base == MAP_FAILED)
		printf("warning: mmapping %s failed\n", filename);

	ret = barebox_register_filedev(hf);
	if (ret)
		goto err_out;
	return 0;

err_out:
	if (fd > 0)
		close(fd);
	free(hf);
	return -1;
}

static int add_dtb(const char *file)
{
	struct stat s;
	void *dtb = NULL;
	int fd;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		goto err_out;
	}

	if (fstat(fd, &s)) {
		perror("fstat");
		goto err_out;
	}

	dtb = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (dtb == MAP_FAILED) {
		perror("mmap");
		goto err_out;
	}

	if (barebox_register_dtb(dtb))
		goto err_out;

	return 0;

 err_out:
	if (dtb)
		munmap(dtb, s.st_size);
	if (fd > 0)
		close(fd);
	return -1;
}

static void print_usage(const char*);

static struct option long_options[] = {
	{"help",   0, 0, 'h'},
	{"malloc", 1, 0, 'm'},
	{"image",  1, 0, 'i'},
	{"env",    1, 0, 'e'},
	{"dtb",    1, 0, 'd'},
	{"stdout", 1, 0, 'O'},
	{"stdin",  1, 0, 'I'},
	{"xres",  1, 0, 'x'},
	{"yres",  1, 0, 'y'},
	{0, 0, 0, 0},
};

static const char optstring[] = "hm:i:e:d:O:I:x:y:";

int main(int argc, char *argv[])
{
	void *ram;
	int opt, ret, fd;
	int malloc_size = CONFIG_MALLOC_SIZE;
	int fdno = 0, envno = 0, option_index = 0;

	while (1) {
		option_index = 0;
		opt = getopt_long(argc, argv, optstring,
			long_options, &option_index);

		if (opt == -1)
			break;

		switch (opt) {
		case 'h':
			print_usage(basename(argv[0]));
			exit(0);
		case 'm':
			malloc_size = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			break;
		case 'e':
			break;
		case 'd':
			ret = add_dtb(optarg);
			if (ret) {
				printf("Failed to load dtb: '%s'\n", optarg);
				exit(1);
			}
			break;
		case 'x':
			sdl_xres = strtoul(optarg, NULL, 0);
			break;
		case 'y':
			sdl_yres = strtoul(optarg, NULL, 0);
			break;
		default:
			break;
		}
	}

	ram = malloc(malloc_size);
	if (!ram) {
		printf("unable to get malloc space\n");
		exit(1);
	}
	mem_malloc_init(ram, ram + malloc_size - 1);

	/*
	 * Reset getopt.
	 * We need to run a second getopt to count -i parameters.
	 * This is for /dev/fd# devices.
	 */
	optind = 1;

	while (1) {
		option_index = 0;
		opt = getopt_long(argc, argv, optstring,
			long_options, &option_index);

		if (opt == -1)
			break;

		switch (opt) {
		case 'i':
			ret = add_image(optarg, "fd%d", &fdno);
			if (ret)
				exit(1);
			break;
		case 'e':
			ret = add_image(optarg, "env%d", &envno);
			if (ret)
				exit(1);
			break;
		case 'O':
			fd = open(optarg, O_WRONLY);
			if (fd < 0) {
				perror("open");
				exit(1);
			}

			barebox_register_console(-1, fd);
			break;
		case 'I':
			fd = open(optarg, O_RDWR);
			if (fd < 0) {
				perror("open");
				exit(1);
			}

			barebox_register_console(fd, -1);
			break;
		default:
			break;
		}
	}

	barebox_register_console(fileno(stdin), fileno(stdout));

	rawmode();
	start_barebox();

	/* never reached */
	return 0;
}

#ifdef __PPC__
/* HACK: we need this symbol on PPC, better ask a PPC export about this :) */
char _SDA_BASE_[4096];
#endif

static void print_usage(const char *prgname)
{
	printf(
"Usage: %s [OPTIONS]\n"
"Start barebox.\n\n"
"Options:\n\n"
"  -m, --malloc=<size>  Start sandbox with a specified malloc-space size in bytes.\n"
"  -i, --image=<file>   Map an image file to barebox. This option can be given\n"
"                       multiple times. The files will show up as\n"
"                       /dev/fd0 ... /dev/fdx under barebox.\n"
"  -i, --image=<dev>=<file>\n"
"                       Same as above, the files will show up as\n"
"                       /dev/<dev>\n"
"  -e, --env=<file>     Map a file with an environment to barebox. With this \n"
"                       option, files are mapped as /dev/env0 ... /dev/envx\n"
"                       and thus are used as the default environment.\n"
"                       An empty file generated with dd will do to get started\n"
"                       with an empty environment.\n"
"  -d, --dtb=<file>     Map a device tree binary blob (dtb) into barebox.\n"
"  -O, --stdout=<file>  Register a file as a console capable of doing stdout.\n"
"                       <file> can be a regular file or a FIFO.\n"
"  -I, --stdin=<file>   Register a file as a console capable of doing stdin.\n"
"                       <file> can be a regular file or a FIFO.\n"
"  -x, --xres=<res>     SDL width.\n"
"  -y, --yres=<res>     SDL height.\n",
	prgname
	);
}
