# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def clk_test_config(env, target, test):
    tests = env.config.get_targets()[target.name].get("clk_tests") or {}
    return tests.get(test) or {}


def test_clk_assigned_check(barebox, barebox_config, env, target):
    skip_disabled(barebox_config, "CONFIG_CMD_CLK_CHECK")

    ignore = clk_test_config(env, target, "assigned_check").get("ignore", [])
    cmd = " ".join(["clk_assigned_check"] + [f"-i {clk}" for clk in ignore])

    stdout, _, returncode = barebox.run(cmd)
    assert returncode == 0, "\n".join(stdout)
