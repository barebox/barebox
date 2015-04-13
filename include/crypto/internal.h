/*
 * (C) Copyright 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2 only
 */

int digest_generic_verify(struct digest *d, const unsigned char *md);
int digest_generic_digest(struct digest *d, const void *data,
			  unsigned int len, u8 *out);
