// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2002 Sysgo Real-Time Solutions GmbH (http://www.elinos.com, Marius Groeger <mgroeger@sysgo.de>)
// SPDX-FileCopyrightText: 2001 Erik Mouw <J.A.K.Mouw@its.tudelft.nl>

#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <of.h>
#include <magicvar.h>
#include <zero_page.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>
#include <asm/secure.h>
#include <asm/boot.h>

void start_kernel_optee(void *optee, void *kernel, void *oftree);

void start_linux(void *adr, int swap, unsigned long initrd_address,
		 unsigned long initrd_size, void *oftree,
		 enum arm_security_state state, void *optee)
{
	phys_addr_t params = 0;
	unsigned architecture;
	int ret;

	if (IS_ENABLED(CONFIG_ARM_SECURE_MONITOR) && state > ARM_STATE_SECURE) {
		ret = armv7_secure_monitor_install();
		if (ret)
			pr_err("Failed to install secure monitor\n");
	}

	if (oftree) {
		pr_debug("booting kernel with devicetree\n");
		params = virt_to_phys(oftree);
		architecture = ~0;
	} else if (IS_ENABLED(CONFIG_BOOT_ATAGS)) {
		pr_debug("booting kernel with ATAGS\n");

		params = virt_to_phys(armlinux_get_bootparams());

		if (params < PAGE_SIZE)
			zero_page_access();

		armlinux_setup_tags(initrd_address, initrd_size, swap);
		architecture = armlinux_get_architecture();
	} else {
		static struct tag dummy = {
			.hdr.tag = ATAG_NONE, .hdr.size = 0
		};
		params = virt_to_phys(&dummy);
		architecture = 0;
		pr_debug("booting kernel without external device tree\n");
	}

	shutdown_barebox();

	if (IS_ENABLED(CONFIG_ARM_SECURE_MONITOR) && state == ARM_STATE_HYP)
		armv7_switch_to_hyp();

	if (swap) {
		u32 reg;
		__asm__ __volatile__("mrc p15, 0, %0, c1, c0" : "=r" (reg));
		reg ^= CR_B; /* swap big-endian flag */
		__asm__ __volatile__("mcr p15, 0, %0, c1, c0" :: "r" (reg));
	}

	if (optee && IS_ENABLED(CONFIG_BOOTM_OPTEE)) {
		start_kernel_optee(optee, adr, oftree);
	} else {
		__jump_to_linux(adr, architecture, params);
	}
}
