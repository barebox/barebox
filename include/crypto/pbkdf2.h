/*
 * (C) Copyright 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 Only
 */

#ifndef __PBKDF2_H__
#define __PBKDF2_H__

#include <digest.h>

int pkcs5_pbkdf2_hmac_sha1(const unsigned char *pwd, size_t pwd_len,
			   const unsigned char *salt, size_t salt_len,
			   uint32_t iteration,
			   uint32_t key_len, unsigned char *buf);

int pkcs5_pbkdf2_hmac(struct digest* d,
		      const unsigned char *pwd, size_t pwd_len,
		      const unsigned char *salt, size_t salt_len,
		      uint32_t iteration,
		      uint32_t key_len, unsigned char *key);

#endif /* __PBKDF2_H__ */
