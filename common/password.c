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
#include <command.h>
#include <magicvar.h>
#include <clock.h>
#include <init.h>
#include <stdlib.h>
#include <globalvar.h>
#include <generated/passwd.h>
#include <crypto/pbkdf2.h>

#if defined(CONFIG_PASSWD_SUM_MD5)
#define PASSWD_SUM "md5"
#elif defined(CONFIG_PASSWD_SUM_SHA1)
#define PASSWD_SUM "sha1"
#elif defined(CONFIG_PASSWD_SUM_SHA256)
#define PASSWD_SUM "sha256"
#elif defined(CONFIG_PASSWD_SUM_SHA512)
#define PASSWD_SUM "sha512"
#else
#define PASSWD_SUM	NULL
#endif

#define PBKDF2_SALT_LEN	32
#define PBKDF2_LENGTH	64
#define PBKDF2_COUNT	10000

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
			ch = getchar();

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
				return -EINTR;
			case CTL_CH('h'):
			case BB_KEY_DEL7:
			case BB_KEY_DEL:
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

	return -ETIMEDOUT;
}
EXPORT_SYMBOL(password);

static int is_passwd_default_enable(void)
{
	return strlen(default_passwd) > 0;
}

static int is_passwd_env_enable(void)
{
	int fd;

	fd = open(PASSWD_FILE, O_RDONLY);

	if (fd < 0)
		return 0;

	close(fd);

	return 1;
}

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

static int read_default_passwd(unsigned char *sum, size_t length)
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

static int read_env_passwd(unsigned char *sum, size_t length)
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

static int write_env_passwd(unsigned char *sum, size_t length)
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

static int check_passwd(unsigned char *passwd, size_t length)
{
	struct digest *d = NULL;
	unsigned char *passwd1_sum;
	unsigned char *passwd2_sum;
	int ret = 0;
	int hash_len;

	if (IS_ENABLED(CONFIG_PASSWD_CRYPTO_PBKDF2)) {
		hash_len = PBKDF2_LENGTH;
	} else {
		d = digest_alloc(PASSWD_SUM);
		if (!d) {
			pr_err("No such digest: %s\n",
			       PASSWD_SUM ? PASSWD_SUM : "NULL");
			return -ENOENT;
		}

		hash_len = digest_length(d);
	}

	passwd1_sum = calloc(hash_len * 2, sizeof(unsigned char));
	if (!passwd1_sum)
		return -ENOMEM;

	passwd2_sum = passwd1_sum + hash_len;

	if (is_passwd_env_enable())
		ret = read_env_passwd(passwd2_sum, hash_len);
	else if (is_passwd_default_enable())
		ret = read_default_passwd(passwd2_sum, hash_len);
	else
		ret = -EINVAL;

	if (ret < 0)
		goto err;

	if (IS_ENABLED(CONFIG_PASSWD_CRYPTO_PBKDF2)) {
		char *key = passwd2_sum + PBKDF2_SALT_LEN;
		char *salt = passwd2_sum;
		int keylen = PBKDF2_LENGTH - PBKDF2_SALT_LEN;

		ret = pkcs5_pbkdf2_hmac_sha1(passwd, length, salt,
			PBKDF2_SALT_LEN, PBKDF2_COUNT, keylen, passwd1_sum);
		if (ret)
			goto err;

		if (strncmp(passwd1_sum, key, keylen) == 0)
			ret = 1;
	} else {
		ret = digest_digest(d, passwd, length, passwd1_sum);

		if (ret)
			goto err;

		if (strncmp(passwd1_sum, passwd2_sum, hash_len) == 0)
			ret = 1;
	}

err:
	free(passwd1_sum);
	digest_free(d);

	return ret;
}

int set_env_passwd(unsigned char* passwd, size_t length)
{
	struct digest *d = NULL;
	unsigned char *passwd_sum;
	int ret, hash_len;

	if (IS_ENABLED(CONFIG_PASSWD_CRYPTO_PBKDF2)) {
		hash_len = PBKDF2_LENGTH;
	} else {
		d = digest_alloc(PASSWD_SUM);
		if (!d)
			return -EINVAL;

		hash_len = digest_length(d);
	}

	passwd_sum = calloc(hash_len, sizeof(unsigned char));
	if (!passwd_sum)
		return -ENOMEM;

	if (IS_ENABLED(CONFIG_PASSWD_CRYPTO_PBKDF2)) {
		char *key = passwd_sum + PBKDF2_SALT_LEN;
		char *salt = passwd_sum;
		int keylen = PBKDF2_LENGTH - PBKDF2_SALT_LEN;

		ret = get_crypto_bytes(passwd_sum, PBKDF2_SALT_LEN);
		if (ret) {
			pr_err("Can't get random numbers\n");
			return ret;
		}

		ret = pkcs5_pbkdf2_hmac_sha1(passwd, length, salt,
				PBKDF2_SALT_LEN, PBKDF2_COUNT, keylen, key);
	} else {
		ret = digest_digest(d, passwd, length, passwd_sum);
	}
	if (ret)
		goto err;

	ret = write_env_passwd(passwd_sum, hash_len);

err:
	digest_free(d);
	free(passwd_sum);

	return ret;
}
EXPORT_SYMBOL(set_env_passwd);

#define PASSWD_MAX_LENGTH	(128 + 1)

#if defined(CONFIG_PASSWD_MODE_STAR)
#define LOGIN_MODE STAR
#elif defined(CONFIG_PASSWD_MODE_CLEAR)
#define LOGIN_MODE CLEAR
#else
#define LOGIN_MODE HIDE
#endif

static int logged_in;
static int login_timeout = 60;
static char *login_fail_command;

/**
 * login() - Prompt for password
 *
 * This function only returns when the correct password has been entered or
 * no password is necessary because either no password is configured or the
 * correct password has been entered in a previous call to this function.
 */
void login(void)
{
	unsigned char passwd[PASSWD_MAX_LENGTH];
	int ret;

	if (!is_passwd_default_enable() && !is_passwd_env_enable())
		return;

	if (logged_in)
		return;

	while (1) {
		printf("Password: ");

		ret = password(passwd, PASSWD_MAX_LENGTH, LOGIN_MODE, login_timeout);
		if (ret < 0)
			run_command(login_fail_command);

		if (ret < 0)
			continue;

		if (check_passwd(passwd, ret) != 1)
			continue;

		logged_in = 1;
		return;
	}
}

static int login_global_init(void)
{
	login_fail_command = xstrdup("boot");

	globalvar_add_simple_int("login.timeout", &login_timeout, "%d");
	globalvar_add_simple_string("login.fail_command", &login_fail_command);

	return 0;
}
late_initcall(login_global_init);

BAREBOX_MAGICVAR_NAMED(global_login_fail_command, global.login.fail_command,
		"command to run when password entry failed");
BAREBOX_MAGICVAR_NAMED(global_login_timeout, global.login.timeout,
		"timeout to type the password");
