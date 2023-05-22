// SPDX-License-Identifier: GPL-2.0-onl

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <mmu.h>
#include <memtest.h>
#include <abort.h>
#include <zero_page.h>
#include <linux/sizes.h>

BSELFTEST_GLOBALS();

#define __expect(ret, cond, fmt, ...) do { \
	bool __cond = (cond); \
	int __ret = (ret); \
	total_tests++; \
	\
	if (!__cond) { \
		failed_tests++; \
		printf("%s:%d error %pe: " fmt "\n", \
		       __func__, __LINE__, ERR_PTR(__ret), ##__VA_ARGS__); \
	} \
} while (0)

#define expect_success(ret, ...) __expect((ret), ((ret) >= 0), __VA_ARGS__)

static void memtest(void __iomem *start, size_t size, const char *desc)
{
	int ret;

	ret = mem_test_bus_integrity((resource_size_t)start,
				     (resource_size_t)start + size - 1, 0);
	expect_success(ret, "%s bus test", desc);

	ret = mem_test_moving_inversions((resource_size_t)start,
					 (resource_size_t)start + size - 1, 0);
	expect_success(ret, "%s moving inverstions test", desc);
}

static inline int __check_mirroring(void __iomem *a, void __iomem *b, bool is_mirror,
				    const char *func, int line)
{
	if ((readl(a) == readl(b)) == (is_mirror))
		return 0;

	printf("%s:%d: mirroring unexpectedly %s: (*%p = 0x%x) %s (*%p = 0x%x)\n", func, line,
	       is_mirror ? "failed" : "succeeded",
	       a, readl(a), is_mirror ? "!=" : "==", b, readl(b));

	mmuinfo(a);
	mmuinfo(b);

	return -EILSEQ;
}

#define check_mirroring(a, b, is_mirror) \
	__check_mirroring((a), (b), (is_mirror), __func__, __LINE__)

static void test_remap(void)
{
	u8 __iomem *buffer = NULL, *mirror = NULL;
	phys_addr_t buffer_phys;
	int i, ret;

	buffer = memalign(SZ_2M, SZ_8M);
	if (WARN_ON(!buffer))
		goto out;

	buffer_phys = virt_to_phys(buffer);

	mirror = memalign(SZ_2M, SZ_8M);
	if (WARN_ON(!mirror))
		goto out;

	pr_debug("allocated buffer = 0x%p, mirror = 0x%p\n", buffer, mirror);

	memtest(buffer, SZ_8M, "cached buffer");
	memtest(mirror, SZ_8M, "cached mirror");

	if (__is_defined(ARCH_HAS_REMAP)) {
		skipped_tests += 10;
		goto out;
	}

	ret = remap_range(buffer, SZ_8M, MAP_UNCACHED);
	memtest(buffer, SZ_8M, "uncached buffer");

	ret = remap_range(mirror, SZ_8M, MAP_UNCACHED);
	memtest(mirror, SZ_8M, "uncached mirror");

	for (i = 0; i < SZ_8M; i += sizeof(u32)) {
		int m = i, b = i;
		writel(0xDEADBEEF, &mirror[m]);
		writel(i, &buffer[b]);
		ret = check_mirroring(&mirror[m], &buffer[b], false);
		if (ret)
			break;
	}

	expect_success(ret, "asserting no mirror before remap");

	ret = arch_remap_range(mirror, buffer_phys, SZ_8M, MAP_UNCACHED);
	expect_success(ret, "remapping with mirroring");

	for (i = 0; i < SZ_8M; i += sizeof(u32)) {
		int m = i, b = i;
		writel(0xDEADBEEF, &mirror[m]);
		writel(i, &buffer[b]);
		ret = check_mirroring(&mirror[m], &buffer[b], true);
		if (ret)
			break;
	}

	expect_success(ret, "asserting mirroring after remap");

	ret = arch_remap_range(mirror, buffer_phys + SZ_4K, SZ_4M, MAP_UNCACHED);
	expect_success(ret, "remapping with mirroring (phys += 4K)");

	for (i = 0; i < SZ_4M; i += sizeof(u32)) {
		int m = i, b = i + SZ_4K;
		writel(0xDEADBEEF, &mirror[m]);
		writel(i, &buffer[b]);
		ret = check_mirroring(&mirror[m], &buffer[b], true);
		if (ret)
			break;
	}

	expect_success(ret, "asserting mirroring after remap (phys += 4K)");

	ret = arch_remap_range(mirror + SZ_4K, buffer_phys, SZ_4M, MAP_UNCACHED);
	expect_success(ret, "remapping with mirroring (virt += 4K)");

	for (i = 0; i < SZ_4M; i += sizeof(u32)) {
		int m = i + SZ_4K, b = i;
		writel(0xDEADBEEF, &mirror[m]);
		writel(i, &buffer[b]);
		ret = check_mirroring(&mirror[m], &buffer[b], true);
		if (ret)
			break;
	}

	expect_success(ret, "asserting mirroring after remap (virt += 4K)");
	return;

	ret = remap_range(buffer, SZ_8M, MAP_DEFAULT);
	expect_success(ret, "remapping buffer with default attrs");
	memtest(buffer, SZ_8M, "newly cached buffer");

	ret = remap_range(mirror, SZ_8M, MAP_DEFAULT);
	expect_success(ret, "remapping mirror with default attrs");
	memtest(mirror, SZ_8M, "newly cached mirror");

	for (i = 0; i < SZ_8M; i += sizeof(u32)) {
		int m = i, b = i;
		writel(0xDEADBEEF, &mirror[m]);
		writel(i, &buffer[b]);
		ret = check_mirroring(&mirror[m], &buffer[b], false);
		if (ret)
			break;
	}

	expect_success(ret, "asserting no mirror after remap restore");
out:
	free(buffer);
	free(mirror);
}

static void test_zero_page(void)
{
	void __iomem *null = NULL;

	total_tests += 3;

	if (!IS_ENABLED(CONFIG_ARCH_HAS_DATA_ABORT_MASK)) {
		pr_info("skipping %s because %s=n\n",
			"CONFIG_ARCH_HAS_DATA_ABORT_MASK", __func__);
		skipped_tests += 3;
		return;
	}

	OPTIMIZER_HIDE_VAR(null);

	/* Check if *NULL traps and data_abort_mask works */

	data_abort_mask();

	(void)readl(null);

	if (!data_abort_unmask()) {
		printf("%s: NULL pointer access did not trap\n", __func__);
		failed_tests++;
	}

	if (!IS_ENABLED(CONFIG_ARCH_HAS_ZERO_PAGE)) {
		pr_info("skipping %s because %s=n\n",
			"CONFIG_ARCH_HAS_ZERO_PAGE", __func__);
		skipped_tests += 2;
		return;
	}

	/* Check if zero_page_access() works */

	data_abort_mask();

	zero_page_access();
	(void)readl(null);
	zero_page_faulting();

	if (data_abort_unmask()) {
		printf("%s: unexpected fault on zero page access\n", __func__);
		failed_tests++;
	}

	/* Check if zero_page_faulting() works */

	data_abort_mask();

	(void)readl(null);

	if (!data_abort_unmask()) {
		printf("%s NULL pointer access did not trap\n", __func__);
		failed_tests++;
	}
}

static void test_mmu(void)
{
	test_zero_page();
	test_remap();
}
bselftest(core, test_mmu);
