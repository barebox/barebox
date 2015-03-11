/*
 * (C) Copyright 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2 only
 */

#ifdef CONFIG_DIGEST_HMAC
int digest_hmac_register(struct digest_algo *algo, unsigned int pad_length);
#else
static inline int digest_hmac_register(struct digest_algo *algo,
				       unsigned int pad_length)
{
	return 0;
}
#endif
