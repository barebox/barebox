
/*
 * common.c - common wrapper functions between barebox and the host
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#define _GNU_SOURCE
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
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <sys/time.h>
/*
 * ...except the ones needed to connect with barebox
 */
#include <mach/linux.h>
#include <mach/hostfile.h>

#define DELETED_OFFSET (sizeof(" (deleted)") - 1)

void __sanitizer_set_death_callback(void (*callback)(void));

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

static char *stickypage_path;

static void prepare_exit(void)
{
	cookmode();
	if (stickypage_path)
		remove(stickypage_path);
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

int arch_ctrlc(void)
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

	now = ts.tv_sec * 1000ULL * 1000 * 1000 + ts.tv_nsec;

	return now;
}

void __attribute__((noreturn)) linux_exit(void)
{
	prepare_exit();
	exit(0);
}

static char **saved_argv;

static int selfpath(char *buf, size_t len)
{
	int ret;

	/* we must follow the symlink, so we can exec an updated executable */
	ret = readlink("/proc/self/exe", buf, len - 1);
	if (ret < 0)
		return ret;

	if (0 < ret && ret < len - 1)
		buf[ret] = '\0';

	return ret;
}

void linux_reexec(void)
{
	char buf[4097];
	ssize_t ret;

	cookmode();

	/* we must follow the symlink, so we can exec an updated executable */
	ret = selfpath(buf, sizeof(buf));
	if (ret > 0) {
		execv(buf, saved_argv);
		if (!strcmp(&buf[ret - DELETED_OFFSET], " (deleted)")) {
			printf("barebox image on disk changed. Loading new.\n");
			buf[ret - DELETED_OFFSET] = '\0';
			execv(buf, saved_argv);
		}
	}

	printf("exec(%s) failed: %d\n", buf, errno);
	/* falls through to generic hang() */
}

void linux_hang(void)
{
	prepare_exit();
	/* falls through to generic hang() */
}

int linux_open(const char *filename, int readwrite)
{
	return open(filename, (readwrite ? O_RDWR : O_RDONLY) | O_CLOEXEC);
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

loff_t linux_lseek(int fd, loff_t offset)
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

static void linux_watchdog(int signo)
{
	linux_reexec();
	_exit(0);
}

int linux_watchdog_set_timeout(unsigned int timeout)
{
	static int signal_handler_installed;

	if (!signal_handler_installed) {
		struct sigaction sact = {
			.sa_flags = SA_NODEFER, .sa_handler = linux_watchdog
		};

		sigemptyset(&sact.sa_mask);
		sigaction(SIGALRM, &sact, NULL);
		signal_handler_installed = 1;
	}

	return alarm(timeout);
}

extern void start_barebox(void);
extern void mem_malloc_init(void *start, void *end);

extern char * strsep_unescaped(char **s, const char *ct);

static int add_image(const char *_str, char *devname_template, int *devname_number)
{
	struct hf_info *hf = calloc(1, sizeof(struct hf_info));
	char *str, *filename, *devname;
	char tmp[16];
	char *opt;
	int ret;

	if (!hf)
		return -1;

	str = strdup(_str);

	filename = strsep_unescaped(&str, ",");
	while ((opt = strsep_unescaped(&str, ","))) {
		if (!strcmp(opt, "ro"))
			hf->is_readonly = 1;
		if (!strcmp(opt, "cdev"))
			hf->is_cdev = 1;
		if (!strcmp(opt, "blkdev"))
			hf->is_blockdev = 1;
	}

	/* parses: "devname=filename" */
	devname = strsep_unescaped(&filename, "=");
	filename = strsep_unescaped(&filename, "=");
	if (!filename) {
		filename = devname;
		snprintf(tmp, sizeof(tmp),
			 devname_template, (*devname_number)++);
		devname = tmp;
	}

	hf->filename = filename;
	hf->devname = strdup(devname);

	ret = barebox_register_filedev(hf);
	if (ret)
		free(hf);

	return ret;
}

extern uint8_t stickypage[4096];

char *linux_get_stickypage_path(void)
{
	size_t nwritten;
	ssize_t ret;
	int fd;

	ret = asprintf(&stickypage_path, "%s/barebox/stickypage.%lu",
		       getenv("XDG_RUNTIME_DIR") ?: "/run", (long)getpid());
	if (ret < 0)
		goto err_asprintf;

	ret = mkdir(dirname(stickypage_path), 0755);
	if (ret < 0 && errno != EEXIST) {
		perror("mkdir");
		goto err_creat;
	}

	stickypage_path[strlen(stickypage_path)] = '/';

	fd = open(stickypage_path, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0644);
	if (fd < 0) {
		if (errno == EEXIST)
			return stickypage_path;

		perror("open");
		goto err_creat;
	}

	for (nwritten = 0; nwritten < sizeof(stickypage); ) {
		ret = write(fd, &stickypage[nwritten], sizeof(stickypage) - nwritten);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			perror("write");
			goto err_write;
		}

		nwritten += ret;
	}

	close(fd);

	return stickypage_path;

err_write:
	close(fd);
err_creat:
	free(stickypage_path);
err_asprintf:
	stickypage_path = NULL;

	return NULL;
}

int linux_open_hostfile(struct hf_info *hf)
{
	char *buf = NULL;
	struct stat s;
	int fd = -1;

	printf("add %s %sbacked by file %s%s\n", hf->devname,
	       hf->filename ? "" : "initially un", hf->filename ?: "",
	       hf->is_readonly ? "(ro)" : "");

	if (!hf->filename)
		return -ENOENT;

	fd = hf->fd = open(hf->filename, (hf->is_readonly ? O_RDONLY : O_RDWR) | O_CLOEXEC);
	if (fd < 0) {
		perror("open");
		goto err_out;
	}

	if (fstat(fd, &s)) {
		perror("fstat");
		goto err_out;
	}

	hf->base = (unsigned long)MAP_FAILED;
	hf->size = s.st_size;

	if (S_ISBLK(s.st_mode)) {
		if (ioctl(fd, BLKGETSIZE64, &hf->size) == -1) {
			perror("ioctl");
			goto err_out;
		}
		if (!hf->is_cdev)
			hf->is_blockdev = 1;
	}
	if (hf->size <= SIZE_MAX) {
		hf->base = (unsigned long)mmap(NULL, hf->size,
				PROT_READ | (hf->is_readonly ? 0 : PROT_WRITE),
				MAP_SHARED, fd, 0);

		if (hf->base == (unsigned long)MAP_FAILED)
			printf("warning: mmapping %s failed: %s\n",
			       hf->filename, strerror(errno));
	} else {
		printf("warning: %s: contiguous map failed\n", hf->filename);
	}

	if (hf->is_blockdev && hf->size % 512 != 0) {
		printf("warning: registering %s as block device failed: invalid block size\n",
		       hf->filename);
		return -EINVAL;
	}

	return 0;

err_out:
	if (fd >= 0)
		close(fd);
	free(buf);
	return -1;
}

static int add_dtb(const char *file)
{
	struct stat s;
	void *dtb = NULL;
	int fd;

	fd = open(file, O_RDONLY | O_CLOEXEC);
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

static char *cmdline;

const char *barebox_cmdline_get(void)
{
	return cmdline;
}

static void print_usage(const char*);

static struct option long_options[] = {
	{"help",     0, 0, 'h'},
	{"malloc",   1, 0, 'm'},
	{"image",    1, 0, 'i'},
	{"command",  1, 0, 'c'},
	{"env",      1, 0, 'e'},
	{"dtb",      1, 0, 'd'},
	{"stdout",   1, 0, 'O'},
	{"stdin",    1, 0, 'I'},
	{"stdinout", 1, 0, 'B'},
	{"xres",     1, 0, 'x'},
	{"yres",     1, 0, 'y'},
	{0, 0, 0, 0},
};

static const char optstring[] = "hm:i:c:e:d:O:I:B:x:y:";

int main(int argc, char *argv[])
{
	void *ram;
	int opt, ret, fd, fd2;
	int malloc_size = CONFIG_MALLOC_SIZE;
	int fdno = 0, envno = 0, option_index = 0;
	char *new_cmdline;
	char *aux;

#ifdef CONFIG_ASAN
	__sanitizer_set_death_callback(prepare_exit);
#endif

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
		case 'c':
			if (asprintf(&new_cmdline, "%s%s\n", cmdline ?: "", optarg) < 0)
				exit(1);
			free(cmdline);
			cmdline = new_cmdline;
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

	saved_argv = argv;

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
			fd = open(optarg, O_WRONLY | O_CLOEXEC);
			if (fd < 0) {
				perror("open");
				exit(1);
			}

			barebox_register_console(-1, fd);
			break;
		case 'I':
			fd = open(optarg, O_RDWR | O_CLOEXEC);
			if (fd < 0) {
				perror("open");
				exit(1);
			}

			barebox_register_console(fd, -1);
			break;
		case 'B':
			aux = strchr(optarg, ',');
			if (!aux) {
				printf("-B, --stdinout requires two file paths given\n");
				exit(1);
			}

			/* open stdout file */
			fd = open(aux + 1, O_WRONLY | O_CLOEXEC);
			if (fd < 0) {
				perror("open stdout");
				exit(1);
			}

			/* open stdin file */
			aux = strndup(optarg, aux - optarg);
			fd2 = open(aux, O_RDWR | O_CLOEXEC);
			if (fd2 < 0) {
				perror("open stdin");
				exit(1);
			}
			free(aux);

			barebox_register_console(fd2, fd);
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
"  -c, --command=<cmd>  Run extra command after init scripts\n"
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
"  -B, --stdinout=<filein>,<fileout>\n"
"                       Register a bidirectional console capable of doing both\n"
"                       stdin and stdout. <filein> and <fileout> can be regular\n"
"                       files or FIFOs.\n"
"  -x, --xres=<res>     SDL width.\n"
"  -y, --yres=<res>     SDL height.\n",
	prgname
	);
}
