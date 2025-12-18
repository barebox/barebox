# SPDX-License-Identifier: GPL-2.0-only

import re
import pytest


def get_dmesg(shell):
    stdout, _, ret = shell.run("journalctl -k --no-pager --grep efi -o cat")
    assert ret == 0
    return stdout


@pytest.mark.lg_feature(['bootable', 'efi'])
def test_efi_kernel_no_warn(shell):
    stdout, stderr, ret = shell.run("journalctl -k --no-pager --grep efi -o cat -p warning")
    assert stdout == []
    assert stderr == []


@pytest.mark.lg_feature(['bootable', 'efi'])
def test_expected_efi_messages(shell, env):
    dmesg = get_dmesg(shell)

    expected_patterns = [
        r"efi:\s+EFI v2\.8 by barebox",
        r"Remapping and enabling EFI services\.",
        r"efivars:\s+Registered efivars operations",
    ]

    for pattern in expected_patterns:
        assert re.search(pattern, "\n".join(dmesg)), \
               f"Missing expected EFI message: {pattern}"


@pytest.mark.lg_feature(['bootable', 'efi'])
def test_efi_systab(shell, env):
    stdout, stderr, ret = shell.run("cat /sys/firmware/efi/systab")
    assert ret == 0
    assert stderr == []
    assert len(stdout) > 0

    expected_patterns = [
    ]

    if 'smbios' in env.get_target_features():
        expected_patterns.append(r"SMBIOS3=")

    for pattern in expected_patterns:
        assert re.search(pattern, "\n".join(stdout)), \
               f"Missing expected entry in systab : {pattern}"


@pytest.mark.lg_feature(['bootable', 'efi'])
def test_efivars_filesystem_not_empty(shell):
    # Directory must not be empty
    stdout, _, ret = shell.run("ls -1 /sys/firmware/efi/efivars")
    assert ret == 0

    assert len(stdout), "EFI variables directory is empty"
