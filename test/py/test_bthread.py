# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def stale_spawners(barebox):
    threads = barebox.run_check("bthread -i")
    if len(threads) == 0:
        return False
    return len([t for t in threads if t.startswith('spawner')]) > 0


def test_bthread(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_BTHREAD")

    assert not stale_spawners(barebox)

    _, _, returncode = barebox.run('bthread -vvvv')
    assert returncode == 0

    assert not stale_spawners(barebox)

    switches = int(barebox.run_check("bthread -c")[0].split()[0])
    yields = int(barebox.run_check("bthread -t")[0].split()[0])

    assert switches > 1000
    assert yields > 1000
