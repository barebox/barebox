# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def test_rustlog(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_RUSTLOG")

    assert any('rustlog: hello' in line for line in barebox.run_check('rustlog hello'))

    # labgrid restores global.loglevel around each command, so set it inline
    out = barebox.run_check('global.loglevel=3; rustlog -l 3 err; rustlog -l 6 info')
    assert any('rustlog: err' in line for line in out)
    assert not any('rustlog: info' in line for line in out)
