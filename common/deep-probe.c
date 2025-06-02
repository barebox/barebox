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
	struct deep_probe_entry *board;

	if (boardstate > DEEP_PROBE_UNKNOWN)
		return boardstate;

	/* determine boardstate */
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

	boardstate = DEEP_PROBE_NOT_SUPPORTED;
	pr_info("not activated\n");
	return false;
}
EXPORT_SYMBOL_GPL(deep_probe_is_supported);
