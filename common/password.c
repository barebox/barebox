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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#if defined(CONFIG_PASSWD_SUM_MD5)
#define PASSWD_SUM "md5"
#elif defined(CONFIG_PASSWD_SUM_SHA1)
#define PASSWD_SUM "sha1"
#elif defined(CONFIG_PASSWD_SUM_SHA256)
#define PASSWD_SUM "sha256"
#endif

int password(unsigned char *passwd, size_t length, int flags)
{
	unsigned char *buf = passwd;
	int pos = 0;
	unsigned char ch;

	if (!passwd)
		return -EINVAL;

	do {
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
			if (flags & STAR && pos > 0)
				puts("\b \b");
			*buf = '\0';
			buf--;
			pos--;
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
	} while(1);
}

int is_passwd_enable(void)
{
	int fd;

	fd = open(PASSWD_FILE, O_RDONLY);

	if (fd < 0)
		return 0;

	close(fd);

	return 1;
}

int passwd_disable(void)
{
	return unlink(PASSWD_FILE);
}

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

int write_passwd(unsigned char *sum, size_t length)
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

int check_passwd(unsigned char* passwd, size_t length)
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

	ret = read_passwd(passwd2_sum, d->length);

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

int set_passwd(unsigned char* passwd, size_t length)
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

	ret = write_passwd(passwd_sum, d->length);

	free(passwd_sum);

	return ret;
}
