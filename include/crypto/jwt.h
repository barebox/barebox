/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __JWT_H_
#define __JWT_H_

#include <linux/types.h>
#include <jsmn.h>

enum jwt_alg {
	JWT_ALG_NONE,
	JWT_ALG_HS256,
	JWT_ALG_HS384,
	JWT_ALG_HS512,
	JWT_ALG_PS256,
	JWT_ALG_PS384,
	JWT_ALG_PS512,
	JWT_ALG_RS256, /* supported */
	JWT_ALG_RS384, /* supported */
	JWT_ALG_RS512, /* supported */
	JWT_ALG_ES256,
	JWT_ALG_ES256K,
	JWT_ALG_ES384,
	JWT_ALG_ES512,
	JWT_ALG_EDDSA,
};

struct jwt_key {
	enum jwt_alg alg;
	union {
		const struct rsa_public_key *rsa_pub;
	} material;
};

struct jwt_part {
	char *content;
	int token_count;
	jsmntok_t *tokens;
};

struct jwt {
	struct jwt_part header;
	struct jwt_part payload;
};

const char *jwt_split(const char *token,
		      const char **payload, const char **signature, const char **end);

struct jwt *jwt_decode(const char *token, const struct jwt_key *key);
void jwt_free(struct jwt *jwt);

const char *jwt_get_payload(const struct jwt *t);

const jsmntok_t *jwt_get_claim(const struct jwt *t, const char *claim);
char *jwt_get_claim_str(const struct jwt *t, const char *claim);

#endif
