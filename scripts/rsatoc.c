// SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-OpenSSL-exception
/*
 * rsatoc - utility to convert an RSA key to a C struct
 *
 * This tool converts an RSA key given as file or PKCS#11
 * URI to a C struct suitable to compile with barebox.
 */
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

static int dts, standalone;

static int rsa_err(const char *msg)
{
	unsigned long sslErr = ERR_get_error();

	fprintf(stderr, "%s", msg);
	fprintf(stderr, ": %s\n",
		ERR_error_string(sslErr, 0));

	return -1;
}

/**
 * rsa_pem_get_pub_key() - read a public key from a .crt file
 *
 * @keydir:	Directory containins the key
 * @name	Name of key file (will have a .crt extension)
 * @rsap	Returns RSA object, or NULL on failure
 * @return 0 if ok, -ve on error (in which case *rsap will be set to NULL)
 */
static int rsa_pem_get_pub_key(const char *path, RSA **rsap)
{
	EVP_PKEY *key;
	X509 *cert;
	RSA *rsa;
	FILE *f;
	int ret;

	*rsap = NULL;
	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open RSA certificate: '%s': %s\n",
			path, strerror(errno));
		return -EACCES;
	}

	/* Read the certificate */
	cert = NULL;
	if (!PEM_read_X509(f, &cert, NULL, NULL)) {
		rewind(f);
		key = PEM_read_PUBKEY(f, NULL, NULL, NULL);
		if (!key) {
			rsa_err("Couldn't read certificate");
			ret = -EINVAL;
			goto err_cert;
		}
	} else {
		/* Get the public key from the certificate. */
		key = X509_get_pubkey(cert);
		if (!key) {
			rsa_err("Couldn't read public key\n");
			ret = -EINVAL;
			goto err_pubkey;
		}
	}

	/* Convert to a RSA_style key. */
	rsa = EVP_PKEY_get1_RSA(key);
	if (!rsa) {
		rsa_err("Couldn't convert to a RSA style key");
		ret = -EINVAL;
		goto err_rsa;
	}
	fclose(f);
	EVP_PKEY_free(key);
	X509_free(cert);
	*rsap = rsa;

	return 0;

err_rsa:
	EVP_PKEY_free(key);
err_pubkey:
	X509_free(cert);
err_cert:
	fclose(f);
	return ret;
}

/**
 * rsa_engine_get_pub_key() - read a public key from given engine
 *
 * @keydir:	Key prefix
 * @name	Name of key
 * @engine	Engine to use
 * @rsap	Returns RSA object, or NULL on failure
 * @return 0 if ok, -ve on error (in which case *rsap will be set to NULL)
 */
static int rsa_engine_get_pub_key(const char *key_id,
				  ENGINE *engine, RSA **rsap)
{
	EVP_PKEY *key;
	RSA *rsa;
	int ret;

	*rsap = NULL;

	key = ENGINE_load_public_key(engine, key_id, NULL, NULL);
	if (!key)
		return rsa_err("Failure loading public key from engine");

	/* Convert to a RSA_style key. */
	rsa = EVP_PKEY_get1_RSA(key);
	if (!rsa) {
		rsa_err("Couldn't convert to a RSA style key");
		ret = -EINVAL;
		goto err_rsa;
	}

	EVP_PKEY_free(key);
	*rsap = rsa;

	return 0;

err_rsa:
	EVP_PKEY_free(key);
	return ret;
}

/*
 * rsa_get_exponent(): - Get the public exponent from an RSA key
 */
static int rsa_get_exponent(RSA *key, uint64_t *e)
{
	int ret;
	BIGNUM *bn_te;
	const BIGNUM *key_e;
	uint64_t te;

	ret = -EINVAL;
	bn_te = NULL;

	if (!e)
		goto cleanup;

	RSA_get0_key(key, NULL, &key_e, NULL);
	if (BN_num_bits(key_e) > 64)
		goto cleanup;

	*e = BN_get_word(key_e);

	if (BN_num_bits(key_e) < 33) {
		ret = 0;
		goto cleanup;
	}

	bn_te = BN_dup(key_e);
	if (!bn_te)
		goto cleanup;

	if (!BN_rshift(bn_te, bn_te, 32))
		goto cleanup;

	if (!BN_mask_bits(bn_te, 32))
		goto cleanup;

	te = BN_get_word(bn_te);
	te <<= 32;
	*e |= te;
	ret = 0;

cleanup:
	if (bn_te)
		BN_free(bn_te);

	return ret;
}

/*
 * rsa_get_params(): - Get the important parameters of an RSA public key
 */
static int rsa_get_params(RSA *key, uint64_t *exponent, uint32_t *n0_invp,
			  BIGNUM **modulusp, BIGNUM **r_squaredp)
{
	BIGNUM *big1, *big2, *big32, *big2_32;
	BIGNUM *n, *r, *r_squared, *tmp;
	const BIGNUM *key_n;
	BN_CTX *bn_ctx = BN_CTX_new();
	int ret = 0;

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

	if (0 != rsa_get_exponent(key, exponent))
		ret = -1;

	RSA_get0_key(key, &key_n, NULL, NULL);
	if (!BN_copy(n, key_n) || !BN_set_word(big1, 1L) ||
	    !BN_set_word(big2, 2L) || !BN_set_word(big32, 32L))
		ret = -1;

	/* big2_32 = 2^32 */
	if (!BN_exp(big2_32, big2, big32, bn_ctx))
		ret = -1;

	/* Calculate n0_inv = -1 / n[0] mod 2^32 */
	if (!BN_mod_inverse(tmp, n, big2_32, bn_ctx) ||
	    !BN_sub(tmp, big2_32, tmp))
		ret = -1;
	*n0_invp = BN_get_word(tmp);

	/* Calculate R = 2^(# of key bits) */
	if (!BN_set_word(tmp, BN_num_bits(n)) ||
	    !BN_exp(r, big2, tmp, bn_ctx))
		ret = -1;

	/* Calculate r_squared = R^2 mod n */
	if (!BN_copy(r_squared, r) ||
	    !BN_mul(tmp, r_squared, r, bn_ctx) ||
	    !BN_mod(r_squared, tmp, n, bn_ctx))
		ret = -1;

	*modulusp = n;
	*r_squaredp = r_squared;

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

static int rsa_engine_init(ENGINE **pe)
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
			fprintf(stderr, "Cannot set PKCS#11 PIN\n");
			goto err_set_rsa;
		}
	}

	if (!ENGINE_set_default_RSA(e)) {
		fprintf(stderr, "Couldn't set engine as default for RSA\n");
		ret = -1;
		goto err_set_rsa;
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

static FILE *outfilep;

static int print_bignum(BIGNUM *num, int num_bits)
{
	BIGNUM *tmp, *big2, *big32, *big2_32;
	BN_CTX *ctx;
	int i;
	uint32_t *arr;

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
	BN_set_word(big32, 32L);
	BN_exp(big2_32, big2, big32, ctx); /* B = 2^32 */

	arr = malloc(num_bits / 32 * sizeof(uint32_t));

	for (i = 0; i < num_bits / 32; i++) {
		BN_mod(tmp, num, big2_32, ctx); /* n = N mod B */
		arr[i] = BN_get_word(tmp);
		BN_rshift(num, num, 32); /*  N = N/B */
	}

	if (dts) {
		for (i = 0; i < num_bits / 32; i++) {
			if (i % 4)
				fprintf(outfilep, " ");
			else
				fprintf(outfilep, "\n\t\t\t\t");
			fprintf(outfilep, "0x%08x", arr[num_bits / 32 - 1 - i]);
			BN_rshift(num, num, 32); /*  N = N/B */
		}
	} else {
		for (i = 0; i < num_bits / 32; i++) {
			if (i % 4)
				fprintf(outfilep, " ");
			else
				fprintf(outfilep, "\n\t");
			fprintf(outfilep, "0x%08x,", arr[i]);
			BN_rshift(num, num, 32); /*  N = N/B */
		}
	}

	free(arr);
	BN_free(tmp);
	BN_free(big2);
	BN_free(big32);
	BN_free(big2_32);

	return 0;
}

static int gen_key(const char *keyname, const char *path)
{
	BIGNUM *modulus, *r_squared;
	uint64_t exponent = 0;
	uint32_t n0_inv;
	int ret;
	int bits;
	RSA *rsa;
	ENGINE *e = NULL;
	char *tmp, *key_name_c;

	tmp = key_name_c = strdup(keyname);

	while (*tmp) {
		if (*tmp == '-')
			*tmp = '_';
		tmp++;
	}

	if (!strncmp(path, "__ENV__", 7)) {
		const char *var = getenv(path + 7);
		if (!var) {
			fprintf(stderr,
				"environment variable \"%s\" is empty\n", path + 7);
			exit(1);
		}
		path = var;
	}

	if (!strncmp(path, "pkcs11:", 7)) {
		ret = rsa_engine_init(&e);
		if (ret)
			exit(1);
		ret = rsa_engine_get_pub_key(path, e, &rsa);
		if (ret)
			exit(1);
	} else {
		ret = rsa_pem_get_pub_key(path, &rsa);
		if (ret)
			exit(1);
	}

	ret = rsa_get_params(rsa, &exponent, &n0_inv, &modulus, &r_squared);
	if (ret)
		return ret;

	bits = BN_num_bits(modulus);

	if (dts) {
		fprintf(outfilep, "\t\tkey-%s {\n", key_name_c);
		fprintf(outfilep, "\t\t\trsa,r-squared = <");
		print_bignum(r_squared, bits);
		fprintf(outfilep, ">;\n");
		fprintf(outfilep, "\t\t\trsa,modulus= <");
		print_bignum(modulus, bits);
		fprintf(outfilep, ">;\n");
		fprintf(outfilep, "\t\t\trsa,exponent = <0x%0lx 0x%lx>;\n",
			(exponent >> 32) & 0xffffffff,
			exponent & 0xffffffff);
		fprintf(outfilep, "\t\t\trsa,n0-inverse = <0x%0x>;\n", n0_inv);
		fprintf(outfilep, "\t\t\trsa,num-bits = <0x%0x>;\n", bits);
		fprintf(outfilep, "\t\t\tkey-name-hint = \"%s\";\n", key_name_c);
		fprintf(outfilep, "\t\t};\n");
	} else {
		fprintf(outfilep, "\nstatic uint32_t %s_modulus[] = {", key_name_c);
		print_bignum(modulus, bits);
		fprintf(outfilep, "\n};\n\n");

		fprintf(outfilep, "static uint32_t %s_rr[] = {", key_name_c);
		print_bignum(r_squared, bits);
		fprintf(outfilep, "\n};\n\n");

		if (standalone) {
			fprintf(outfilep, "struct rsa_public_key __key_%s;\n", key_name_c);
			fprintf(outfilep, "struct rsa_public_key __key_%s = {\n", key_name_c);
		} else {
			fprintf(outfilep, "static struct rsa_public_key %s = {\n", key_name_c);
		}

		fprintf(outfilep, "\t.len = %d,\n", bits / 32);
		fprintf(outfilep, "\t.n0inv = 0x%0x,\n", n0_inv);
		fprintf(outfilep, "\t.modulus = %s_modulus,\n", key_name_c);
		fprintf(outfilep, "\t.rr = %s_rr,\n", key_name_c);
		fprintf(outfilep, "\t.exponent = 0x%0lx,\n", exponent);
		fprintf(outfilep, "\t.key_name_hint = \"%s\",\n", keyname);
		fprintf(outfilep, "};\n");

		if (!standalone)
			fprintf(outfilep, "\nstruct rsa_public_key *%sp __attribute__((section(\".rsa_keys.rodata.%s\"))) = &%s;\n",
				key_name_c, key_name_c, key_name_c);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *path, *keyname;
	int i, opt;
	char *outfile = NULL;

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
		fprintf(stderr, "Usage: %s [-ods] <key_name_hint>:<crt> ...\n", argv[0]);
		fprintf(stderr, "\t-o FILE\twrite output into FILE instead of stdout\n");
		fprintf(stderr, "\t-d\tgenerate device tree snippet instead of C code\n");
		fprintf(stderr, "\t-s\tgenerate standalone key outside FIT image keyring\n");
		exit(1);
	}

	if (dts) {
		fprintf(outfilep, "/dts-v1/;\n");
		fprintf(outfilep, "/ {\n");
		if (standalone)
			fprintf(outfilep, "\tsignature-standalone {\n");
		else
			fprintf(outfilep, "\tsignature {\n");
	} else if (standalone) {
		fprintf(outfilep, "#include <rsa.h>\n");
	}

	for (i = optind; i < argc; i++) {
		keyname = argv[i];

		path = strchr(keyname, ':');
		if (!path) {
			fprintf(stderr,
				"keys must be given as <key_name_hint>:<crt>\n");
			exit(1);
		}

		*path = 0;
		path++;

		if (!strncmp(path, "__ENV__", 7)) {
			const char *orig_path = path;

			path = getenv(orig_path + 7);
			if (!path) {
				fprintf(stderr, "%s doesn't contain a path\n",
					orig_path + 7);
				exit(1);
			}
		}

		gen_key(keyname, path);
	}

	if (dts) {
		fprintf(outfilep, "\t};\n");
		fprintf(outfilep, "};\n");
	}

	exit(0);
}
