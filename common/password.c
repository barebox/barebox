/*
 * Copyright (c) 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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

#include <common.h>
#include <password.h>
#include <errno.h>
#include <readkey.h>
#include <fs.h>
#include <fcntl.h>
#include <digest.h>
#include <malloc.h>
#include <xfuncs.h>
#include <clock.h>
#include <generated/passwd.h>

#if defined(CONFIG_PASSWD_SUM_MD5)
#define PASSWD_SUM "md5"
#elif defined(CONFIG_PASSWD_SUM_SHA1)
#define PASSWD_SUM "sha1"
#elif defined(CONFIG_PASSWD_SUM_SHA256)
#define PASSWD_SUM "sha256"
#endif

int password(unsigned char *passwd, size_t length, int flags, int timeout)
{
	unsigned char *buf = passwd;
	int pos = 0;
	unsigned char ch;
	uint64_t start;

	if (!passwd)
		return -EINVAL;

	start = get_time_ns();

	do {
		if (tstc()) {
			ch = getc();

			switch (ch) {
			case '\r':
			case '\n':
				*buf = '\0';
				puts("\r\n");
				return pos;
			case '\0':
			case '\t':
				continue;
			case CTL_CH('c'):
				passwd[0] = '\0';
				puts("\r\n");
				return 0;
			case CTL_CH('h'):
			case KEY_DEL7:
			case KEY_DEL:
				if (pos > 0) {
					if (flags & STAR)
						puts("\b \b");

					*buf = '\0';
					buf--;
					pos--;
				}
				continue;
			default:
				if (pos < length - 1) {
					if (flags & STAR)
						putchar('*');
					else if (flags & CLEAR)
						putchar(ch);

					*buf = ch;
					buf++;
					pos++;
				} else {
					if (flags & STAR)
						putchar('\a');
				}
			}
		}
	} while (!is_timeout(start, timeout * SECOND) || timeout == 0);

	return -1;
}
EXPORT_SYMBOL(password);

int is_passwd_default_enable(void)
{
	return strlen(default_passwd) > 0;
}
EXPORT_SYMBOL(is_passwd_default_enable);

int is_passwd_env_enable(void)
{
	int fd;

	fd = open(PASSWD_FILE, O_RDONLY);

	if (fd < 0)
		return 0;

	close(fd);

	return 1;
}
EXPORT_SYMBOL(is_passwd_env_enable);

int passwd_env_disable(void)
{
	return unlink(PASSWD_FILE);
}
EXPORT_SYMBOL(passwd_env_disable);

static unsigned char to_digit(unsigned char c)
{
	if (c >= '0' && c <= '9')
		c -= '0';
	else
		c -= 'a' - 10;

	return c;
}

static unsigned char to_hexa(unsigned char c)
{
	if (c < 10)
		c += '0';
	else
		c += 'a' - 10;

	return c;
}

int read_passwd(unsigned char *sum, size_t length)
{
	if (is_passwd_env_enable())
		return read_env_passwd(sum, length);
	else if (is_passwd_default_enable())
		return read_default_passwd(sum, length);
	else
		return -EINVAL;
}

int read_default_passwd(unsigned char *sum, size_t length)
{
	int i = 0;
	int len = strlen(default_passwd);
	unsigned char *buf = (unsigned char *)default_passwd;
	unsigned char c;

	if (!sum || length < 1)
		return -EINVAL;

	for (i = 0; i < len && length > 0; i++) {
		c = buf[i];
		i++;

		*sum = to_digit(c) << 4;

		c = buf[i];

		*sum |= to_digit(c);
		sum++;
		length--;
	}

	return 0;
}
EXPORT_SYMBOL(read_default_passwd);

int read_env_passwd(unsigned char *sum, size_t length)
{
	int fd;
	int ret = 0;
	unsigned char c;

	if (!sum && length < 1)
		return -EINVAL;

	fd = open(PASSWD_FILE, O_RDONLY);

	if (fd < 0)
		return fd;

	do {
		ret = read(fd, &c, sizeof(char));

		if (ret < 0)
			goto exit;

		*sum = to_digit(c) << 4;

		ret = read(fd, &c, sizeof(char));

		if (ret < 0)
			goto exit;

		*sum |= to_digit(c);
		sum++;
		length--;
	} while(length > 0);

exit:

	ret = 0;

	close(fd);

	return ret;
}
EXPORT_SYMBOL(read_env_passwd);

int write_env_passwd(unsigned char *sum, size_t length)
{
	int fd;
	unsigned char c;
	int ret = 0;

	if (!sum && length < 1)
		return -EINVAL;

	fd = open(PASSWD_DIR, O_RDONLY);

	if (fd < 0)
		mkdir(PASSWD_DIR, 644);

	close(fd);

	fd = open(PASSWD_FILE, O_WRONLY | O_CREAT, 600);

	if (fd < 0)
		return fd;

	do {
		c = to_hexa(*sum >> 4 & 0xf);

		ret = write(fd, &c, sizeof(unsigned char));

		if (ret < 0)
			goto exit;

		c = to_hexa(*sum & 0xf);

		ret = write(fd, &c, sizeof(unsigned char));

		if (ret < 0)
			goto exit;

		sum++;
		length--;
	} while(length > 0);

	ret = 0;

exit:
	close(fd);

	return ret;
}
EXPORT_SYMBOL(write_env_passwd);

static int __check_passwd(unsigned char* passwd, size_t length, int std)
{
	struct digest *d;
	unsigned char *passwd1_sum;
	unsigned char *passwd2_sum;
	int ret = 0;

	d = digest_get_by_name(PASSWD_SUM);

	passwd1_sum = calloc(d->length, sizeof(unsigned char));

	if (!passwd1_sum)
		return -ENOMEM;

	passwd2_sum = calloc(d->length, sizeof(unsigned char));

	if (!passwd2_sum) {
		ret = -ENOMEM;
		goto err1;
	}

	d->init(d);

	d->update(d, passwd, length);

	d->final(d, passwd1_sum);

	if (std)
		ret = read_env_passwd(passwd2_sum, d->length);
	else
		ret = read_default_passwd(passwd2_sum, d->length);

	if (ret < 0)
		goto err2;

	if (strncmp(passwd1_sum, passwd2_sum, d->length) == 0)
		ret = 1;

err2:
	free(passwd2_sum);
err1:
	free(passwd1_sum);

	return ret;
}

int check_default_passwd(unsigned char* passwd, size_t length)
{
	return __check_passwd(passwd, length, 0);
}
EXPORT_SYMBOL(check_default_passwd);

int check_env_passwd(unsigned char* passwd, size_t length)
{
	return __check_passwd(passwd, length, 1);
}
EXPORT_SYMBOL(check_env_passwd);

int check_passwd(unsigned char* passwd, size_t length)
{
	if (is_passwd_env_enable())
		return check_env_passwd(passwd, length);
	else if (is_passwd_default_enable())
		return check_default_passwd(passwd, length);
	else
		return -EINVAL;
}

int set_env_passwd(unsigned char* passwd, size_t length)
{
	struct digest *d;
	unsigned char *passwd_sum;
	int ret;

	d = digest_get_by_name(PASSWD_SUM);

	passwd_sum = calloc(d->length, sizeof(unsigned char));

	if (!passwd_sum)
		return -ENOMEM;

	d->init(d);

	d->update(d, passwd, length);

	d->final(d, passwd_sum);

	ret = write_env_passwd(passwd_sum, d->length);

	free(passwd_sum);

	return ret;
}
EXPORT_SYMBOL(set_env_passwd);
