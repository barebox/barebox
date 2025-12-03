// SPDX-License-Identifier: GPL-2.0

#include <barebox-info.h>
#include <bbu.h>
#include <blobgen.h>
#include <block.h>
#include <command.h>
#include <complete.h>
#include <dsa.h>
#include <envfs.h>
#include <environment.h>
#include <fb.h>
#include <featctrl.h>
#include <firmware.h>
#include <getopt.h>
#include <globalvar.h>
#include <machine_id.h>
#include <memory.h>
#include <net.h>
#include <pm_domain.h>
#include <sound.h>
#include <stdio.h>
#include <structio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <watchdog.h>

#include <efi/efi-mode.h>
#include <efi/efi-device.h>

#include <generated/utsrelease.h>
#include <linux/clk.h>
#include <linux/device/bus.h>
#include <linux/hw_random.h>
#include <linux/kasan.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/nvmem-consumer.h>
#include <linux/pci.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pstore.h>
#include <linux/reboot-mode.h>
#include <linux/rtc.h>
#include <linux/scmi_protocol.h>
#include <linux/string_choices.h>
#include <linux/tee_drv.h>
#include <linux/usb/usb.h>

#define BFETCH_TMP_ENV	"/tmp/.bfetch.env.tmp"

#define LINE_WIDTH 40
#define WHITE "\033[1;37m"
#define YELLOW "\033[33m"
#define BOLD "\033[1m"
#define RED  "\033[31m"
#define LIGHT_RED  "\033[1;91m"
#define DARK_RED  "\033[0;31m"
#define PURPLE  "\033[35m"
#define RESET "\033[0m"
#define BOLD_RESET RESET BOLD

#define LOGO_WIDTH	44
#define LOGO_HEIGHT	25

static const char logo_lines[LOGO_HEIGHT][LOGO_WIDTH] = {
	":##:                                   :##:",
	"-%%:  =#####.                          :%%-",
	"      #@@@@@.                              ",
	"      *@@@@@.                              ",
	"      *@@@@@.                              ",
	"      *@@@@@.      :=##=.                  ",
	"      *@@@@@.   :+%@@@@@@#+:               ",
	"      *@@@@@:-*@@@@@@@@@@@@@%*-.           ",
	"      *@@@@@@@@@@@@@@%%@@@@@@@@@#=.        ",
	"      *@@@@@@@@@@@%**+=+*%@@@@@@@@@%+.     ",
	"      *@@@@@@@%#*++++++===+*#%@@@@@@@+     ",
	"      *@@@@@#*+++++++++++++===+#@@@@@+     ",
	"      *@@@@@*+++++++++++++++++=*@@@@@+     ",
	"      *@@@@@*+++++++++++++++++=*@@@@@+     ",
	"      *@@@@@*+++++++++++++++++=*@@@@@+     ",
	"      *@@@@@*+++++++++++++++++=*@@@@@+     ",
	"      *@@@@@*++++++++++++++++==*@@@@@+     ",
	"      *@@@@@%#*+++++++++++===+*%@@@@@+     ",
	"      *@@@@@@@@%#*++++===+*#%@@@@@@@@+     ",
	"       -*%@@@@@@@@@%*++*%@@@@@@@@@%+:      ",
	"          :+#@@@@@@@@@@@@@@@@@@#=:         ",
	"             .=*@@@@@@@@@@@%*-.            ",
	"                 -+%@@@@%+:                ",
	"-@@-                :==.               :@@-",
	":**.                                   .**:",
};

static const char style_lines[LOGO_HEIGHT][LOGO_WIDTH] = {
	"*@@*                                   *@@*",
	"*@@*  *******                          *@@*",
	"      *@@@@@*                              ",
	"      *@@@@@*                              ",
	"      *@@@@@*                              ",
	"      *@@@@@*      ******                  ",
	"      *@@@@@*   ***@@@@@@***               ",
	"      *@@@@@***@@@@@@@@@@@@@****           ",
	"      *@@@@@@@@@@@@@@rR@@@@@@@@@***        ",
	"      *@@@@@@@@@@@rrrrRRRR@@@@@@@@@***     ",
	"      *@@@@@@@rrrrrrrrRRRRRRRR@@@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@rrrrrrrrrrRRRRRRRRRR@@@@@*     ",
	"      *@@@@@@@@rrrrrrrRRRRRRR@@@@@@@@*     ",
	"       ***@@@@@@@@@rrrRRR@@@@@@@@@***      ",
	"          ***@@@@@@@@@@@@@@@@@@***         ",
	"             ***@@@@@@@@@@@****            ",
	"                 ***@@@@***                ",
	"*@@*                ****               *@@*",
	"*@@*                                   *@@*",
};

static_assert(sizeof(logo_lines) == sizeof(style_lines));

static bool skip_logo;

static bool logo(unsigned *lineidx)
{
	const char *c, *s;
	char last_style = '\0';

	if (skip_logo)
		return false;

	if (*lineidx >= LOGO_HEIGHT) {
		printf("%*s" "    ", LOGO_WIDTH, "");
		return false;
	}

	c = logo_lines[*lineidx];
	s = style_lines[*lineidx];

	while (*c) {
		if (last_style != *s) {
			switch (*s) {
			case '*': puts(RESET); break;
			case '@': puts(RESET BOLD); break;
			case 'r': puts(LIGHT_RED); break;
			case 'R': puts(DARK_RED); break;
			case ' ': break;
			default:  BUG();
			}

			last_style = *s;
		}

		putchar(*c);

		c++;
		s++;
	}

	printf(RESET " ");
	++*lineidx;
	return true;
}

#define print_line(key, fmt, ...) do { \
	logo(line); printf(DARK_RED "%s" RESET ": " fmt "\n", key, ##__VA_ARGS__); \
} while (0)

static inline bool __print_string_list(unsigned *line, const char *key,
				       struct string_list *sl,
				       const char *joinstr)
{
	char *str;

	if (string_list_empty(sl))
		return false;

	str = string_list_join(sl, joinstr);

	print_line(key, "%s", str);

	free(str);

	return true;
}
#define print_string_list(k, sl, js) __print_string_list(line, k, sl, js)

static inline bool bus_has_devices(const char *name)
{
	struct bus_type *bus;
	struct device *dev;

	bus = get_bus_by_name("virtio");
	if (!bus)
		return false;

	bus_for_each_device(bus, dev)
		return true;

	return false;
}

static void print_header(unsigned *line)
{
	int size = 0;

	if (!IS_ENABLED(CONFIG_GLOBALVAR))
		return;

	logo(line);
	printf(DARK_RED "%s" RESET "@" DARK_RED "%s" RESET "\n",
		      globalvar_get("user"), globalvar_get("hostname"));
	size += strlen(globalvar_get("user") ?: "");
	size++;
	size += strlen(globalvar_get("hostname") ?: "");
	logo(line);

	for (int i = 0; i < size; i++)
		putchar('-');
	putchar('\n');
}

static void print_barebox_info(unsigned *line)
{
	const char *compat = NULL, *model;

	print_line("Kernel", "barebox " UTS_RELEASE);

	model = globalvar_get("model");
	of_property_read_string(of_get_root_node(), "compatible", &compat);
	if (compat)
		print_line("Model", "%s (%s)", model, compat);
	else if (model)
		print_line("Model", "%s", model);

	if (*CONFIG_NAME)
		print_line("Config", "%s %s", CONFIG_ARCH_LINUX_NAME, CONFIG_NAME);
	else
		print_line("Architecture", "%s", CONFIG_ARCH_LINUX_NAME);
}

static struct bobject *print_cpu_mem_info(unsigned *line)
{
	struct bobject *bret = NULL;
	struct memory_bank *mem;
	unsigned long memsize = 0;
	int nbanks = 0;
	int ret;

	/* TODO: show info for other arches, e.g. RISC-V S-Mode/M-Mode */
	if (IS_ENABLED(CONFIG_ARM)) {
		ret = structio_run_command(&bret, "cpuinfo");
		if (!ret) {
			if (IS_ENABLED(CONFIG_ARM64))
				print_line("CPU", "%s at EL%s",
					   bobject_get_param(bret, "core"),
					   bobject_get_param(bret, "exception_level"));
			else if (IS_ENABLED(CONFIG_ARM32))
				print_line("CPU", "%s", bobject_get_param(bret, "core"));
		}
	} else if (IS_ENABLED(CONFIG_RISCV)) {
		ret = structio_run_command(&bret, "cpuinfo");
		if (!ret) {
			print_line("CPU", "%s in %s-Mode",
				   bobject_get_param(bret, "Architecture"),
				   bobject_get_param(bret, "Mode"));
		}
	} else if (IS_ENABLED(CONFIG_GLOBALVAR)) {
		print_line("CPU", "%s-endian", globalvar_get("endianness"));
	}

	for_each_memory_bank(mem) {
		memsize += resource_size(mem->res);
		nbanks++;
	}

	if (nbanks > 1)
		print_line("Memory",      "%s across %u banks",
			   size_human_readable(memsize), nbanks);
	else if (nbanks == 1)
		print_line("Memory",      "%s",
			   size_human_readable(memsize));

	return bret;
}

static void print_shell_console(unsigned *line)
{
	const char *shell;
	struct command *cmd;
	struct string_list sl;
	struct console_device *console;

	if (IS_ENABLED(CONFIG_SHELL_HUSH))
		shell = "Hush";
	else if (IS_ENABLED(CONFIG_SHELL_SIMPLE))
		shell = "Simple";
	else
		shell = "None";

	if (IS_ENABLED(CONFIG_COMMAND_SUPPORT)) {
		int ncmds = 0, naliases = 0;

		for_each_command(cmd) {
			ncmds++;
			naliases += strv_length(cmd->aliases);
		}

		print_line("Shell", "%s with %u commands and %u aliases",
			   shell, ncmds, naliases);
	} else {
		print_line("Shell", "%s", shell);
	}

	string_list_init(&sl);

	for_each_console(console)
		string_list_add(&sl, console->devfs.name);
	if (!string_list_empty(&sl)) {
		char *str = string_list_join(&sl, " ");

		print_line("Consoles", "%s", str);

		free(str);
	}

	string_list_free(&sl);
}

static void print_framebuffers(unsigned *line)
{
	struct string_list sl;
	struct fb_info *info;

	if (!IS_ENABLED(CONFIG_VIDEO))
		return;

	string_list_init(&sl);

	class_for_each_container_of_device(&fb_class, info, dev) {
		string_list_add_asprintf(&sl, "%s: %ux%ux%u",
					 dev_name(&info->dev),
					 info->xres, info->yres,
					 info->bits_per_pixel);
	}

	if (!string_list_empty(&sl)) {
		char *str = string_list_join(&sl, ", ");

		print_line("Framebuffers", "%s", str);

		free(str);
	}

	string_list_free(&sl);
}

static void print_features(unsigned *line)
{
	struct stat st;
	struct string_list sl;
	bool features = false;
	size_t tmp;

	string_list_init(&sl);

	if (blobgen_get(NULL))
		string_list_add(&sl, "BLOBGEN");
	if (clk_have_nonfixed_providers())
		string_list_add(&sl, "CLK");
	if (IS_ENABLED(CONFIG_DSA) && !list_empty(&dsa_switch_list))
		string_list_add(&sl, "DSA");
	if (IS_ENABLED(CONFIG_FEATURE_CONTROLLER) &&
	    !list_empty(&of_feature_controllers))
		string_list_add(&sl, "FEATCTL");
	if (!stat("/dev/fw_cfg", &st))
		string_list_add(&sl, "FW_CFG");
	if (firmwaremgr_find_by_node(NULL))
		string_list_add(&sl, "FWMGR");
	if (genpd_is_active())
		string_list_add(&sl, "GENPD");
	if (hwrng_get_first())
		string_list_add(&sl, "HWRNG");
	if (machine_id_get_hashable(&tmp))
		string_list_add(&sl, "MACHINE-ID");
	if (IS_ENABLED(CONFIG_MCI_TUNING)) {
		string_list_add(&sl, "MCI-TUNING");
		// TODO: check that loaded driver supports it
	}
	if (IS_ENABLED(CONFIG_PCI) && !list_empty(&pci_root_buses))
		string_list_add(&sl, "PCI");
	if (IS_ENABLED(CONFIG_PINCTRL) && !list_empty(&pinctrl_list))
		string_list_add(&sl, "PINCTRL");
	if (pstore_is_ready())
		string_list_add(&sl, "PSTORE");
	if (reboot_mode_get())
		string_list_add(&sl, "REBOOT-MODE");
	if (IS_ENABLED(CONFIG_RTC) && !list_empty(&rtc_class.list))
		string_list_add(&sl, "RTC");
	if (IS_ENABLED(CONFIG_SOUND) && sound_card_get_default())
		string_list_add(&sl, "SOUND");
	if (IS_ENABLED(CONFIG_SDL))
		string_list_add(&sl, "USB");
	if (bus_has_devices("virtio"))
		string_list_add(&sl, "VIRTIO");
	if (watchdog_get_default())
		string_list_add(&sl, "WDOG");

	if (print_string_list("Features", &sl, " "))
		features = true;

	string_list_reinit(&sl);

	if (IS_ENABLED(CONFIG_FS_DEVFS))
		string_list_add(&sl, "/dev");
	if (IS_ENABLED(CONFIG_9P_FS))
		string_list_add(&sl, "9P");
	if (efi_is_loader())
		string_list_add(&sl, "EFI");
	if (bbu_handlers_available())
		string_list_add(&sl, "BBU");
	if (IS_ENABLED(CONFIG_BTHREAD))
		string_list_add(&sl, "BTHREAD");
	if (deep_probe_is_supported())
		string_list_add(&sl, "DEEP");
	if (IS_ENABLED(CONFIG_GCOV))
		string_list_add(&sl, "GCOV");
	if (IS_ENABLED(CONFIG_RISCV_ICACHE))
		string_list_add(&sl, "I$");
	if (IS_ENABLED(CONFIG_JWT))
		string_list_add(&sl, "JWT");
	if (IS_ENABLED(CONFIG_RATP))
		string_list_add(&sl, "RATP");
	if (IS_ENABLED(CONFIG_CRYPTO_RSA))
		string_list_add(&sl, "RSA");
	if (IS_ENABLED(CONFIG_CRYPTO_ECDSA))
		string_list_add(&sl, "ECDSA");
	if (IS_ENABLED(CONFIG_SDL))
		string_list_add(&sl, "SDL");
	if (IS_ENABLED(CONFIG_ARM_MMU_PERMISSIONS))
		string_list_add(&sl, "W^X");

	if (IS_ENABLED(CONFIG_FS_UBOOTVARFS) && !stat("/dev/ubootvar", &st))
		string_list_add(&sl, "UBOOTVARFS");

	/* TODO: detect semihosting */

	print_string_list(features ? " barebox" : "Features", &sl, " ");

	string_list_free(&sl);
}

static void print_netdevs(unsigned *line)
{
	struct eth_device *edev;
	int nif = 0, nifup = 0;

	if (!IS_ENABLED(CONFIG_NET))
		return;

	for_each_netdev(edev) {
		nif++;
		if (edev->active)
			nifup++;
	}

	print_line("Network", "%u interface%s, %u up",
		   nif, str_plural(nif), nifup);
	if (nifup) {
		for_each_netdev(edev) {
			IPaddr_t ipaddr;

			if (!edev->active)
				continue;

			ipaddr = net_get_ip(edev);

			logo(line);
			printf(DARK_RED "  %s" RESET ": %pI4/%u\n",
			       eth_name(edev), &ipaddr,
			       netmask_to_prefix(edev->netmask));
		}
	}
}

static void print_hardening(unsigned *line)
{
	struct string_list sl;

	string_list_init(&sl);

	if (kasan_enabled())
		string_list_add(&sl, IS_ENABLED(CONFIG_KASAN) ? "KASAN" : "ASAN");
	if (IS_ENABLED(CONFIG_UBSAN))
		string_list_add(&sl, "UBSAN");
	if (IS_ENABLED(CONFIG_DMA_API_DEBUG))
		string_list_add(&sl, "DMA-API");

	print_string_list("Debugging", &sl, " ");

	string_list_reinit(&sl);

	if (!IS_ENABLED(CONFIG_INIT_STACK_NONE) &&
	    IS_ENABLED(CONFIG_INIT_ON_ALLOC_DEFAULT_ON) &&
	    IS_ENABLED(CONFIG_INIT_ON_FREE_DEFAULT_ON) &&
	    IS_ENABLED(CONFIG_ZERO_CALL_USED_REGS)) {
		string_list_add(&sl, "init-all");
	} else {
		if (!IS_ENABLED(CONFIG_INIT_STACK_NONE))
			string_list_add(&sl, "init-stack");
		if (IS_ENABLED(CONFIG_INIT_ON_ALLOC_DEFAULT_ON))
			string_list_add(&sl, "init-alloc");
		if (IS_ENABLED(CONFIG_INIT_ON_FREE_DEFAULT_ON))
			string_list_add(&sl, "init-free");
		if (IS_ENABLED(CONFIG_ZERO_CALL_USED_REGS))
			string_list_add(&sl, "init-regs");
	}
	if (IS_ENABLED(CONFIG_FORTIFY_SOURCE))
		string_list_add(&sl, "fortify");
	if (IS_ENABLED(CONFIG_STACK_GUARD_PAGE))
		string_list_add(&sl, "stack-guard");
	if (!IS_ENABLED(CONFIG_STACKPROTECTOR_NONE))
		string_list_add(&sl, "stack-prot");
	if (!IS_ENABLED(CONFIG_PBL_STACKPROTECTOR_NONE))
		string_list_add(&sl, "stack-prot-pbl");

	print_string_list("Hardening", &sl, " ");

	string_list_free(&sl);
}

static void print_devices_drivers(unsigned *line)
{
	struct bus_type *bus;
	struct device *dev;
	int nbusses = 0, ndevs = 0, ndrvs = 0, nbound = 0;

	for_each_device(dev) {
		ndevs++;
		if (dev->driver)
			nbound++;
	}

	print_line("Devices", "%u with %u bound", ndevs, nbound);

	for_each_bus(bus) {
		struct driver *drv;
		bool has_drivers = false;

		bus_for_each_driver(bus, drv) {
			has_drivers = true;
			ndrvs++;
		}

		nbusses++;
	}

	print_line("Drivers", "%u drivers across %u busses", ndrvs, nbusses);
}

static void print_storage(unsigned *line)
{
	const char *other_key;
	unsigned nmtds = 0, nnvmem =0;
	struct block_device *blk;
	unsigned nblkdevs[BLK_TYPE_COUNT] = {};
	unsigned blkdev_sizes[BLK_TYPE_COUNT] = {};
	struct mtd_info *mtd;
	size_t mtd_size = 0, nvmem_size = 0;
	struct string_list sl;
	struct device *dev;

	if (IS_ENABLED(CONFIG_BLOCK)) {
		for_each_block_device(blk) {
			if (blk->type >= BLK_TYPE_COUNT)
				continue;

			nblkdevs[blk->type]++;
			blkdev_sizes[blk->type] += blk->num_blocks * BLOCKSIZE(blk);
		}
	}

	string_list_init(&sl);

	for (int i = 0; i < BLK_TYPE_COUNT; i++) {
		if (!nblkdevs[i])
			continue;

		string_list_add_asprintf(&sl, "%ux %s (%s)", nblkdevs[i],
					 blk_type_str(i),
					 size_human_readable(blkdev_sizes[i]));
	}

	if (print_string_list("Block devices", &sl, ", "))
		other_key = "Other storage";
	else
		other_key = "Storage";

	string_list_reinit(&sl);

	if (IS_ENABLED(CONFIG_MTD)) {
		class_for_each_container_of_device(&mtd_class, mtd, dev) {
			if (mtd->parent)
				continue;

			nmtds++;
			mtd_size += mtd->size;
		}

		if (nmtds)
			string_list_add_asprintf(&sl, "%ux MTD (%s)", nmtds,
						 size_human_readable(mtd_size));
	}

	if (IS_ENABLED(CONFIG_NVMEM)) {
		class_for_each_device(&nvmem_class, dev) {
			nnvmem++;
			nvmem_size += nvmem_device_size(nvmem_from_device(dev));
		}

		if (nnvmem)
			string_list_add_asprintf(&sl, "%ux NVMEM (%s)", nnvmem,
						 size_human_readable(nvmem_size));
	}

	print_string_list(other_key, &sl, ", ");

	string_list_free(&sl);
}

static void print_env(unsigned *line)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ENV_HANDLING))
		return;

	ret = envfs_save(BFETCH_TMP_ENV, NULL, 0);
	if (!ret) {
		struct stat st;

		if (!stat(BFETCH_TMP_ENV, &st))
			print_line("Environment", "%llu bytes", st.st_size);

		unlink(BFETCH_TMP_ENV);
	}
}

static int tee_devinfo(struct bobject **bret)
{
	struct device *dev;

	if (!IS_ENABLED(CONFIG_TEE))
		return -ENOSYS;

	bus_for_each_device(&tee_bus_type, dev)
		return structio_devinfo(bret, dev);

	return -ENODEV;
}

static void print_firmware(unsigned *line, struct bobject *cpuinfo)
{
	const char *psci_version;
	struct bobject *bret;
	bool have_scmi = false, have_tee = false;
	const char *sbi_version = NULL;

	psci_version = getenv("psci.version");

	if (IS_ENABLED(CONFIG_ARM_SCMI_PROTOCOL) && !list_empty(&scmi_list))
		have_scmi = true;
	if (tee_devinfo(&bret) == 0)
		have_tee = true;
	if (IS_ENABLED(CONFIG_RISCV_SBI))
		sbi_version = bobject_get_param(cpuinfo, "SBI version");

	if (!psci_version && !have_scmi && !have_tee && !efi_is_payload() &&
	    !sbi_version)
		return;

	print_line("Firmware", "");

	if (psci_version)
		print_line("  PSCI", "v%s over %s",
			   psci_version, getenv("psci.method"));

	if (have_scmi)
		print_line("  SCMI", "yes");

	if (sbi_version) {
		const char *imp = bobject_get_param(cpuinfo, "SBI implementation name");

		if (!imp)
			imp = bobject_get_param(cpuinfo, "SBI implementation ID");

		if (imp) {
			print_line("  SBI", "%s by %s %s", sbi_version, imp,
				   bobject_get_param(cpuinfo, "SBI implementation version") ?: "");
		} else {
			print_line("  SBI", "%s", sbi_version);
		}
	}

	if (have_tee) {
		const char *name = bobject_get_param(bret, "impl.name");
		if (name)
			print_line("  TEE", "%s rev. %s",
				   name, bobject_get_param(bret, "impl.rev"));
		else
			print_line("  TEE", "impementation ID %s",
				   bobject_get_param(bret, "impl.id"));
		bobject_free(bret);
	}

	if (efi_is_payload()) {
		print_line("  UEFI", "v%s.%s by %s v%s",
			   dev_get_param(&efi_bus.dev, "major"),
			   dev_get_param(&efi_bus.dev, "minor"),
			   dev_get_param(&efi_bus.dev, "fw_vendor"),
			   dev_get_param(&efi_bus.dev, "fw_revision"));
	}

	bobject_free(cpuinfo);
}

static int do_bfetch(int argc, char *argv[])
{
	struct bobject *bret, *cpuinfo;
	unsigned _line, *line = &_line;
	int opt;
	int ret;

	skip_logo = false;
	while((opt = getopt(argc, argv, "n")) > 0) {
		switch(opt) {
		case 'n':
			skip_logo = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	print_header(line);

	print_barebox_info(line);
	cpuinfo = print_cpu_mem_info(line);

	ret = structio_run_command(&bret, "uptime");
	if (!ret) {
		print_line("Uptime", "%s", bobject_get_param(bret, "uptime"));
		bobject_free(bret);
	}

	print_shell_console(line);
	print_framebuffers(line);
	print_features(line);
	print_netdevs(line);

	/* print_line("Compiled by", "TODO"); */

	if (*buildsystem_version_string)
		print_line("Buildsystem Version", "%s", buildsystem_version_string);

	print_hardening(line);
	print_devices_drivers(line);
	print_storage(line);
	print_env(line);
	print_firmware(line, cpuinfo);

	while (logo(line))
		putchar('\n');

	if (!skip_logo)
		printf(RESET "\n\n");

	return 0;
}

BAREBOX_CMD_HELP_START(bfetch)
BAREBOX_CMD_HELP_TEXT("Print system information from barebox point of view")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-n",  "Don't print the ASCII logo")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bfetch)
	.cmd		= do_bfetch,
	BAREBOX_CMD_DESC("Print device info")
	BAREBOX_CMD_OPTS("[-n]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
