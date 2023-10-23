// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <rsa.h>
#include <bselftest.h>
#include <crypto/jwt.h>
#include <console.h>

BSELFTEST_GLOBALS();

static const jsmntok_t *check_token(const jsmntok_t *token,
				    const char *claim,
				    const char *payload,
				    jsmntype_t expected_type,
				    const char *expected_value)
{
	total_tests++;

	if (token->type != expected_type) {
		failed_tests++;
		printf("claim %s has type mismatch: got %d, but %d expected\n",
		       claim, token->type, expected_type);
		return NULL;
	}

	total_tests++;

	if (!jsmn_eq(expected_value, payload, token)) {
		failed_tests++;
		printf("claim %s: value has mismatch: got %.*s, but %s expected\n",
		       claim, (int)(token->end - token->start),
		       &payload[token->start], expected_value);
		return NULL;
	}

	return token;
}

static const jsmntok_t *jwt_check_claim(const struct jwt *jwt,
					const char *claim,
					jsmntype_t expected_type,
					const char *expected_value)
{
	const jsmntok_t *token;

	total_tests++;

	token = jwt_get_claim(jwt, claim);
	if (!token) {
		failed_tests++;
		printf("claim %s couldn't be located\n", claim);
		return NULL;
	}

	return check_token(token, claim, jwt_get_payload(jwt),
			   expected_type, expected_value);
}

static const char jwt_rs256[] =
	"  eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
	"eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiYWRtaW4iOnRydWUsImlhdCI6MTUxNjIzOTAyMn0."
	"NHVaYe26MbtOYhSKkoKYdFVomg4i8ZJd8_-RU8VNbftc4TSMb4bXP3l3YlNWACwyXPGf"
	"fz5aXHc6lty1Y2t4SWRqGteragsVdZufDn5BlnJl9pdR_kdVFUsra2rWKEofkZeIC4yW"
	"ytE58sMIihvo9H1ScmmVwBcQP6XETqYd0aSHp1gOa9RdUPDvoXQ5oqygTqVtxaDr6wUF"
	"KrKItgBMzWIdNZ6y7O9E0DhEPTbE9rfBo6KTFsHAZnMg4k68CDp2woYIaXbmYTWcvbzI"
	"uHO7_37GT79XdIwkm95QJ7hYC9RiwrV7mesbY4PAahERJawntho0my942XheVLmGwLMBkQ\n \n";

static void test_jwt(void)
{
	char *jwt_rs256_mangled, *ch;
	struct jwt_key jwt_key;
	struct jwt *jwt;
	extern const struct rsa_public_key __key_jwt_test;
	int old_loglevel;

	jwt_key.alg = JWT_ALG_RS256;
	jwt_key.material.rsa_pub = &__key_jwt_test;
	total_tests++;

	jwt = jwt_decode(jwt_rs256, &jwt_key);
	if (IS_ERR(jwt)) {
		printf("failed to parse jwt\n");
		failed_tests++;
	} else {
		jwt_check_claim(jwt, "sub", JSMN_STRING, "1234567890");
		jwt_check_claim(jwt, "name", JSMN_STRING, "John Doe");
		jwt_check_claim(jwt, "admin", JSMN_PRIMITIVE, "true");
		jwt_check_claim(jwt, "iat", JSMN_PRIMITIVE, "1516239022");

		jwt_free(jwt);
	}

	/*
	 * Following tests intentionally fail and JWT failures are intentionally
	 * noisy, so we decrease logging a bit during their run
	 */

	old_loglevel = barebox_loglevel;
	barebox_loglevel = MSG_CRIT;

	jwt_rs256_mangled = strdup(jwt_rs256);
	ch = &jwt_rs256_mangled[strlen(jwt_rs256_mangled) - 1];
	*ch = *ch == '_' ? '-' : '_';

	total_tests++;

	jwt = jwt_decode(jwt_rs256_mangled, &jwt_key);
	if (!IS_ERR(jwt)) {
		printf("%s:%d expected JWT verification to fail\n", __func__, __LINE__);
		failed_tests++;
		jwt_free(jwt);
	}

	free(jwt_rs256_mangled);

	jwt_rs256_mangled = strdup(jwt_rs256);
	ch = &jwt_rs256_mangled[0];
	*ch = *ch == '_' ? '-' : '_';

	total_tests++;

	jwt = jwt_decode(jwt_rs256_mangled, &jwt_key);
	if (!IS_ERR(jwt)) {
		printf("%s:%d expected JWT verification to fail\n", __func__, __LINE__);
		failed_tests++;
		jwt_free(jwt);
	}

	free(jwt_rs256_mangled);

	total_tests++;

	jwt_key.alg = JWT_ALG_RS384;

	jwt = jwt_decode(jwt_rs256, &jwt_key);
	if (!IS_ERR(jwt)) {
		printf("%s:%d expected JWT verification to fail\n", __func__, __LINE__);
		failed_tests++;
		jwt_free(jwt);
	}

	total_tests++;

	jwt_key.alg = JWT_ALG_NONE;

	jwt = jwt_decode(jwt_rs256, &jwt_key);
	if (!IS_ERR(jwt)) {
		printf("%s:%d expected JWT verification to fail\n", __func__, __LINE__);
		failed_tests++;
		jwt_free(jwt);
	}

	barebox_loglevel = old_loglevel;
}
bselftest(parser, test_jwt);
