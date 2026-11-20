# SPDX-License-Identifier: GPL-2.0-only

import pytest
from .helper import skip_disabled


@pytest.mark.parametrize('name,option', [
    ('do_kallsyms', 'CONFIG_KALLSYMS'),
    ('kallsyms_lookup_name', 'CONFIG_KALLSYMS'),
    ('barebox_loglevel', 'CONFIG_KALLSYMS_ALL'),
])
def test_cmd_kallsyms(barebox, barebox_config, name, option):
    skip_disabled(barebox_config, "CONFIG_CMD_KALLSYMS", option)

    [addr] = barebox.run_check(f'kallsyms {name}')
    [sym] = barebox.run_check(f'kallsyms {addr}')
    assert sym.startswith(f'{name}+0x0/')


def test_cmd_kallsyms_unknown(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_KALLSYMS")

    _, _, ret = barebox.run('kallsyms no_such_symbol')
    assert ret != 0

    if 'CONFIG_KALLSYMS_ALL' not in barebox_config:
        _, _, ret = barebox.run('kallsyms barebox_loglevel')
        assert ret != 0
