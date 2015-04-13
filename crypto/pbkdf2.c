/*
 * (C) Copyright 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 Only
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <crypto/pbkdf2.h>

int pkcs5_pbkdf2_hmac(struct digest* d,
		      const unsigned char *pwd, size_t pwd_len,
		      const unsigned char *salt, size_t salt_len,
		      uint32_t iteration,
		      uint32_t key_len, unsigned char *key)
{
	int i, j, k;
	unsigned char cnt[4];
	uint32_t pass_len;
	unsigned char *tmpdgt;
	uint32_t d_len;
	int ret;

	if (!d)
		return -EINVAL;

	d_len = digest_length(d);
	tmpdgt = malloc(d_len);
	if (!tmpdgt)
		return -ENOMEM;

	i = 1;

	ret = digest_set_key(d, pwd, pwd_len);
	if (ret)
		goto err;

	while (key_len) {
		pass_len = min(key_len, d_len);
		cnt[0] = (i >> 24) & 0xff;
		cnt[1] = (i >> 16) & 0xff;
		cnt[2] = (i >> 8) & 0xff;
		cnt[3] = i & 0xff;
		ret = digest_init(d);
		if (ret)
			goto err;
		ret = digest_update(d, salt, salt_len);
		if (ret)
			goto err;
		ret = digest_update(d, cnt, 4);
		if (ret)
			goto err;
		ret = digest_final(d, tmpdgt);
		if (ret)
			goto err;

		memcpy(key, tmpdgt, pass_len);

		for (j = 1; j < iteration; j++) {
			ret = digest_digest(d, tmpdgt, d_len, tmpdgt);
			if (ret)
				goto err;

			for(k = 0; k < pass_len; k++)
				key[k] ^= tmpdgt[k];
		}

		key_len -= pass_len;
		key += pass_len;
		i++;
	}

	ret = 0;
err:
	free(tmpdgt);

	return ret;;
}

int pkcs5_pbkdf2_hmac_sha1(const unsigned char *pwd, size_t pwd_len,
			   const unsigned char *salt, size_t salt_len,
			   uint32_t iter,
			   uint32_t key_len, unsigned char *key)
{
	int ret;
	struct digest* d = digest_alloc("hmac(sha1)");

	ret = pkcs5_pbkdf2_hmac(d, pwd, pwd_len, salt, salt_len, iter,
				 key_len, key);

	digest_free(d);
	return ret;
}
