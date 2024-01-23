#define pr_fmt(fmt)     "imx9: " fmt

#include <init.h>
#include <common.h>
#include <linux/clk.h>
#include <mach/imx/generic.h>
#include <mach/imx/ele.h>
#include <linux/bitfield.h>
#include <mach/imx/imx9-regs.h>
#include <tee/optee.h>
#include <asm-generic/memory_layout.h>
#include <asm/optee.h>
#include <mach/imx/scratch.h>

#define SPEED_GRADING_MASK GENMASK(11, 6)
#define MARKETING_GRADING_MASK     GENMASK(5, 4)

static u32 imx9_read_shadow_fuse(int fuse)
{
	void *ocotp = IOMEM(MX9_OCOTP_BASE_ADDR);

	return readl(ocotp + 0x8000 + (fuse << 2));
}

static u32 imx9_cpu_speed_grade_hz(void)
{
	u32 speed, max_speed;
	u32 val;

	val = imx9_read_shadow_fuse(19);

	val = FIELD_GET(SPEED_GRADING_MASK, val) & 0xf;

	speed = 2300000000 - val * 100000000;

	if (cpu_is_mx93())
		max_speed = 1700000000;

	/* In case the fuse of speed grade not programmed */
	if (speed > max_speed)
		speed = max_speed;

	return speed;
}

static void imx93_set_arm_clock(void)
{
	struct clk *pll = clk_lookup("arm_pll");
	struct clk *sel = clk_lookup("a55_sel");
	struct clk *alt = clk_lookup("a55_alt");
	u32 speed;

	if (IS_ERR(pll) || IS_ERR(sel) || IS_ERR(alt)) {
		pr_err("Failed to get clocks\n");
		return;
	}

	speed = imx9_cpu_speed_grade_hz();
	if (!speed)
		return;

	pr_debug("Setting CPU clock to %dMHz\n", speed / 1000000);

	clk_set_parent(sel, alt);
	clk_set_rate(pll, speed);
	clk_set_parent(sel, pll);
}

static const bool imx93_have_npu(void)
{
	u32 val = imx9_read_shadow_fuse(19);

	if (val & BIT(13))
		return false;
	else
		return true;
}

static const int imx93_ncores(void)
{
	u32 val = imx9_read_shadow_fuse(19);

	if (val & BIT(15))
		return 1;
	else
		return 2;
}

static const int imx93_is_9x9(void)
{
	u32 val = imx9_read_shadow_fuse(20);
	u32 pack_9x9_fused = BIT(4) | BIT(17) | BIT(19) | BIT(24);

	if ((val & pack_9x9_fused) == pack_9x9_fused)
		return true;
	else
		return false;
}

#define TEMP_COMMERCIAL         0
#define TEMP_EXTCOMMERCIAL      1
#define TEMP_INDUSTRIAL         2
#define TEMP_AUTOMOTIVE         3

static void imx93_cpu_temp_grade(int *minc, int *maxc, char *code)
{
	u32 val = imx9_read_shadow_fuse(19);
	int min, max;
	char c;

	switch (FIELD_GET(MARKETING_GRADING_MASK, val)) {
	case TEMP_AUTOMOTIVE:
		min = -40;
		max = 125;
		c = 'A';
		break;
	case TEMP_INDUSTRIAL:
		min = -40;
		max = 105;
		c = 'C';
		break;
	case TEMP_EXTCOMMERCIAL:
		if (cpu_is_mx93()) {
			/* imx93 only has extended industrial*/
			min = -40;
			max = 125;
			c = 'X';
		} else {
			min = -20;
			max = 105;
			c = '-';
		}
		break;
	case TEMP_COMMERCIAL:
		min = 0;
		max = 95;
		c = 'D';
		break;
	}

	if (minc)
		*minc = min;
	if (maxc)
		*maxc = max;
	if (code)
		*code = c;
}

static void imx93_type(void)
{
	int subfamily, min, max;
	char code;

	if (imx93_is_9x9()) {
		if (imx93_have_npu())
			subfamily = 2;
		else
			subfamily = 1;
	} else {
		if (imx93_have_npu())
			subfamily = 5;
		else
			subfamily = 3;
	}

	imx93_cpu_temp_grade(&min, &max, &code);

	pr_info("Detected IMX93%d%d%c (%d - %dC)\n", subfamily, imx93_ncores(), code, min, max);
}

int imx93_init(void)
{
	imx93_type();
	imx93_set_arm_clock();

	if (IS_ENABLED(CONFIG_PBL_OPTEE)) {
		static struct of_optee_fixup_data optee_fixup_data = {
			.shm_size = OPTEE_SHM_SIZE,
			.method = "smc",
		};

		optee_set_membase(imx_scratch_get_optee_hdr());
		of_optee_fixup(of_get_root_node(), &optee_fixup_data);
		of_register_fixup(of_optee_fixup, &optee_fixup_data);
	}

	return 0;
}
