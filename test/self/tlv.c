// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <crypto/rsa.h>
#include <bselftest.h>
#include <base64.h>
#include <console.h>
#include <tlv/tlv.h>

BSELFTEST_GLOBALS();

static const char cpu_tlv_encoded[] =
"vCiN/gAAAH4AAAAAAAIAGGd1dGVmZWUyLUQwMS1SMDEtVjAxLUM/PwADAAgAAAAAZ+"
"Z4XgAEAAswMDA0MC4wMDAxMAAFAAEAAAYACGJhc2UsUG9FAAcACzAwMDM2LjAwMDEwA"
"AgAGGd1dGVmZWUyLVMwMS1SMDEtVjAxLUM/PwASAAcEGHTioAPA6zhwRw==";

static const char io_tlv_encoded[] =
"3KWocAAAAEQAAAAAAAMACAAAAABn5nheAAUAAQAABgAEYmFzZQAHAAswMDAzNy4wMDA"
"xMAAIABhndXRlZmVlMi1TMDItUjAxLVYwMS1DPz+xJ7bM";

static u8 *base64_decode_alloc(const char *encoded, size_t *len)
{
	size_t enclen = strlen(encoded);
	u8 *buf = xmalloc(enclen);
	*len = decode_base64(buf, enclen, encoded);
	return buf;
}

static void assert_equal(struct device_node *a, struct device_node *b)
{
	int ret;

	ret = of_diff(a, b, -1);
	if (ret == 0)
		return;

	pr_warn("comparison of %s and %s failed: no differences expected, %u found.\n",
		a->full_name, b->full_name, ret);
	of_diff(a, b, 1);
	failed_tests++;
}

static void test_lxa_tlv(void)
{
	extern char __dtb_tlv_start[], __dtb_tlv_end[];
	struct tlv_device *cpu_tlvdev, *io_tlvdev;
	size_t cpu_bloblen, io_bloblen;
	void *cpu_blob = base64_decode_alloc(cpu_tlv_encoded, &cpu_bloblen);
	void *io_blob = base64_decode_alloc(io_tlv_encoded, &io_bloblen);
	struct device_node *expected, *np;

	total_tests = 4;

	expected = of_unflatten_dtb(__dtb_tlv_start,
				    __dtb_tlv_end - __dtb_tlv_start);
	if (WARN_ON(IS_ERR(expected))) {
		skipped_tests = total_tests;
		return;
	}

	cpu_tlvdev = tlv_register_device(cpu_blob, NULL);
	if (IS_ERR(cpu_tlvdev)) {
		free(cpu_blob);
		failed_tests++;
		skipped_tests++;
	}

	io_tlvdev = tlv_register_device(io_blob, NULL);
	if (IS_ERR(io_tlvdev)) {
		free(io_blob);
		failed_tests++;
		skipped_tests++;
	}

	for_each_child_of_node(expected, np) {
		if (!IS_ERR(cpu_tlvdev) && !strcmp(np->full_name, "/tlv0"))
			assert_equal(tlv_of_node(cpu_tlvdev), np);
		if (!IS_ERR(io_tlvdev) && !strcmp(np->full_name, "/tlv1"))
			assert_equal(tlv_of_node(io_tlvdev), np);
	}

	if (!IS_ERR(cpu_tlvdev))
		tlv_free_device(cpu_tlvdev);
	if (!IS_ERR(io_tlvdev))
		tlv_free_device(io_tlvdev);
}
bselftest(parser, test_lxa_tlv);
