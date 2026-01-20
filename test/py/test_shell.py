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


def test_barebox_test_var_exists(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_TEST", "CONFIG_CMD_ECHO")

    # Create a file with variable definitions
    barebox.run_check('echo -o /tmp/testvars "TEST_VAR=value"')
    barebox.run_check('echo -a /tmp/testvars "EMPTY_VAR="')

    # Test with unset variable - should fail
    _, _, returncode = barebox.run('test -v NONEXISTENT_VAR')
    assert returncode == 1

    _, _, returncode = barebox.run('[[ -v NONEXISTENT_VAR ]]')
    assert returncode == 1

    # Test with set variable - should succeed
    _, _, returncode = barebox.run('. /tmp/testvars; test -v TEST_VAR')
    assert returncode == 0

    _, _, returncode = barebox.run('. /tmp/testvars; [[ -v TEST_VAR ]]')
    assert returncode == 0

    # Test with empty but set variable - should succeed
    _, _, returncode = barebox.run('. /tmp/testvars; test -v EMPTY_VAR')
    assert returncode == 0

    _, _, returncode = barebox.run('. /tmp/testvars; [[ -v EMPTY_VAR ]]')
    assert returncode == 0

    # Test negation with !
    _, _, returncode = barebox.run('test ! -v NONEXISTENT_VAR')
    assert returncode == 0

    _, _, returncode = barebox.run('. /tmp/testvars; test ! -v TEST_VAR')
    assert returncode == 1

    # Test in conditional context
    barebox.run_check('. /tmp/testvars; [[ -v TEST_VAR ]] && echo ok')
    barebox.run_check('[[ ! -v NONEXISTENT_VAR ]] && echo ok')

    # Clean up
    barebox.run_check('rm /tmp/testvars')
