#include <common.h>
#include <asm/sections.h>
#include <mach/romapi.h>
#include <mach/atf.h>

static int imx8m_bootrom_load(struct rom_api *rom_api, void *adr, size_t size)
{
	while (size) {
		size_t chunksize = min(size, (size_t)1024);
		int ret;

		ret = rom_api->download_image(adr, 0, chunksize,
					      (uintptr_t)adr ^ chunksize);
		if (ret != ROM_API_OKAY) {
			pr_err("Failed to load piggy data (ret = %x)\n", ret);
			return -EIO;
		}

		adr += chunksize;
		size -= chunksize;
	}

	return 0;
}

/* read piggydata via a bootrom callback and place it behind our copy in SDRAM */
static int imx8m_bootrom_load_image(struct rom_api *rom_api)
{
	return imx8m_bootrom_load(rom_api,
				  (void *)MX8M_ATF_BL33_BASE_ADDR + barebox_pbl_size,
				  __image_end - __piggydata_start);
}

int imx8mp_bootrom_load_image(void)
{
	struct rom_api *rom_api = (void *)0x980;

	OPTIMIZER_HIDE_VAR(rom_api);

	return imx8m_bootrom_load_image(rom_api);
}

int imx8mn_bootrom_load_image(void)
{
	return imx8mp_bootrom_load_image();
}
