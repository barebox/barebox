// SPDX-License-Identifier: GPL-2.0-only

#include <command.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/printk.h>
#include <linux/sprintf.h>
#include <stringlist.h>
#include <getopt.h>
#include <driver.h>

static struct clk *clk_assigned_get(struct device_node *np,
				    struct of_phandle_args *clkspec)
{
	struct clk *clk;
	char args[32] = "";
	int n = 0;

	clk = of_clk_get_from_provider(clkspec);
	/* onecell providers return NULL for clocks barebox doesn't implement */
	if (!IS_ERR_OR_NULL(clk))
		return clk;

	for (int i = 0; i < clkspec->args_count; i++)
		n += scnprintf(args + n, sizeof(args) - n, " %u", clkspec->args[i]);

	printf("%pOF: clock <&%pOF%s> not found\n", np, clkspec->np, args);
	return NULL;
}

static bool clk_assigned_check_one(struct device_node *np, int index,
				   bool clk_supplier,
				   struct string_list *ignore)
{
	struct of_phandle_args clkspec;
	struct clk *clk, *parent;
	unsigned long rate;
	u32 assigned;
	bool ok = true;

	/* skip empty (null) phandles */
	if (of_parse_phandle_with_args(np, "assigned-clocks", "#clock-cells",
				       index, &clkspec))
		return true;
	if (clkspec.np == np && !clk_supplier)
		return true;

	clk = clk_assigned_get(np, &clkspec);
	if (!clk)
		return false;
	if (string_list_contains(ignore, clk->name))
		return true;

	if (!of_parse_phandle_with_args(np, "assigned-clock-parents",
					"#clock-cells", index, &clkspec) &&
	    (clkspec.np != np || clk_supplier)) {
		parent = clk_assigned_get(np, &clkspec);
		if (!parent) {
			ok = false;
		} else if (clk_get_parent(clk) != parent) {
			printf("%pOF: %pC has parent %pC instead of %pC\n",
			       np, clk, clk_get_parent(clk), parent);
			ok = false;
		}
	}

	if (!of_property_read_u32_index(np, "assigned-clock-rates", index,
					&assigned) && assigned) {
		rate = clk_get_rate(clk);
		if (rate != assigned && rate != clk_round_rate(clk, assigned)) {
			printf("%pOF: %pC runs at %lu Hz instead of %u Hz\n",
			       np, clk, rate, assigned);
			ok = false;
		}
	}

	return ok;
}

static int do_clk_assigned_check(int argc, char *argv[])
{
	struct device_node *np;
	struct string_list ignore;
	int opt, failed = 0;

	string_list_init(&ignore);

	while ((opt = getopt(argc, argv, "i:")) > 0) {
		switch (opt) {
		case 'i':
			string_list_add(&ignore, optarg);
			break;
		default:
			string_list_free(&ignore);
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind != argc) {
		string_list_free(&ignore);
		return COMMAND_ERROR_USAGE;
	}

	for_each_node_with_property(np, "assigned-clocks") {
		bool clk_supplier = of_clk_provider_registered(np);
		int i, nclks;

		/* only providers and probed devices had them applied */
		if (!clk_supplier && !(np->dev && dev_is_probed(np->dev)))
			continue;

		nclks = of_count_phandle_with_args(np, "assigned-clocks",
						   "#clock-cells");
		for (i = 0; i < nclks; i++)
			failed += !clk_assigned_check_one(np, i, clk_supplier,
							  &ignore);
	}

	string_list_free(&ignore);

	if (!failed)
		return COMMAND_SUCCESS;

	printf("assigned clocks not in effect: %d\n", failed);
	return COMMAND_ERROR;
}

BAREBOX_CMD_HELP_START(clk_assigned_check)
BAREBOX_CMD_HELP_TEXT("Check that the assigned-clocks of all clock providers and probed")
BAREBOX_CMD_HELP_TEXT("devices still have their assigned parents and rates.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-i CLK",  "ignore assignments to CLK, may be given multiple times")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clk_assigned_check)
	.cmd		= do_clk_assigned_check,
	BAREBOX_CMD_DESC("check assigned clocks against the device tree")
	BAREBOX_CMD_OPTS("[-i CLK]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_clk_assigned_check_help)
BAREBOX_CMD_END
