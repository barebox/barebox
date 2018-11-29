/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */
#define pr_fmt(fmt) "HABv3: " fmt

#include <common.h>
#include <hab.h>
#include <io.h>

struct hab_status {
	u8 value;
	const char *str;
};

static struct hab_status hab_status[] = {
	{ 0x12, "Download code can't be executed, please check the HAB type" },
	{ 0x17, "SCC unexpectedly not in secure state" },
	{ 0x1b, "secureRAM self test failure" },
	{ 0x1d, "secureRAM initialization failure" },
	{ 0x1e, "secureRAM secret key invalid" },
	{ 0x27, "secureRAM secrect key unexpectedly in use" },
	{ 0x2b, "secureRAM internal failure" },
	{ 0x2d, "CSF UID does not match either processor UID or generic UID" },
	{ 0x2e, "CSF TYPE does not match processor TYPE" },
	{ 0x33, "certificate parsing failed or the certificate contained an unsupported key" },
	{ 0x35, "signature verification failed" },
	{ 0x36, "hash verification failed" },
	{ 0x39, "Failure not matching any other description" },
	{ 0x3a, "CSF customer/product code does not match processor customer/product code" },
	{ 0x47, "installation or comparison of SRKs failed" },
	{ 0x4b, "CSF command sequence contains unsupported command identifier" },
	{ 0x4d, "CSF length is unsupported" },
	{ 0x4e, "absence of expected CSF header" },
	{ 0x55, "error during assert verification" },
	{ 0x56, "Download code can't be executed, please check the HAB type" },
	{ 0x63, "DCD invalid" },
	{ 0x66, "write operation to register failed" },
	{ 0x67, "INT_BOOT fuse is blown but BOOT pins are set for external boot" },
	{ 0x69, "CSF missing when HAB TYPE is not HAB-disabled" },
	{ 0x6a, "NANDFC boot buffer load failed" },
	{ 0x6c, "Exception has occured" },
	{ 0x6f, "RAM application pointer is NULL or ERASED_FLASH" },
	{ 0x87, "key indexis either unsupported, or an attempt is made to overwrite the SRK from a CSF command" },
	{ 0x88, "Successful download completion" },
	{ 0x8b, "an attempt is made to read a key from the list of subordinate public keys at a location "
	  "where no key is installed" },
	{ 0x8d, "data specified is out of bounds" },
	{ 0x8e, "algorithm type is either invalid or ortherwise unsupported" },
};

int imx_habv3_get_status(uint32_t status)
{
	int i;

	if (status == 0xf0) {
		pr_info("status OK\n");
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(hab_status); i++) {
		if (hab_status[i].value == status) {
			pr_err("status: 0x%02x: %s\n", status, hab_status[i].str);
			return -EPERM;
		}
	}

	pr_err("unknown status code 0x%02x\n", status);

	return -EPERM;
}

int imx25_hab_get_status(void)
{
	if (!cpu_is_mx25())
		return 0;

	return imx_habv3_get_status(readl(IOMEM(0x780018d4)));
}
postmmu_initcall(imx25_hab_get_status);
