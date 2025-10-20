# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def test_bselftest(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_SELFTEST")

    stdout, _, returncode = barebox.run('selftest', timeout=30)
    assert returncode == 0, "selftest failed:\n{}\n".format("\n".join(stdout))
