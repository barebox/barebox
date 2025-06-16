// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "deep-probe: " fmt

#if defined(CONFIG_DEBUG_INITCALLS) || defined(CONFIG_DEBUG_PROBES)
#define DEBUG
#endif

#include <common.h>
#include <deep-probe.h>
#include <of.h>

enum deep_probe_state {
	DEEP_PROBE_UNKNOWN = -1,
	DEEP_PROBE_NOT_SUPPORTED,
	DEEP_PROBE_SUPPORTED
};

static enum deep_probe_state boardstate = DEEP_PROBE_UNKNOWN;

bool deep_probe_is_supported(void)
{
	bool deep_probe_default = IS_ENABLED(CONFIG_DEEP_PROBE_DEFAULT);
	struct deep_probe_entry *board;
	struct device_node *root;

	if (boardstate > DEEP_PROBE_UNKNOWN)
		return boardstate;

	/*
	 * Deep probe requires resources to be described in DT.
	 * We intentionally omit setting boardstate here on the
	 * off-chance that this function is called too early.
	 */
	root = of_get_root_node();
	if (!root)
		return false;

	/* determine boardstate according to BAREBOX_DEEP_PROBE_ENABLE */
	for (board = __barebox_deep_probe_start;
	     board != __barebox_deep_probe_end; board++) {
		const struct of_device_id *matches = board->device_id;

		for (; matches->compatible; matches++) {
			if (of_machine_is_compatible(matches->compatible)) {
				boardstate = DEEP_PROBE_SUPPORTED;
				pr_debug("supported due to %s\n", matches->compatible);
				return true;
			}
		}
	}

	/*
	 * Deep probe used to be opt-in. As we want all new board support
	 * to make use of it, we add a config option to make it opt-out and
	 * start emitting a warning for boards that are undecided:
	 *
	 * - Boards that don't support deep probe should explicitly opt-out
	 *   using barebox,disable-deep-probe in their device tree.
	 *
	 * - Boards that support deep probe can indicate so using
	 *   barebox,deep-probe if there is no BAREBOX_DEEP_PROBE_ENABLE
	 *   already.
	 *
	 * - New boards without board code will default to deep-probe enabled,
	 *   because CONFIG_DEEP_PROBE_DEFAULT=y would be set in the defconfigs.
	 *
	 * Refer to barebox,deep-probe.rst for more information.
	 */
	if (of_property_read_bool(root, "barebox,disable-deep-probe")) {
		boardstate = DEEP_PROBE_NOT_SUPPORTED;
		pr_info("disabled in device tree\n");
	} else if (of_property_read_bool(root, "barebox,deep-probe")) {
		boardstate = DEEP_PROBE_SUPPORTED;
		pr_debug("enabled in device tree\n");
	} else if (deep_probe_default) {
		boardstate = DEEP_PROBE_SUPPORTED;
		pr_debug("activated by default\n");
	} else {
		boardstate = DEEP_PROBE_NOT_SUPPORTED;
		pr_warn("DT missing barebox,deep-probe or barebox,disable-deep-probe property\n");
		pr_info("not activated by default\n");
	}

	return boardstate;
}
EXPORT_SYMBOL_GPL(deep_probe_is_supported);
