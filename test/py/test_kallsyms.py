# SPDX-License-Identifier: GPL-2.0-only

import pytest
from .helper import skip_disabled


@pytest.mark.parametrize('name', ['do_kallsyms', 'kallsyms_lookup_name'])
def test_cmd_kallsyms(barebox, barebox_config, name):
    skip_disabled(barebox_config, "CONFIG_CMD_KALLSYMS")

    [addr] = barebox.run_check(f'kallsyms {name}')
    [sym] = barebox.run_check(f'kallsyms {addr}')
    assert sym.startswith(f'{name}+0x0/')


def test_cmd_kallsyms_unknown(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_KALLSYMS")

    _, _, ret = barebox.run('kallsyms no_such_symbol')
    assert ret != 0
