// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>

#include <command.h>
#include <crypto/sha.h>
#include <errno.h>
#include <getopt.h>
#include <libfile.h>
#include <linux/kstrtox.h>

#include <rk_secure_boot.h>

static int rksecure_write_hash_hex(const char *hex, u32 key_size_bits)
{
	int ret;
	u8 digest[SHA256_DIGEST_SIZE];

	if (strlen(hex) != SHA256_DIGEST_SIZE * 2) {
		printf("%s has wrong size: Expected %d characters\n",
		       hex, SHA256_DIGEST_SIZE * 2);
		return -EINVAL;
	}

	ret = hex2bin(digest, hex, SHA256_DIGEST_SIZE);
	if (ret < 0) {
		printf("Failed to parse hash %s\n", hex);
		return -EINVAL;
	}

	return rk_secure_boot_burn_hash(digest, key_size_bits);
}

static int rksecure_write_hash_file(const char *filename, u32 key_size_bits)
{
	int ret;
	size_t size;
	void *digest;

	ret = read_file_2(filename, &size, &digest, SHA256_DIGEST_SIZE + 1);
	if (ret)
		return ret;
	if (size != SHA256_DIGEST_SIZE) {
		printf("%s has wrong size: Expected %d bytes\n",
		       filename, SHA256_DIGEST_SIZE);
		return -EINVAL;
	}

	ret = rk_secure_boot_burn_hash(digest, key_size_bits);

	free(digest);

	return ret;
}

static int rksecure_print_info(void)
{
	struct rk_secure_boot_info info;
	int ret;

	ret = rk_secure_boot_get_info(&info);
	if (ret) {
		printf("Failed to read secure boot info\n");
		return ret;
	}

	printf("Public Root Key hash: %*phN\n"
	       "Secure boot: %s\n"
	       "Simulation: %s\n",
	       (int)sizeof(info.hash), info.hash,
	       info.lockdown ? "enabled" : "disabled",
	       info.simulation ? "enabled" : "disabled");

	return 0;
}

static int do_rksecure(int argc, char *argv[])
{
	int opt;
	int ret;
	char *hashfile = NULL;
	char *hash = NULL;
	int lockdown = 0;
	int info = 0;
	int key_size_bits = 0;

	while ((opt = getopt(argc, argv, "s:x:b:li")) > 0) {
		switch (opt) {
		case 's':
			hashfile = optarg;
			break;
		case 'x':
			hash = optarg;
			break;
		case 'b':
			kstrtouint(optarg, 10, &key_size_bits);
			break;
		case 'l':
			lockdown = 1;
			break;
		case 'i':
			info = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!info && !lockdown && !hashfile && !hash)
		return COMMAND_ERROR_USAGE;

	if (info)
		return rksecure_print_info();

	if (hashfile && hash) {
		printf("-s and -x options may not be given together\n");
		return COMMAND_ERROR_USAGE;
	}

	if (hashfile) {
		ret = rksecure_write_hash_file(hashfile, key_size_bits);
		if (ret)
			return ret;
	} else if (hash) {
		ret = rksecure_write_hash_hex(hash, key_size_bits);
		if (ret)
			return ret;
	}

	if (lockdown) {
		ret = rk_secure_boot_lockdown_device();
		if (ret)
			return ret;
		printf("Device successfully locked down\n");
	}

	return 0;
}

BAREBOX_CMD_HELP_START(rksecure)
BAREBOX_CMD_HELP_TEXT("Manage Rockchip Secure Boot")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_OPT ("-i",		"Print info about secure boot status")
BAREBOX_CMD_HELP_OPT ("-s <file>",	"Burn Super Root Key hash from <file>")
BAREBOX_CMD_HELP_OPT ("-x <sha256>",	"Burn Super Root Key hash from hex string")
BAREBOX_CMD_HELP_OPT ("-b <bits>",	"Bit length of signing key")
BAREBOX_CMD_HELP_OPT ("-l",		"Lockdown device. Dangerous! After executing only signed images can be booted")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(rksecure)
	.cmd = do_rksecure,
	BAREBOX_CMD_DESC("Manage Rockchip Secure Boot")
	BAREBOX_CMD_OPTS("isxbl")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_rksecure_help)
BAREBOX_CMD_END
