# SPDX-License-Identifier: GPL-2.0-only

import json
import pytest
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


def clk_tree(tree, parent=None):
    for name, clk in tree.items():
        if isinstance(clk, dict):
            yield name, (clk["rate"], parent, clk["enable_count"])
            yield from clk_tree(clk, name)


def test_clk_dump(barebox, barebox_config, env, target):
    config = clk_test_config(env, target, "dump")
    if "reference" not in config:
        pytest.skip("no clk_dump to compare against")
    skip_disabled(barebox_config, "CONFIG_CMD_CLK")

    ignore = config.get("ignore", [])

    with open(config["reference"]) as f:
        expected = dict(clk_tree(json.load(f)))
    stdout = barebox.run_check("clk_dump -j")
    actual = dict(clk_tree(json.loads("".join(stdout))))

    # Unused clocks keep whatever rate they come with
    mismatches = []
    for clk, (rate, parent, enabled) in expected.items():
        if not enabled or clk in ignore:
            continue
        got = actual.get(clk, (None, None))[:2]
        if got != (rate, parent):
            mismatches.append(f"{clk}: {got[0]} Hz from {got[1]}, "
                              f"expected {rate} Hz from {parent}")

    assert not mismatches, "\n".join(mismatches)
