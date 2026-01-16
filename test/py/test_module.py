# SPDX-License-Identifier: GPL-2.0-only

import pytest
from .helper import skip_disabled


def test_insmod_driver(barebox, barebox_config, env):
    """Test that insmod works for drivers."""
    skip_disabled(barebox_config, "CONFIG_MODULES", "CONFIG_CMD_LSMOD")

    if 'modules' not in env.get_target_features():
        pytest.skip("modules feature not enabled for this board")

    [stdout], _, ret = barebox.run("lspci")
    assert ret != 0, "Test assumes all PCI drivers are modules"
    assert stdout == "No PCI bus detected"

    # First load the module
    barebox.run_check("insmod /modules/drivers/pci/pci-ecam-generic.ko")

    # Check lsmod output
    stdout = barebox.run_check("lsmod")
    output = "\n".join(stdout)
    assert "pci_ecam_generic" in output, \
        f"Module not found in lsmod output: {output}"

    stdout, _, ret = barebox.run("lspci")
    assert ret == 0, "Loading PCI driver should result in PCI bus detection"
    assert len(stdout) > 1 or stdout[0] != "No PCI bus detected"


def test_insmod_command(barebox, barebox_config, env):
    """Test that insmod works for commands."""
    skip_disabled(barebox_config, "CONFIG_MODULES", "CONFIG_CMD_LSMOD")

    if 'modules' not in env.get_target_features():
        pytest.skip("modules feature not enabled for this board")

    [stdout], _, ret = barebox.run("pm_domain")
    assert ret != 0, "pm_domain shouldn't yet exist"
    assert stdout == "pm_domain: Command not found"

    # First load the module
    barebox.run_check("insmod /modules/commands/pm_domain.ko")

    # Check lsmod output
    stdout = barebox.run_check("lsmod")
    output = "\n".join(stdout)
    assert "pm_domain" in output, \
        f"Module not found in lsmod output: {output}"

    stdout, _, ret = barebox.run("pm_domain")
    assert ret == 0, "pm_domain should exist now"
    assert stdout[0] != "pm_domain: Command not found"
