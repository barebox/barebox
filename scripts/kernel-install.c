/*
 * kernel-install - install a kernel according to the bootloader spec:
 * http://www.freedesktop.org/wiki/Specifications/BootLoaderSpec/
 *
 * Copyright (C) 2013 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 *
 * This tool is useful for installing kernels in a bootloader spec
 * conformant way. It can be used to install kernels for the currently
 * running system, but also to install kernels for another system which
 * is available as a removable media such as an SD card.
 *
 * Some examples:
 *
 * kernel-install --add --kernel-version=3.11 --kernel=/somewhere/zImage \
 *		--title "Linux-3.11"
 *
 * This is the simplest example. It assumes we want to install a kernel for the
 * currently running system. Usually the kernel should get some commandline
 * options which can be passed using the -o option. Devicetree and initrd can be
 * specified with --devicetree=<file> or --initrd=<file>.
 *
 * For preparing boot media from another host (or the same host, but another
 * rootfs) things get slightly more complicated. Apart from the image files
 * kernel-install generally needs a machine-id (which is, in native mode, read
 * from /etc/machine-id) and access to /boot of newly installed entry.
 * /boot can be specified in different ways:
 *
 * --boot=/boot             - specify the path where /boot is mounted
 * --boot=/dev/sdd1         - specify the partition which contains /boot.
 *                            It is mounted using pmount or mount
 * --device=/dev/sdd        - If this option is given kernel-install tries
 *                            to find /boot on this device using the mechanisms
 *                            described in the bootloader spec.
 *
 * machine-id can be specified with:
 * --machine-id=<machine-id>    - explicitly specify a machine-id
 * --root=/root or
 * --root=/dev/sdd2             - specify where the root of the installed system
 *                                can be found. The machine id is then taken
 *                                from /etc/machine-id from this filesystem/path
 *
 * Optionally kernel-install can automatically generate a root=PARTUUID= kernel
 * parameter for the kernel to find its root filesystem. This is done with the
 * --add-root-option parameter. Additionally the --device= parameter must be
 * specified so that kernel-install can determine the UUID of the device.
 *
 * Now for an example using most of the available features:
 *
 * kernel-install --device=/dev/sdd --root=/dev/sdd2 --title="Linux-3.12" \
 *	--kernel-version="3.12" --kernel=/some/zImage \
 *	--devicetree=/some/devicetree --initrd=/some/initrd \
 *	--add-root-option --options="console=ttyS0,115200"
 *
 * This would install a kernel on /dev/sdd. The /boot partition would be found
 * automatically, the root partition has to be specified due to the usage of
 * --add-root-option
 *
 * BUGS:
 * - Currently only DOS partition tables are supported. There's no support
 *   for GPT yet.
 *
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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>
#include <dirent.h>

static int verbose;
static int force;
static int interactive = 1;
static int remove_kernel_num = -1;
static int set_default_num = -1;
static int set_once_num = -1;

static char *host_root_path, *kernel_image, *options, *device_path;
static char *host_boot_path, *kernel_version, *title, *machine_id;
static char *initrd_image, *devicetree_image;
static char *host_mount_root_path, *host_mount_boot_path;

static uint32_t nt_disk_signature;
static int root_partition_num;

struct loader_entry {
	char *title;
	char *machine_id;
	char *options;
	char *kernel;
	char *devicetree;
	char *initrd;
	char *version;
	char *host_path;
	char *config_file;
	int num;
	struct loader_entry *next;
};

static struct loader_entry *loader_entries;

static void loader_entry_var_set(struct loader_entry *e, const char *name, char *val)
{
	if (!strcmp(name, "title"))
		e->title = val;
	else if (!strcmp(name, "machine-id"))
		e->machine_id = val;
	else if (!strcmp(name, "options"))
		e->options = val;
	else if (!strcmp(name, "linux"))
		e->kernel = val;
	else if (!strcmp(name, "devicetree"))
		e->devicetree = val;
	else if (!strcmp(name, "initrd"))
		e->initrd = val;
	else if (!strcmp(name, "version"))
		e->version = val;
}

static struct loader_entry *loader_entry_open(const char *path)
{
	FILE *f;
	struct loader_entry *e;
	int ret;

	f = fopen(path, "r");
	if (!f)
		return NULL;

	e = calloc(sizeof(*e), 1);

	e->host_path = strdup(path);

	while (1) {
		char *line = NULL;
		char *name, *val, *end;
		size_t s;

		ret = getline(&line, &s, f);
		if (ret < 0)
			break;

		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

		name = line;
		end = name;

		while (*end && (*end != ' ' && *end != '\t'))
			end++;

		if (*line == '#') {
			free(line);
			continue;
		}

		if (!*end) {
			loader_entry_var_set(e, name, NULL);
			continue;
		}

		*end = 0;

		end++;

		while (*end == ' ' || *end == '\t')
			end++;

		if (!*end) {
			loader_entry_var_set(e, name, NULL);
			continue;
		}

		val = end;

		loader_entry_var_set(e, name, val);
	}

	fclose(f);

	return e;
}

/*
 * printf wrapper around 'system'
 */
static int systemp(const char *fmt, ...)
{
	va_list args;
	char *buf;
	int ret;

	va_start (args, fmt);

	ret = vasprintf(&buf, fmt, args);

	va_end (args);

	if (ret < 0) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	if (verbose)
		fprintf(stderr, "executing command: %s\n", buf);

	ret = system(buf);

	if (ret > 0)
		ret = WEXITSTATUS(ret);

	free(buf);

	return ret;
}

static void *safe_asprintf(const char *fmt, ...)
{
	va_list args;
	char *buf = NULL;
	int ret;

	va_start (args, fmt);

	ret = vasprintf(&buf, fmt, args);

	va_end (args);

	if (ret < 0) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	return buf;
}

static void verbose_printf(const char *fmt, ...)
{
	va_list args;

	if (!verbose)
		return;

	va_start (args, fmt);

	vprintf(fmt, args);

	va_end (args);
}

static int make_directory(const char *dir)
{
	char *s = strdup(dir);
	char *path = s;
	char c;
	int ret = 0;

	do {
		c = 0;

		/* Bypass leading non-'/'s and then subsequent '/'s. */
		while (*s) {
			if (*s == '/') {
				do {
					++s;
				} while (*s == '/');
				c = *s;		/* Save the current char */
				*s = 0;		/* and replace it with nul. */
				break;
			}
			++s;
		}

		if (mkdir(path, 0777) < 0) {

			/* If we failed for any other reason than the directory
			 * already exists, output a diagnostic and return -1.*/
			if (errno != EEXIST) {
				ret = -errno;
				break;
			}
		}
		if (!c)
			goto out;

		/* Remove any inserted nul from the path (recursive mode). */
		*s = c;

	} while (1);

out:
	free(path);
	if (ret)
		errno = -ret;
	return ret;
}

static int append_option(const char *fmt, ...)
{
	va_list args;
	char *buf;
	int ret;

	va_start (args, fmt);

	ret = vasprintf(&buf,  fmt, args);

	va_end (args);

	if (ret < 0) {
		fprintf(stderr, "out of memory\n");
		exit (1);
	}

	if (options) {
		char *new_options = safe_asprintf("%s %s", options, buf);
		free(options);
		free(buf);
		options = new_options;
	} else {
		options = buf;
	}

	return 0;
}

static char *get_mount_path(char *path)
{
	FILE *f;
	int ret;
	char *out_path = NULL;

	f = fopen("/proc/mounts", "r");
	if (!f) {
		fprintf(stderr, "Cannot open /proc/mounts: %s\n", strerror(errno));
		return NULL;
	}

	while (1) {
		char *line = NULL, *delim;
		size_t insize;

		ret = getline(&line, &insize, f);
		if (ret < 0)
			break;

		delim = strchr(line, ' ');
		if (!delim) {
			free(line);
			continue;
		}

		*delim = 0;

		if (strcmp(line, path)) {
			free(line);
			continue;
		}

		delim++;

		out_path = delim;

		delim = strchr(delim, ' ');
		if (!delim) {
			free(line);
			out_path = NULL;
			break;
		}

		*delim = 0;
		break;
	}

	fclose(f);

	if (out_path)
		return strdup(out_path);
	else
		return NULL;
}

enum mount_type {
	MOUNT_UNKNOWN,
	MOUNT_PMOUNT,
	MOUNT_MOUNT,
	MOUNT_ERROR,
};

static enum mount_type get_mount_type(void)
{
	static enum mount_type mount_type = MOUNT_UNKNOWN;
	int ret;
	uid_t uid;

	if (mount_type != MOUNT_UNKNOWN)
		return mount_type;

	ret = systemp("which pmount");
	if (!ret) {
		mount_type = MOUNT_PMOUNT;
		goto out;
	}

	verbose_printf("pmount not found\n");

	uid = getuid();
	if (uid == 0) {
		mount_type = MOUNT_MOUNT;
		goto out;
	}

	fprintf(stderr, "'pmount' not found and I am not root. Unable to mount\n");
	mount_type = MOUNT_ERROR;
out:
	return mount_type;
}

static char *mount_path_pmount(char *in_path)
{
	char *out_path;
	int ret;

	ret = systemp("pmount %s", in_path);
	if (ret) {
		fprintf(stderr, "failed to pmount %s\n", in_path);
		return NULL;
	}

	out_path = safe_asprintf("/media/%s", basename(in_path));

	return out_path;
}

static char *mount_path_mount(char *in_path)
{
	char *out_path, *str;
	int ret;

	str = safe_asprintf("/tmp/kernel-install-%s-XXXXXX", basename(in_path));

	out_path = mkdtemp(str);
	if (!out_path) {
		fprintf(stderr, "unable to create temporary directory: %s\n",
				strerror(errno));
		free(str);
		return NULL;
	}

	ret = systemp("mount %s %s", in_path, out_path);
	if (ret) {
		fprintf(stderr, "failed to mount %s: %s\n", in_path,
				strerror(errno));
		rmdir(out_path);
		free(out_path);
		return NULL;
	}

	return out_path;
}

/*
 * mount_path - make a device or directory available.
 * @in_path:   the input device or directory
 * @newmount:  if this function mounts a device, this variable is true
 *             on exit.
 *
 * returns the path under which the device is available.
 *
 * We do our best to make a device or directory available. If the input
 * path is a directory, just return it. If it is a block device and the
 * device is already mounted according to /proc/mounts, return the path
 * where it's mounted. If it's not mounted already try to mount it. We
 * first try pmount if that's available. If not, see if we are root and
 * can use regular 'mount'.
 */
static char *mount_path(char *in_path, int *newmount)
{
	struct stat s;
	int ret;
	char *out_path;

	*newmount = 0;

	ret = stat(in_path, &s);
	if (ret) {
		fprintf(stderr, "Cannot mount %s: %s\n", in_path, strerror(errno));
		return NULL;
	}

	if (S_ISDIR(s.st_mode))
		return strdup(in_path);

	if (!S_ISBLK(s.st_mode)) {
		fprintf(stderr, "%s is not a directory and not a block device\n",
				in_path);
		return NULL;
	}

	out_path = get_mount_path(in_path);
	if (out_path) {
		verbose_printf("%s already mounted at %s\n", in_path, out_path);
		return out_path;
	}

	switch (get_mount_type()) {
	default:
	case MOUNT_ERROR:
		return NULL;
	case MOUNT_PMOUNT:
		out_path = mount_path_pmount(in_path);
		if (out_path) {
			*newmount = 1;
			return out_path;
		}
		fprintf(stderr, "cannot mount %s\n", in_path);
		return NULL;
	case MOUNT_MOUNT:
		out_path = mount_path_mount(in_path);
		if (out_path) {
			*newmount = 1;
			return out_path;
		}
		fprintf(stderr, "cannot mount %s\n", in_path);
		return NULL;
	}


	return NULL;
}

static void detect_root_partition_num(char *device)
{
	struct stat s;
	int ret;
	char digit;

	ret = stat(device, &s);
	if (ret) {
		fprintf(stderr, "%s: %s\n", device, strerror(errno));
		return;
	}

	if (!S_ISBLK(s.st_mode))
		return;

	digit = device[strlen(device) - 1];
	if (!isdigit(digit))
		return;

	root_partition_num = digit - '0';
	printf("rootnum: %d\n", root_partition_num);
}

static void umount_path(const char *path)
{
	switch (get_mount_type()) {
	case MOUNT_PMOUNT:
		systemp("pumount %s", path);
		break;
	case MOUNT_MOUNT:
		systemp("umount %s", path);
		break;
	default:
	case MOUNT_ERROR:
		break;
	}
}

static int determine_root_boot_path(const char *device_path)
{
	unsigned char *buf;
	int ret, fd, i;
	char *partname;
	struct stat s;

	buf = malloc(512);
	if (!buf)
		return -ENOMEM;

	fd = open(device_path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -errno;
	}

	ret = read(fd, buf, 512);
	if (ret < 512)
		perror("read");

	close(fd);

	if (ret < 512)
		return -errno;

	if (buf[510] != 0x55 || buf[511] != 0xaa) {
		fprintf(stderr, "not a DOS bootsector\n");
		return EINVAL;
	}

	nt_disk_signature = buf[440] | (buf[441] << 8) | (buf[442] << 16) | (buf[443] << 24);

	for (i = 0; i < 4; i++) {
		uint8_t type = buf[446 + 4 + i * 64];
		if (type == 0xea) {
			verbose_printf("using partition %d as /boot\n", i);
			break;
		}
	}

	if (i == 4 && !host_boot_path) {
		fprintf(stderr, "cannot find a valid /boot partition on %s\n",
				device_path);
		return -EINVAL;
	}

	/* /dev/sdgx */
	partname = safe_asprintf("%s%c", device_path, '1' + i);
	ret = stat(partname, &s);
	if (!ret) {
		host_boot_path = partname;
		return 0;
	}

	free(partname);

	/* /dev/mmcblkxpy */
	partname = safe_asprintf("%sp%c", device_path, '1' + i);
	ret = stat(partname, &s);
	if (!ret) {
		host_boot_path = partname;
		return 0;
	}

	free(partname);

	/* /dev/disk/by-xxx/xxx-party */
	partname = safe_asprintf("%s-part%c", device_path, '1' + i);
	ret = stat(partname, &s);
	if (!ret) {
		host_boot_path = partname;
		return 0;
	}

	free(partname);

	return 0;
}

static int determine_machine_id(void)
{
	char buf[512] = {};
	int fd, ret;
	char *path, *tmp;

	if (machine_id)
		return 0;

	if (!host_root_path)
		return -EINVAL;

	path = safe_asprintf("%s/etc/machine-id", host_root_path);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -errno;
	}

	ret = read(fd, buf, 512);
	if (ret < 0) {
		perror("read");
		goto out;
	}

	if (ret == 512) {
		fprintf(stderr, "machine-id file too big\n");
		ret = -EINVAL;
		goto out;
	}

	tmp = buf;
	while (*tmp) {
		if (!isalnum(*tmp)) {
			*tmp = '\0';
			break;
		}
		tmp++;
	}

	machine_id = strdup(buf);

	ret = 0;
out:
	close(fd);
	return ret;
}

static void cleanup(void)
{
	if (host_mount_root_path)
		umount_path(host_mount_root_path);
	if (host_mount_boot_path)
		umount_path(host_mount_boot_path);
}

static int yesno(const char *str)
{
	int ch;

	if (force)
		return 0;
	if (!interactive)
		return 1;
	printf("%s", str);

	ch = getchar();
	if (ch == 'y')
		return 0;
	return 1;
}

static int do_add_kernel(void)
{
	char *conf_path, *conf_file, *conf_dir, *images_dir;
	char *kernel_path, *host_images_dir, *host_kernel_path;
	char *initrd_path, *host_initrd_path;
	char *devicetree_path, *host_devicetree_path;
	int ret, fd;
	struct stat s;

	ret = determine_machine_id();
	if (ret) {
		fprintf(stderr, "failed to determine machine-id\n");
		return -EINVAL;
	}

	if (!machine_id) {
		fprintf(stderr, "No machine-id given\n");
		return -EINVAL;
	}

	if (!kernel_version) {
		fprintf(stderr, "no Kernel version given\n");
		return -EINVAL;
	}

	if (!kernel_image) {
		fprintf(stderr, "No Linux image given\n");
		return -EINVAL;
	}

	conf_dir = safe_asprintf("%s/loader/entries", host_boot_path);
	conf_file = safe_asprintf("%s-%s.conf", machine_id, kernel_version);
	conf_path = safe_asprintf("%s/%s", conf_dir, conf_file);
	images_dir = safe_asprintf("%s/%s", machine_id, kernel_version);
	host_images_dir = safe_asprintf("%s/%s", host_boot_path, images_dir);
	kernel_path = safe_asprintf("%s/linux", images_dir);
	host_kernel_path = safe_asprintf("%s/linux", host_images_dir);
	initrd_path = safe_asprintf("%s/initrd", images_dir);
	host_initrd_path = safe_asprintf("%s/initrd", host_images_dir);
	devicetree_path = safe_asprintf("%s/devicetree", images_dir);
	host_devicetree_path = safe_asprintf("%s/devicetree", host_images_dir);

	ret = stat(conf_path, &s);
	if (!ret) {
		fprintf(stderr, "entry %s already exists.\n", conf_file);
		ret = yesno("overwrite? (y/n) ");
		if (ret)
			return -EINVAL;
	}

	ret = make_directory(conf_dir);
	if (ret) {
		fprintf(stderr, "failed to create directory %s: %s\n",
				conf_dir, strerror(errno));
		return ret;
	}

	ret = make_directory(host_images_dir);
	if (ret) {
		fprintf(stderr, "failed to create directory %s: %s\n",
				host_images_dir, strerror(errno));
		return ret;
	}

	fd = open(conf_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "failed to create %s: %s\n", conf_path, strerror(errno));
		return -errno;
	}

	dprintf(fd, "title %s\n", title);
	dprintf(fd, "version %s\n", kernel_version);
	dprintf(fd, "machine-id %s\n", machine_id);
	if (options)
		dprintf(fd, "options %s\n", options);
	dprintf(fd, "linux %s\n", kernel_path);
	if (initrd_image)
		dprintf(fd, "initrd %s\n", initrd_path);
	if (devicetree_image)
		dprintf(fd, "devicetree %s\n", devicetree_path);

	ret = close(fd);
	if (ret)
		return ret;

	ret = systemp("cp %s %s", kernel_image, host_kernel_path);
	if (ret) {
		fprintf(stderr, "unable to copy kernel image\n");
		return ret;
	}

	if (initrd_image) {
		ret = systemp("cp %s %s", initrd_image, host_initrd_path);
		if (ret) {
			fprintf(stderr, "unable to copy initrd image\n");
			return ret;
		}
	}

	if (devicetree_image) {
		ret = systemp("cp %s %s", devicetree_image, host_devicetree_path);
		if (ret) {
			fprintf(stderr, "unable to copy devicetree image\n");
			return ret;
		}
	}

	printf("written config file: %s\n", conf_path);

	return 0;
}

static int do_open_entries(void)
{
	DIR *dir;
	char *entries, *entry_path;
	struct loader_entry *e = NULL, *first = NULL;
	int i = 0;

	if (loader_entries)
		return 0;

	entries = safe_asprintf("%s/loader/entries", host_boot_path);

	dir = opendir(entries);
	if (!dir) {
		fprintf(stderr, "cannot open %s\n", entries);
		return -errno;
	}

	while (1) {
		struct dirent *ent;
		struct loader_entry *tmp;

		ent = readdir(dir);
		if (!ent)
			break;
		if (ent->d_name[0] == '.')
			continue;
		entry_path = safe_asprintf("%s/%s", entries, ent->d_name);

		tmp = loader_entry_open(entry_path);
		if (!tmp) {
			fprintf(stderr, "cannot open %s\n", entry_path);
			break;
		}

		tmp->config_file = strdup(ent->d_name);

		tmp->num = i++;

		if (first)
			e->next = tmp;
		else
			first = tmp;

		e = tmp;
	}

	closedir(dir);

	loader_entries = first;

	return 0;
}

static struct loader_entry *loader_entry_by_num(int num)
{
	struct loader_entry *e;

	e = loader_entries;

	while (e) {
		if (e->num == num)
			return e;
		e = e->next;
	}

	return NULL;
}

static int do_list_entries(void)
{
	struct loader_entry *e;
	int ret;

	ret = do_open_entries();
	if (ret)
		return ret;

	e = loader_entries;

	while (e) {
		printf("Entry %d:\n", e->num);

		if (e->title)
			printf("\ttitle:      %s\n", e->title);
		if (e->version)
			printf("\tversion:    %s\n", e->version);
		if (e->machine_id)
			printf("\tmachine_id: %s\n", e->machine_id);
		if (e->options)
			printf("\toptions:    %s\n", e->options);
		if (e->kernel)
			printf("\tlinux:      %s\n", e->kernel);
		if (e->devicetree)
			printf("\tdevicetree: %s\n", e->devicetree);
		if (e->initrd)
			printf("\tinitrd:     %s\n", e->initrd);
		e = e->next;
	}

	return 0;
}

static int is_file_referenced(const char *filename)
{
	struct loader_entry *e = loader_entries;

	while (e) {
		if (e->kernel && !strcmp(e->kernel, filename))
			return 1;
		if (e->initrd && !strcmp(e->initrd, filename))
			return 1;
		if (e->devicetree && !strcmp(e->devicetree, filename))
			return 1;
		e = e->next;
	}

	return 0;
}

static int remove_if_unreferenced(const char *filename)
{
	char *path, *dir;
	int ret;

	if (!filename)
		return -EINVAL;

	if (is_file_referenced(filename))
		return -EBUSY;

	path = safe_asprintf("%s/%s", host_boot_path, filename);

	verbose_printf("removing unrefenced %s\n", path);

	ret = unlink(path);
	if (ret) {
		fprintf(stderr, "cannot remove %s: %s\n", path, strerror(errno));
		return ret;
	}

	dir = dirname(path);
	rmdir(dir);
	dir = dirname(path);
	rmdir(dir);

	free(path);

	return 0;
}

static int do_remove_kernel(void)
{
	char *input = NULL;
	size_t insize;
	int remove_num = -1;
	struct loader_entry *e;
	int ret;
	char *kernel, *devicetree, *initrd;

	do_open_entries();

	if (!loader_entries) {
		fprintf(stderr, "No entries to remove\n");
		return -ENOENT;
	}

	if (remove_kernel_num >= 0)
		remove_num = remove_kernel_num;

	if (remove_num < 0 && interactive) {
		do_list_entries();
		printf("which kernel do you like to remove?\n");
		ret = getline(&input, &insize, stdin);
		if (ret)
			return -errno;
		if (!strlen(input))
			return -EINVAL;
		if (!isdigit(*input))
			return -EINVAL;
		remove_num = atoi(input);
	}

	if (remove_num < 0) {
		fprintf(stderr, "no entry number given\n");
		return -EINVAL;
	}

	e = loader_entry_by_num(remove_num);
	if (!e) {
		fprintf(stderr, "no entry with num %d\n", remove_num);
		return -ENOENT;
	}

	verbose_printf("removing entry %s\n", e->host_path);

	ret = unlink(e->host_path);
	if (ret) {
		fprintf(stderr, "cannot remove %s\n", e->host_path);
		return -errno;
	}

	kernel = e->kernel;
	devicetree = e->devicetree;
	initrd = e->initrd;

	e->kernel = NULL;
	e->devicetree = NULL;
	e->initrd = NULL;

	remove_if_unreferenced(kernel);
	remove_if_unreferenced(initrd);
	remove_if_unreferenced(devicetree);

	return 0;
}

static int do_set_once_default(const char *name, int entry)
{
	int fd, ret;
	struct loader_entry *e;
	char *host_default_path;

	do_open_entries();

	if (!loader_entries) {
		fprintf(stderr, "No entries found\n");
		return -ENOENT;
	}

	if (entry < 0 && interactive) {
		char *input = NULL;
		size_t insize;

		do_list_entries();

		printf("\nwhich entry shall be used?\n");
		ret = getline(&input, &insize, stdin);
		if (ret < 0)
			return -errno;
		if (!strlen(input))
			return -EINVAL;
		if (!isdigit(*input))
			return -EINVAL;
		entry = atoi(input);
	}

	if (entry < 0) {
		fprintf(stderr, "no entry number given\n");
		return -EINVAL;
	}

	e = loader_entry_by_num(entry);
	if (!e) {
		fprintf(stderr, "no entry with num %d\n", entry);
		return -ENOENT;
	}

	host_default_path = safe_asprintf("%s/%s", host_boot_path, name);

	fd = open(host_default_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "failed to create %s: %s\n", host_default_path, strerror(errno));
		return -errno;
	}

	dprintf(fd, "loader/entries/%s\n", e->config_file);

	ret = close(fd);
	if (ret)
		return ret;

	return 0;
}

static int do_set_default(void)
{
	return do_set_once_default("default", set_default_num);
}

static int do_set_once(void)
{
	return do_set_once_default("once", set_once_num);
}

enum opt {
	OPT_KERNEL = 1,
	OPT_INITRD,
	OPT_DEVICETREE,
	OPT_ONCE,
	OPT_ROOT_PARTITION_NO,
	OPT_DEVICE,
	OPT_ADD_ROOT_ARGUMENT,
};

static struct option long_options[] = {
	{"add",		no_argument,		0,	'a' },
	{"remove",	optional_argument,	0,	'r' },
	{"list",	no_argument,		0,	'l' },
	{"default",	optional_argument,	0,	'd' },
	{"once",	optional_argument,	0,	OPT_ONCE },
	{"device",	required_argument,	0,	OPT_DEVICE },
	{"root",	required_argument,	0,	'R' },
	{"boot",	required_argument,	0,	'b' },
	{"kernel-version", required_argument,	0,	'k' },
	{"title",	required_argument,	0,	't' },
	{"machine-id",	required_argument,	0,	'm' },
	{"kernel",	required_argument,	0,	OPT_KERNEL },
	{"initrd",	required_argument,	0,	OPT_INITRD },
	{"devicetree",	required_argument,	0,	OPT_DEVICETREE },
	{"options",	required_argument,	0,	'o' },
	{"verbose",	no_argument,		0,	'v' },
	{"add-root-option", no_argument,	0,	OPT_ADD_ROOT_ARGUMENT },
	{"num-root-part", required_argument,	0,	OPT_ROOT_PARTITION_NO },
	{"help",	 no_argument,		0,	'h' },
	{0,		0,			0,	0 }
};

static void usage(char *name)
{
	printf(
"Usage: %s [OPTIONS]\n"
"Install, uninstall and list kernels according to the bootloader spec\n"
"\n"
"command options, exactly one must be present:\n"
"\n"
"-a, --add                      Add a new boot entry\n"
"-r, --remove[=num]             Remove a boot entry. If <num> is not present\n"
"                               ask for it interactively\n"
"-l, --list                     List all available entries\n"
"-d, --default[=num]            Make an entry the default. If <num> is not\n"
"                               present ask for it interactively\n"
"--once[=num]                   start an entry once\n"
"\n"
"other options:\n"
"\n"
"--kernel-version=<version>     Specify kernel version, used for generating\n"
"                               config filenames/directories. must be unique\n"
"                               for each installed operating system\n"
"--title=<name>                 Title for the entry. If unspecified defaults\n"
"                               to \"Linux-<version>\"\n"
"--machine-id=<id>              Specify machine id. Should be unique for each\n"
"                               installation. Can be left unspecified when it\n"
"                               can be read from <rootpath>/etc/machine-id.\n"
"--kernel=<kernel>              Path to the kernel to install\n"
"--initrd=<initrd>              Path to the initrd to install, optional\n"
"--devicetree=<devicetree>      Path to the devicetree to install, optional\n"
"-o, --options=<options>        Commandline options for the kernel, can be\n"
"                               given multiple times\n"
"-v, --verbose                  Be more verbose\n"
"--add-root-option              If present, add a \"root=PARTUUID=xxxxxxxx-yy\"\n"
"                               option to the kernel commandline. The partuuid\n"
"                               is determined from the device given with the\n"
"                               --device option and the partition number\n"
"                               determined from either the device specified\n"
"                               with --root or from the --num-root-part option.\n"
"--num-root-part=<partnum>      Specify partition number for --add-root-option\n"
"-h, --help                     This help\n"
"\n"
"Options for non-native mode:\n"
"\n"
"Each of the following options disables native mode. Useful for preparing\n"
"boot media on another host.\n"
"--device=<devicepath>          Specify device to work on\n"
"--root=<path|device>           Specify path or device to use as '/', defaults to '/'\n"
"--boot=<path|device>           Specify path or device to use as '/boot', defaults to '/boot'\n",
	name);
}

int main(int argc, char *argv[])
{
	int c;
	char *root = NULL;
	int option_index, add_kernel = 0, remove_kernel = 0, add_root_argument = 0;
	int ret, list = 0, set_default = 0, newmount;
	int native_mode = 1, set_once = 0;

	while (1) {
		c = getopt_long(argc, argv, "b:R:d:k:p:m:lo:aruvh", long_options, &option_index);
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'b':
			native_mode = 0;
			host_boot_path = optarg;
			break;
		case 'R':
			native_mode = 0;
			root = optarg;
			break;
		case 'l':
			list = 1;
			break;
		case 'd':
			set_default = 1;
			if (optarg)
				set_default_num = atoi(optarg);
			break;
		case OPT_ONCE:
			set_once = 1;
			if (optarg)
				set_once_num = atoi(optarg);
			break;
		case OPT_DEVICE:
			native_mode = 0;
			device_path = optarg;
			break;
		case 'k':
			kernel_version = optarg;
			break;
		case 't':
			title = optarg;
			break;
		case 'm':
			machine_id = optarg;
			break;
		case OPT_KERNEL:
			kernel_image = optarg;
			break;
		case 'o':
			append_option("%s", optarg);
			break;
		case 'a':
			add_kernel = 1;
			break;
		case 'r':
			remove_kernel = 1;
			if (optarg)
				remove_kernel_num = atoi(optarg);
			break;
		case OPT_ADD_ROOT_ARGUMENT:
			add_root_argument = 1;
			break;
		case OPT_ROOT_PARTITION_NO:
			root_partition_num = atoi(optarg);
			break;
		case OPT_INITRD:
			initrd_image = optarg;
			break;
		case OPT_DEVICETREE:
			devicetree_image = optarg;
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	if (!list && !remove_kernel && !set_default && !add_kernel && !set_once) {
		fprintf(stderr, "no command given\n");
		exit (1);
	}

	if (native_mode) {
		host_boot_path = "/boot";
		host_root_path = "";
	}

	if (device_path) {
		ret = determine_root_boot_path(device_path);
		if (ret)
			exit(1);
	}

	if (host_boot_path) {
		verbose_printf("using partition %s for /boot as determined by device argument\n",
				host_boot_path);
	}

	if (!host_boot_path) {
		fprintf(stderr, "No partition or directory given for /boot\n");
		goto out;
	}

	host_boot_path = mount_path(host_boot_path, &newmount);
	if (!host_boot_path)
		goto out;

	if (newmount)
		host_mount_boot_path = host_boot_path;

	if (root) {
		host_root_path = mount_path(root, &newmount);
		if (!host_root_path)
			goto out;
		if (newmount)
			host_mount_root_path = host_root_path;
	}

	if (!title)
		title = safe_asprintf("Linux-%s", kernel_version);

	if (add_root_argument) {
		if (!nt_disk_signature) {
			fprintf(stderr, "no nt disk signature found for root-uuid\n"
					"Cannot add root argument\n");
			goto out;
		}

		if (!root_partition_num) {
			if (!root) {
				fprintf(stderr, "no root partition number and no device for / given\n"
						"Cannot add root argument\n");
				goto out;
			}

			detect_root_partition_num(root);
		}

		if (!root_partition_num) {
			fprintf(stderr, "no root partition number given\n"
					"Cannot add root argument\n");

			goto out;
		}

		append_option("root=PARTUUID=%08X-%02d", nt_disk_signature, root_partition_num);
	}

	if (list) {
		ret = do_list_entries();
		goto out;
	}

	if (remove_kernel) {
		ret = do_remove_kernel();
		goto out;
	}

	if (set_default) {
		ret = do_set_default();
		goto out;
	}

	if (set_once) {
		ret = do_set_once();
		goto out;
	}

	if (add_kernel) {
		ret = do_add_kernel();
		goto out;
	}

	ret = 0;

out:
	cleanup();
	exit(ret == 0 ? 0 : 1);
}
