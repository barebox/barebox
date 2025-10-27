# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def test_barebox_true(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_TRUE")

    _, _, returncode = barebox.run('true')
    assert returncode == 0


def test_barebox_false(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_FALSE")

    _, _, returncode = barebox.run('false')
    assert returncode == 1


def test_barebox_md5sum(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_MD5SUM", "CONFIG_CMD_ECHO")

    barebox.run_check("echo -o md5 test")
    out = barebox.run_check("md5sum md5")
    assert out == ["d8e8fca2dc0f896fd7cb4cb0031ba249  md5"]


def test_barebox_version(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_VERSION")

    stdout, _, returncode = barebox.run('version')
    assert 'barebox' in stdout[1]
    assert returncode == 0


def test_barebox_no_err(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_DMESG")

    # TODO extend by err once all qemu platforms conform
    stdout, _, _ = barebox.run('dmesg -l crit,alert,emerg')
    assert stdout == []
