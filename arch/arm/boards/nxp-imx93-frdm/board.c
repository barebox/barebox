// SPDX-License-Identifier: GPL-2.0

#include <deep-probe.h>

static const struct of_device_id frdm_imx93_of_match[] = {
        {
                .compatible = "fsl,imx93-11x11-frdm",
        },
        { /* sentinel */ },
};

BAREBOX_DEEP_PROBE_ENABLE(frdm_imx93_of_match);
