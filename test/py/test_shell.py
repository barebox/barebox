# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled
import json


def test_barebox_true(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_TRUE")

    _, _, returncode = barebox.run('true')
    assert returncode == 0


def test_barebox_false(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_FALSE")

    _, _, returncode = barebox.run('false')
    assert returncode == 1


def test_barebox_echo_cat(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_ECHO", "CONFIG_CMD_CAT")

    barebox.run_check('rm -f stanza*')

    barebox.run_check('echo -o stanza-1 meow')
    barebox.run_check('echo -o stanza-1 mau')
    barebox.run_check('echo -o stanza-2 meow')
    barebox.run_check('echo -a stanza-2 meow')
    barebox.run_check('cat  -o stanza-3 /dev/null')
    barebox.run_check('echo -a stanza-3 mau')
    barebox.run_check('echo -a stanza-3 ma')

    expected = ["mau", "meow", "meow", "mau", "ma"]

    stdout = barebox.run_check('ls -l stanza-*')

    assert len(stdout) == 3

    assert barebox.run_check('cat stanza-*') == expected

    barebox.run_check('rm -f stanza')

    stdout = barebox.run_check('cat -o stanza stanza-*')
    assert stdout == []
    assert barebox.run_check('cat stanza') == expected

    stdout = barebox.run_check('cat -a stanza stanza-*')
    assert stdout == []
    assert barebox.run_check('cat stanza') == expected + expected


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


def count_dicts_in_command_output(barebox, cmd):
    def count_dicts(obj):
        count = 0
        if isinstance(obj, dict):
            count += 1  # count this dict itself
            for value in obj.values():
                count += count_dicts(value)
        elif isinstance(obj, list):
            for item in obj:
                count += count_dicts(item)
        return count

    stdout = "\n".join(barebox.run_check(cmd))
    return count_dicts(json.loads(stdout))


def test_cmd_iomem(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_IOMEM")

    regions = count_dicts_in_command_output(barebox, 'iomem -j')
    assert regions > 0

    assert count_dicts_in_command_output(barebox, 'iomem -jv') == regions
    if regions > 1:
        assert count_dicts_in_command_output(barebox, 'iomem -jg') > regions
        assert count_dicts_in_command_output(barebox, 'iomem -vjg') > regions
    else:
        assert count_dicts_in_command_output(barebox, 'iomem -jg') >= regions
        assert count_dicts_in_command_output(barebox, 'iomem -vjg') >= regions


def test_cmd_clk(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_CLK")

    regions = count_dicts_in_command_output(barebox, 'clk_dump -j')
    assert regions >= 0

    assert count_dicts_in_command_output(barebox, 'clk_dump -vj') == regions
