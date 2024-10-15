// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "jwt: " fmt

#include <crypto/jwt.h>
#include <crypto/rsa.h>
#include <errno.h>
#include <linux/printk.h>
#include <base64.h>
#include <jsmn.h>
#include <linux/ctype.h>

#define JP(...)	(const char *[]) { __VA_ARGS__, NULL }

static enum hash_algo digest_algo_by_jwt_alg(enum jwt_alg alg)
{
	switch (alg) {
		case JWT_ALG_RS256:
			return HASH_ALGO_SHA256;
		case JWT_ALG_RS384:
			return HASH_ALGO_SHA384;
		case JWT_ALG_RS512:
			return HASH_ALGO_SHA512;
		default:
			BUG();
	}
}

static u8 *do_hash(const u8 *buf, size_t len, enum hash_algo algo)
{
	struct digest *digest;
	int ret = 0;
	u8 *hash;

	digest = digest_alloc_by_algo(algo);
	if (!digest) {
		pr_err("signature algorithm not supported\n");
		return ERR_PTR(-ENOSYS);
	}

	hash = xzalloc(digest_length(digest));
	ret = digest_digest(digest, buf, len, hash);
	digest_free(digest);

	if (ret) {
		free(hash);
		return ERR_PTR(ret);
	}

	return hash;
}

static int jwt_part_parse(struct jwt_part *part, const char *content, size_t len)
{
	size_t decoded_len;

	part->content = xmalloc(len);
	decoded_len = decode_base64url(part->content, len, content);
	part->content[decoded_len] = '\0';
	part->tokens = jsmn_parse_alloc(part->content, decoded_len, &part->token_count);
	if (!part->tokens) {
		free(part->content);
		return -EILSEQ;
	}

	return 0;
}

static void jwt_part_free(struct jwt_part *part)
{
	free(part->tokens);
	free(part->content);
}

static const char *jwt_alg_names[] = {
	[JWT_ALG_NONE]   = "none",
	[JWT_ALG_HS256]  = "HS256",
	[JWT_ALG_HS384]  = "HS384",
	[JWT_ALG_HS512]  = "HS512",
	[JWT_ALG_PS256]  = "PS256",
	[JWT_ALG_PS384]  = "PS384",
	[JWT_ALG_PS512]  = "PS512",
	[JWT_ALG_RS256]  = "RS256",
	[JWT_ALG_RS384]  = "RS384",
	[JWT_ALG_RS512]  = "RS512",
	[JWT_ALG_ES256]  = "ES256",
	[JWT_ALG_ES256K] = "ES256K",
	[JWT_ALG_ES384]  = "ES384",
	[JWT_ALG_ES512]  = "ES512",
	[JWT_ALG_EDDSA]  = "EDDSA",
};

static bool jwt_header_ok(struct jwt *jwt, enum jwt_alg alg)
{
	struct jwt_part *header = &jwt->header;
	const jsmntok_t *token;

	token = jsmn_locate(JP("typ"), header->content, header->tokens);
	if (!token)
		return false;

	if (!jsmn_strcase_eq("JWT", header->content, token))
		return false;

	if (alg >= ARRAY_SIZE(jwt_alg_names))
		return false;

	token = jsmn_locate(JP("alg"), header->content, header->tokens);
	if (!token)
		return false;

	return jsmn_strcase_eq(jwt_alg_names[alg], header->content, token);
}

void jwt_free(struct jwt *jwt)
{
	jwt_part_free(&jwt->payload);
	jwt_part_free(&jwt->header);
	free(jwt);
}

const char *jwt_split(const char *token,
		      const char **payload, const char **signature, const char **end)
{
	const char *p, *p_end;

	token = skip_spaces(token);

	p = strchr(token, '.');
	if (!p)
		return ERR_PTR(-EINVAL);
	if (payload)
		*payload = ++p;

	p = strchr(p, '.');
	if (!p)
		return ERR_PTR(-EINVAL);
	if (signature)
		*signature = ++p;

	/* seek to first space or '\0' */
	for (p_end = p; *p_end && !isspace(*p_end); p_end++)
		;

	/* ensure the trailing spaces aren't followed by anything */
	if (*skip_spaces(p_end) != '\0')
		return ERR_PTR(-EINVAL);

	*end = p_end;

	return token;
}

struct jwt *jwt_decode(const char *token, const struct jwt_key *key)
{
	const char *alg_name = jwt_alg_names[key->alg];
	enum hash_algo hash_algo;
	const char *payload, *signature, *end;
	u8 *sigbin;
	size_t sig_len, sigbin_len;
	struct jwt *jwt;
	u8 *hash;
	int ret;

	token = jwt_split(token, &payload, &signature, &end);
	if (IS_ERR(token))
		return ERR_CAST(token);

	sig_len = end - signature;

	switch (key->alg) {
	case JWT_ALG_RS256:
	case JWT_ALG_RS384:
	case JWT_ALG_RS512:
		if (sig_len == 0)
			return ERR_PTR(-EILSEQ);

		sigbin = xzalloc(sig_len);
		sigbin_len = decode_base64url(sigbin, sig_len, signature);

		hash_algo = digest_algo_by_jwt_alg(key->alg);
		hash = do_hash(token, signature - token - 1, hash_algo);
		if (IS_ERR(hash)) {
			free(sigbin);
			return ERR_CAST(hash);
		}

		ret = rsa_verify(key->material.rsa_pub, sigbin, sigbin_len, hash,
				 hash_algo);
		free(hash);
		free(sigbin);
		if (ret < 0) {
			pr_debug("%s signature does not match: %pe\n",
				 alg_name, ERR_PTR(ret));
			return ERR_PTR(ret);
		}

		break;
	default:
		return ERR_PTR(-ENOSYS);
	}

	pr_debug("verification for algo %s ok\n", alg_name);

	jwt = xzalloc(sizeof(*jwt));

	ret = jwt_part_parse(&jwt->header, token, payload - token - 1);
	if (ret || !jwt_header_ok(jwt, key->alg)) {
		ret = ret ?: -EINVAL;
		pr_debug("failed to parse header: %pe\n", ERR_PTR(ret));
		goto err;
	}

	ret = jwt_part_parse(&jwt->payload, payload, signature - payload - 1);
	if (ret) {
		pr_debug("failed to parse payload: %pe\n", ERR_PTR(ret));
		goto err;
	}

	return jwt;

err:
	jwt_free(jwt);
	return ERR_PTR(ret);
}

const char *jwt_get_payload(const struct jwt *t)
{
	return t->payload.content;
}

const jsmntok_t *jwt_get_claim(const struct jwt *t, const char *claim)
{
	return jsmn_locate(JP(claim), t->payload.content, t->payload.tokens);
}

char *jwt_get_claim_str(const struct jwt *t, const char *claim)
{
	return jsmn_strdup(JP(claim), t->payload.content, t->payload.tokens);
}
