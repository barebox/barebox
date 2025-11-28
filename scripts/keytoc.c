// SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-OpenSSL-exception
/*
 * keytoc - utility to convert a public key to a C struct
 *
 * This tool converts an public key given as file or PKCS#11
 * URI to a C struct suitable to compile with barebox.
 *
 * TODO: Find a better way for reimport_key()
 *
 */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/engine.h>
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#include <openssl/store.h>

#include <stdbool.h>
#include <ctype.h>

struct keyinfo {
	char *name_hint;
	char *keyring;
	char *path;
	char *name_c;
};

static int dts, standalone;

static void enomem_exit(const char *func)
{
	fprintf(stderr, "%s: Out of memory\n", func);
	exit(2);
}

static int openssl_error(const char *fmt, ...)
{
	va_list va;
	unsigned long sslErr = ERR_get_error();

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, ": %s\n",
		ERR_error_string(sslErr, 0));

	return -1;
}

/**
 * pem_get_pub_key() - read a public key from a .crt file
 *
 * @keydir:	Directory containins the key
 * @name	Name of key file (will have a .crt extension)
 * @key		Returns key object, or NULL on failure
 * @return 0 if ok, -ve on error (in which case *rsap will be set to NULL)
 */
static int pem_get_pub_key(const char *path, EVP_PKEY **pkey)
{
	EVP_PKEY *key;
	X509 *cert;
	FILE *f;
	int ret;

	*pkey = NULL;
	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open certificate '%s': %s\n",
			path, strerror(errno));
		return -EACCES;
	}

	/* Read the certificate */
	cert = NULL;
	if (!PEM_read_X509(f, &cert, NULL, NULL)) {
		/* Can't open certificate, maybe it's a pubkey */
		rewind(f);
		key = PEM_read_PUBKEY(f, NULL, NULL, NULL);
		if (!key) {
			openssl_error("Couldn't read certificate/pubkey %s\n", path);
			ret = -EINVAL;
			goto err_cert;
		}
	} else {
		/* Get the public key from the certificate. */
		key = X509_get_pubkey(cert);
		if (!key) {
			openssl_error("Couldn't read public key from certificate\n");
			ret = -EINVAL;
			goto err_pubkey;
		}
	}

	fclose(f);
	X509_free(cert);

	*pkey = key;

	return 0;

err_pubkey:
	X509_free(cert);
err_cert:
	fclose(f);
	return ret;
}

static int engine_init(ENGINE **pe)
{
	ENGINE *e;
	int ret;
	const char *key_pass = getenv("KBUILD_SIGN_PIN");

	ENGINE_load_builtin_engines();

	e = ENGINE_by_id("pkcs11");
	if (!e) {
		fprintf(stderr, "Engine isn't available\n");
		ret = -1;
		goto err_engine_by_id;
	}

	if (!ENGINE_init(e)) {
		fprintf(stderr, "Couldn't initialize engine\n");
		ret = -1;
		goto err_engine_init;
	}

	if (key_pass) {
		if (!ENGINE_ctrl_cmd_string(e, "PIN", key_pass, 0)) {
			ret = -1;
			fprintf(stderr, "Cannot set PKCS#11 PIN\n");
			goto err_set_rsa;
		}
	}

	*pe = e;

	return 0;

err_set_rsa:
	ENGINE_finish(e);
err_engine_init:
	ENGINE_free(e);
err_engine_by_id:
#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	(defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	ENGINE_cleanup();
#endif
	return ret;
}

/**
 * engine_get_pub_key() - read a public key from given engine
 *
 * @keydir:	Key prefix
 * @name	Name of key
 * @key		Returns key object, or NULL on failure
 * @return 0 if ok, -ve on error (in which case *rsap will be set to NULL)
 */
static int engine_get_pub_key(const char *key_id, EVP_PKEY **key)
{
	ENGINE *e;
	int ret;

	ret = engine_init(&e);
	if (ret)
		return ret;

	*key = ENGINE_load_public_key(e, key_id, NULL, NULL);
	if (!*key)
		return openssl_error("Failure loading public key from engine");

	return 0;
}

/*
 * rsa_get_exponent(): - Get the public exponent from an RSA key
 */
static int rsa_get_exponent(EVP_PKEY *key, uint64_t *e)
{
	int ret;
	BIGNUM *bn_te = NULL;
	BIGNUM *key_e = NULL;
	uint64_t te;

	ret = EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_RSA_E, &key_e);
	if (!ret)
		return -EINVAL;

	if (BN_num_bits(key_e) > 64) {
		ret = -EINVAL;
		goto cleanup;
	}

	*e = BN_get_word(key_e);

	if (BN_num_bits(key_e) < 33) {
		ret = 0;
		goto cleanup;
	}

	bn_te = BN_dup(key_e);
	if (!bn_te) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!BN_rshift(bn_te, bn_te, 32)) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!BN_mask_bits(bn_te, 32)) {
		ret = -EINVAL;
		goto cleanup;
	}

	te = BN_get_word(bn_te);
	te <<= 32;
	*e |= te;
	ret = 0;

cleanup:
	if (bn_te)
		BN_free(bn_te);
	BN_free(key_e);

	return ret;
}

/*
 * rsa_get_params(): - Get the important parameters of an RSA public key
 */
static int rsa_get_params(EVP_PKEY *key, uint64_t *exponent, uint32_t *n0_invp,
			  BIGNUM **modulusp, BIGNUM **r_squaredp)
{
	BIGNUM *big1, *big2, *big32, *big2_32;
	BIGNUM *n, *r, *r_squared, *tmp;
	BIGNUM *key_n = NULL;
	BN_CTX *bn_ctx = BN_CTX_new();
	int ret;

	/* Initialize BIGNUMs */
	big1 = BN_new();
	big2 = BN_new();
	big32 = BN_new();
	r = BN_new();
	r_squared = BN_new();
	tmp = BN_new();
	big2_32 = BN_new();
	n = BN_new();
	if (!big1 || !big2 || !big32 || !r || !r_squared || !tmp || !big2_32 ||
	    !n) {
		fprintf(stderr, "Out of memory (bignum)\n");
		return -ENOMEM;
	}

	ret = rsa_get_exponent(key, exponent);
	if (ret)
		goto cleanup;

	ret = EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_RSA_N, &key_n);
	if (!ret)
		return -EINVAL;

	if (!BN_copy(n, key_n) || !BN_set_word(big1, 1L) ||
	    !BN_set_word(big2, 2L) || !BN_set_word(big32, 32L)) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* big2_32 = 2^32 */
	if (!BN_exp(big2_32, big2, big32, bn_ctx)) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Calculate n0_inv = -1 / n[0] mod 2^32 */
	if (!BN_mod_inverse(tmp, n, big2_32, bn_ctx) ||
	    !BN_sub(tmp, big2_32, tmp)) {
		ret = -EINVAL;
		goto cleanup;
	}

	*n0_invp = BN_get_word(tmp);

	/* Calculate R = 2^(# of key bits) */
	if (!BN_set_word(tmp, BN_num_bits(n)) ||
	    !BN_exp(r, big2, tmp, bn_ctx)) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Calculate r_squared = R^2 mod n */
	if (!BN_copy(r_squared, r) ||
	    !BN_mul(tmp, r_squared, r, bn_ctx) ||
	    !BN_mod(r_squared, tmp, n, bn_ctx)) {
		ret = -EINVAL;
		goto cleanup;
	}

	*modulusp = n;
	*r_squaredp = r_squared;

	ret = 0;

cleanup:
	BN_free(big1);
	BN_free(big2);
	BN_free(big32);
	BN_free(r);
	BN_free(tmp);
	BN_free(big2_32);
	if (ret) {
		fprintf(stderr, "Bignum operations failed\n");
		return -ENOMEM;
	}

	return ret;
}

static FILE *outfilep;

static int print_bignum(BIGNUM *num, int num_bits, int width)
{
	BIGNUM *tmp, *big2, *big32, *big2_32;
	BN_CTX *ctx;
	int i;
	uint64_t *arr;

	tmp = BN_new();
	big2 = BN_new();
	big32 = BN_new();
	big2_32 = BN_new();

	/*
	 * Note: This code assumes that all of the above succeed, or all fail.
	 * In practice memory allocations generally do not fail (unless the
	 * process is killed), so it does not seem worth handling each of these
	 * as a separate case. Technicaly this could leak memory on failure,
	 * but a) it won't happen in practice, and b) it doesn't matter as we
	 * will immediately exit with a failure code.
	 */
	if (!tmp || !big2 || !big32 || !big2_32) {
		fprintf(stderr, "Out of memory (bignum)\n");
		return -ENOMEM;
	}
	ctx = BN_CTX_new();
	if (!tmp) {
		fprintf(stderr, "Out of memory (bignum context)\n");
		return -ENOMEM;
	}
	BN_set_word(big2, 2L);
	BN_set_word(big32, width);
	BN_exp(big2_32, big2, big32, ctx); /* B = 2^width */

	arr = malloc(num_bits / width * sizeof(*arr));
	if (!arr)
		enomem_exit(__func__);

	for (i = 0; i < num_bits / width; i++) {
		BN_mod(tmp, num, big2_32, ctx); /* n = N mod B */
		arr[i] = BN_get_word(tmp);
		BN_rshift(num, num, width); /*  N = N/B */
	}

	if (dts) {
		for (i = 0; i < num_bits / width; i++) {
			if (i % 4)
				fprintf(outfilep, " ");
			else
				fprintf(outfilep, "\n\t\t\t\t");
			fprintf(outfilep, "0x%0*jx", width / 4, arr[num_bits / width - 1 - i]);
			BN_rshift(num, num, width); /*  N = N/B */
		}
	} else {
		for (i = 0; i < num_bits / width; i++) {
			if (i % 4)
				fprintf(outfilep, " ");
			else
				fprintf(outfilep, "\n\t");
			fprintf(outfilep, "0x%0*jx,", width / 4, arr[i]);
			BN_rshift(num, num, width); /*  N = N/B */
		}
	}

	free(arr);
	BN_free(tmp);
	BN_free(big2);
	BN_free(big32);
	BN_free(big2_32);

	return 0;
}

/*
 * When imported from a HSM the key doesn't have the EC point parameters,
 * only the pubkey itself exists. Exporting the pubkey and creating a new
 * pkey from it resolves this. This can likely (hopefully) be improved, but
 * I don't have an idea how.
 */
static EVP_PKEY *reimport_key(EVP_PKEY *pkey)
{
	char group[128] = {};
	size_t outlen;
	OSSL_PARAM *params;
	OSSL_PARAM_BLD *param_bld = NULL;
	unsigned char pub[128] = {};
	size_t len;
	EVP_PKEY *pkey_out = NULL;
	EVP_PKEY_CTX *pkey_ctx;
	int ret;

	ret = EVP_PKEY_get_utf8_string_param(pkey, "group", group, sizeof(group), &outlen);
	if (!ret)
		return NULL;

	ret = EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY,
					      pub, sizeof(pub), &len);
	if (ret <= 0)
		return NULL;

	param_bld = OSSL_PARAM_BLD_new();
	if (!param_bld)
		return NULL;

	ret = OSSL_PARAM_BLD_push_utf8_string(param_bld, "group", group, 0);
	if (ret <= 0)
		return NULL;

	ret = OSSL_PARAM_BLD_push_octet_string(param_bld, "pub", pub, len);
	if (ret <= 0)
		return NULL;

	params = OSSL_PARAM_BLD_to_param(param_bld);
	if (!params)
		return NULL;

	pkey_ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pkey, NULL);
	if (!pkey_ctx)
		return NULL;

	ret = EVP_PKEY_fromdata_init(pkey_ctx);
	if (ret <= 0)
		return NULL;

	ret = EVP_PKEY_fromdata(pkey_ctx, &pkey_out, EVP_PKEY_PUBLIC_KEY, params);
	if (ret <= 0)
		return NULL;

	return pkey_out;
}

static int print_hash(EVP_PKEY *key)
{
	int i, ret;
	BIO *mem;
	BUF_MEM *p;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	mem = BIO_new(BIO_s_mem());

	ret = i2d_PUBKEY_bio(mem, key);
	if (ret != 1)
		goto err;

	BIO_get_mem_ptr(mem, &p);

	ret = SHA256_Init(&sha256);
	if (ret != 1)
		goto err;

	ret = SHA256_Update(&sha256, p->data, p->length);
	if (ret != 1)
		goto err;

	ret = SHA256_Final(hash, &sha256);
	if (ret != 1)
		goto err;

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
		fprintf(outfilep, "0x%02x, ", hash[i]);

	fprintf(outfilep, "\n");

	ret = 0;
err:
	BIO_free(mem);

	return ret ? -EINVAL : 0;
}

static int gen_key_ecdsa(EVP_PKEY *key, struct keyinfo *info)
{
	char group[128];
	size_t outlen;
	int ret, bits;
	BIGNUM *key_x = NULL, *key_y = NULL;

	key = reimport_key(key);
	if (!key)
		return -EINVAL;

	ret = EVP_PKEY_get_int_param(key, "bits", &bits);
	if (!ret)
		return -EINVAL;
	ret = EVP_PKEY_get_utf8_string_param(key, "group", group, sizeof(group), &outlen);
	if (!ret)
		return -EINVAL;

	ret = EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_EC_PUB_X, &key_x);
	if (!ret)
		return -EINVAL;

	ret = EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_EC_PUB_Y, &key_y);
	if (!ret)
		return -EINVAL;

	if (dts) {
		fprintf(stderr, "ERROR: generating a dts snippet for ECDSA keys is not yet supported\n");
		return -EOPNOTSUPP;
	} else {
		fprintf(outfilep, "\nstatic unsigned char %s_hash[] = {\n\t", info->name_c);

		ret = print_hash(key);
		if (ret)
			return ret;

		fprintf(outfilep, "\n};\n\n");

		fprintf(outfilep, "\nstatic const uint64_t %s_x[] = {", info->name_c);
		ret = print_bignum(key_x, bits, 64);
		if (ret)
			return ret;

		fprintf(outfilep, "\n};\n\n");

		fprintf(outfilep, "static const uint64_t %s_y[] = {", info->name_c);
		ret = print_bignum(key_y, bits, 64);
		if (ret)
			return ret;

		fprintf(outfilep, "\n};\n\n");

		fprintf(outfilep, "static struct ecdsa_public_key %s = {\n", info->name_c);

		fprintf(outfilep, "\t.curve_name = \"%s\",\n", group);
		fprintf(outfilep, "\t.x = %s_x,\n", info->name_c);
		fprintf(outfilep, "\t.y = %s_y,\n", info->name_c);
		fprintf(outfilep, "};\n");
		if (!standalone) {
			fprintf(outfilep, "\nstatic struct public_key %s_public_key = {\n", info->name_c);
			fprintf(outfilep, "\t.type = PUBLIC_KEY_TYPE_ECDSA,\n");
			fprintf(outfilep, "\t.key_name_hint = \"%s\",\n", info->name_hint);
			fprintf(outfilep, "\t.keyring = \"%s\",\n", info->keyring);
			fprintf(outfilep, "\t.hash = %s_hash,\n", info->name_c);
			fprintf(outfilep, "\t.hashlen = %u,\n", SHA256_DIGEST_LENGTH);
			fprintf(outfilep, "\t.ecdsa = &%s,\n", info->name_c);
			fprintf(outfilep, "};\n");
			fprintf(outfilep, "\n");
			fprintf(outfilep, "const struct public_key *__%s_public_key __ll_elem(.public_keys.rodata.%s) = &%s_public_key;\n", info->name_c, info->name_c, info->name_c);
		}
	}

	return 0;
}

static char *try_resolve_env(char *input)
{
	char *var;

	if (!input || strncmp(input, "__ENV__", 7))
		return input;

	var = getenv(input + 7);
	if (!var || *var == '\0') {
		fprintf(stderr,
			"environment variable \"%s\" is not set or empty\n",
			input + 7);
		return NULL;
	}

	return var;
}

static int gen_key_rsa(EVP_PKEY *key, struct keyinfo *info)
{
	BIGNUM *modulus, *r_squared;
	uint64_t exponent = 0;
	uint32_t n0_inv;
	int bits;
	int ret;

	ret = rsa_get_params(key, &exponent, &n0_inv, &modulus, &r_squared);
	if (ret)
		return ret;

	bits = BN_num_bits(modulus);

	if (dts) {
		fprintf(outfilep, "\t\tkey-%s {\n", info->name_c);
		fprintf(outfilep, "\t\t\trsa,r-squared = <");
		ret = print_bignum(r_squared, bits, 32);
		if (ret)
			return ret;

		fprintf(outfilep, ">;\n");
		fprintf(outfilep, "\t\t\trsa,modulus= <");
		ret = print_bignum(modulus, bits, 32);
		if (ret)
			return ret;

		fprintf(outfilep, ">;\n");
		fprintf(outfilep, "\t\t\trsa,exponent = <0x%0lx 0x%lx>;\n",
			(exponent >> 32) & 0xffffffff,
			exponent & 0xffffffff);
		fprintf(outfilep, "\t\t\trsa,n0-inverse = <0x%0x>;\n", n0_inv);
		fprintf(outfilep, "\t\t\trsa,num-bits = <0x%0x>;\n", bits);
		fprintf(outfilep, "\t\t\tkey-name-hint = \"%s\";\n", info->name_c);
		fprintf(outfilep, "\t\t};\n");
	} else {
		fprintf(outfilep, "\nstatic unsigned char %s_hash[] = {\n\t", info->name_c);

		ret = print_hash(key);
		if (ret)
			return ret;

		fprintf(outfilep, "\n};\n\n");

		fprintf(outfilep, "\nstatic const uint32_t %s_modulus[] = {", info->name_c);
		ret = print_bignum(modulus, bits, 32);
		if (ret)
			return ret;

		fprintf(outfilep, "\n};\n\n");

		fprintf(outfilep, "static const uint32_t %s_rr[] = {", info->name_c);
		ret = print_bignum(r_squared, bits, 32);
		if (ret)
			return ret;

		fprintf(outfilep, "\n};\n\n");

		if (standalone) {
			fprintf(outfilep, "struct rsa_public_key __key_%s;\n", info->name_c);
			fprintf(outfilep, "struct rsa_public_key __key_%s = {\n", info->name_c);
		} else {
			fprintf(outfilep, "static struct rsa_public_key %s = {\n", info->name_c);
		}

		fprintf(outfilep, "\t.len = %d,\n", bits / 32);
		fprintf(outfilep, "\t.n0inv = 0x%0x,\n", n0_inv);
		fprintf(outfilep, "\t.modulus = %s_modulus,\n", info->name_c);
		fprintf(outfilep, "\t.rr = %s_rr,\n", info->name_c);
		fprintf(outfilep, "\t.exponent = 0x%0lx,\n", exponent);
		fprintf(outfilep, "};\n");

		if (!standalone) {
			fprintf(outfilep, "\nstatic struct public_key %s_public_key = {\n", info->name_c);
			fprintf(outfilep, "\t.type = PUBLIC_KEY_TYPE_RSA,\n");
			fprintf(outfilep, "\t.key_name_hint = \"%s\",\n", info->name_hint);
			fprintf(outfilep, "\t.keyring = \"%s\",\n", info->keyring);
			fprintf(outfilep, "\t.hash = %s_hash,\n", info->name_c);
			fprintf(outfilep, "\t.hashlen = %u,\n", SHA256_DIGEST_LENGTH);
			fprintf(outfilep, "\t.rsa = &%s,\n", info->name_c);
			fprintf(outfilep, "};\n");
			fprintf(outfilep, "\n");
			fprintf(outfilep, "const struct public_key *__%s_public_key __ll_elem(.public_keys.rodata.%s) = &%s_public_key;\n", info->name_c, info->name_c, info->name_c);
		}
	}

	return 0;
}

static int gen_key(struct keyinfo *info)
{
	int ret;
	EVP_PKEY *key;

	if (!info->path)
		exit(1);

	if (!strncmp(info->path, "pkcs11:", 7)) {
		ret = engine_get_pub_key(info->path, &key);
		if (ret)
			exit(1);
	} else {
		ret = pem_get_pub_key(info->path, &key);
		if (ret)
			exit(1);
	}

	/* generate built-in keys */
	ret = gen_key_ecdsa(key, info);
	if (ret == -EOPNOTSUPP)
		return ret;

	if (ret)
		ret = gen_key_rsa(key, info);

	return ret;
}

static bool is_identifier(char **s)
{
	char *p = *s;

	/* [a-zA-Z] */
	if (!isalpha(*p))
		return false;
	p++;

	/* [a-zA-Z0-9_-]* */
	while (isalnum(*p) || *p == '_' || *p == '-')
		p++;

	*s = p;
	return true;
}

static bool parse_info(char *p, struct keyinfo *out)
{
	char *k = p;
	char *v;

	if (*p == '\0') /* spec starts with colon. This is valid. */
		return true;

	if (!is_identifier(&p))
		return false;

	if (*p == '\0') {
		out->name_hint = strdup(k);
		if (!k)
			enomem_exit(__func__);
		return true; /* legacy syntax */
	}

	/* new syntax: `<k>=<v>[,k=v[...]]` */
	for (;;) {
		if (*p != '=')
			return false;
		*p = '\0';

		p++;
		/* read `<k>=`, excepting <v> */
		v = p;
		if (!is_identifier(&p))
			return false;

		if (*p == '\0' || *p == ',') {
			char d = *p;
			*p = '\0';
			v = strdup(v);
			if (!v)
				enomem_exit(__func__);
			if (strcmp(k, "keyring") == 0)
				out->keyring = strdup(v);
			else if (strcmp(k, "fit-hint") == 0)
				out->name_hint = strdup(v);
			else
				return false;

			if (d == '\0')
				return true;
		} else {
			return false;
		}
		p++;
		k = p;
		if (!is_identifier(&p))
			return true;
	}
}

static bool get_name_path(const char *keyspec, struct keyinfo *out)
{
	char *sep, *spec;

	spec = strdup(keyspec);
	if (!spec)
		enomem_exit(__func__);

	sep = strchr(spec, ':');
	if (!sep) {
		out->path = spec;
		return true;
	}

	*sep = 0;
	if (!parse_info(spec, out)) {
		free(spec);
		return false;
	}
	sep++;
	out->path = strdup(sep);
	if (!out->path)
		enomem_exit(__func__);

	free(spec);
	return true;
}

int main(int argc, char *argv[])
{
	int i, opt, ret;
	char *outfile = NULL;
	int keycount;
	struct keyinfo *keylist;

	outfilep = stdout;

	while ((opt = getopt(argc, argv, "o:ds")) > 0) {
		switch (opt) {
		case 'o':
			outfile = optarg;
			break;
		case 'd':
			dts = 1;
			break;
		case 's':
			standalone = 1;
			break;
		}
	}

	if (outfile) {
		outfilep = fopen(outfile, "w");
		if (!outfilep) {
			fprintf(stderr, "cannot open %s: %s\n", outfile,
				strerror(errno));
			exit(1);
		}
	}

	if (optind == argc) {
		fprintf(stderr, "Usage: %s [-ods] keyring=<keyring>[,fit-hint=<hint>]:<crt> ...\n", argv[0]);
		fprintf(stderr, "\t-o FILE\twrite output into FILE instead of stdout\n");
		fprintf(stderr, "\t-d\tgenerate device tree snippet instead of C code\n");
		fprintf(stderr, "\t-s\tgenerate standalone key outside FIT image keyring\n");
		exit(1);
	}

	keycount = argc - optind;
	keylist = calloc(sizeof(struct keyinfo), keycount);

	for (i = 0; i < keycount; i++) {
		const char *keyspec = try_resolve_env(argv[optind + i]);
		struct keyinfo *info = &keylist[i];

		if (!keyspec)
			exit(1);

		if (!strncmp(keyspec, "pkcs11:", 7)) { // legacy format of pkcs11 URI
			info->path = strdup(keyspec);
		} else {
			if (!get_name_path(keyspec, info)) {
				fprintf(stderr, "invalid keyspec %i: %s\n", optind, keyspec);
				exit(1);
			}
		}
	}

	if (dts) {
		fprintf(outfilep, "/dts-v1/;\n");
		fprintf(outfilep, "/ {\n");
		if (standalone)
			fprintf(outfilep, "\tsignature-standalone {\n");
		else
			fprintf(outfilep, "\tsignature {\n");
	} else if (standalone) {
		fprintf(outfilep, "#include <crypto/ecdsa.h>\n");
		fprintf(outfilep, "#include <crypto/rsa.h>\n");
	}


	for (i = 0; i < keycount; i++) {
		struct keyinfo *info = &keylist[i];

		/* resolve __ENV__ for name_hint and path */
		info->name_hint = try_resolve_env(info->name_hint);
		info->path = try_resolve_env(info->path);

		if (asprintf(&info->name_c, "key_%i", i + 1) < 0)
			enomem_exit("asprintf");

		/* unfortunately, the fit name hint is mandatory in the barebox codebase */
		if (!info->name_hint)
			info->name_hint = info->name_c;

		if (!info->keyring) {
			info->keyring = strdup("fit");
			fprintf(stderr, "Warning: No keyring provided in keyspec, defaulting to keyring=fit for %s\n", argv[optind + i]);
		}

		ret = gen_key(info);
		if (ret)
			exit(1);
	}

	if (dts) {
		fprintf(outfilep, "\t};\n");
		fprintf(outfilep, "};\n");
	}

	/* all keyinfo fields freed on exit */
	exit(0);
}
