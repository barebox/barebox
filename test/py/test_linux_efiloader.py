# SPDX-License-Identifier: GPL-2.0-only

import re
import pytest


def get_journalctl(shell, kernel=True, grep=None):
    opts = ''
    if grep is not None:
        opts += f" --grep={grep}"
    if kernel:
        opts += " -k"
    stdout, _, ret = shell.run(f"journalctl --no-pager {opts} -o cat")
    assert ret == 0
    return stdout


@pytest.mark.lg_feature(['bootable', 'efi'])
@pytest.mark.parametrize('efiloader', [False, True])
def test_boot_manual_with_initrd(strategy, barebox, env, efiloader):
    """Test booting Debian kernel directly without GRUB"""

    barebox.run_check(f"global.bootm.efi={'required' if efiloader else 'disabled'}")

    def get_option(strategy, opt):
        config = strategy.target.env.config
        return config.get_target_option(strategy.target.name, opt)

    try:
        root_dev = get_option(strategy, "root_dev")
        kernel_path = get_option(strategy, "bootm.image")
    except KeyError:
        pytest.fail("Feature bootable enabled, but root_dev/bootm.image option missing.")  # noqa

    # Detect block devices
    barebox.run_check("detect -a")
    barebox.run_check(f"ls /mnt/{root_dev}/")

    [kernel_path] = barebox.run_check(f"ls /mnt/{root_dev}/{kernel_path}")

    try:
        initrd_path = get_option(strategy, "bootm.initrd")
        [initrd_path] = barebox.run_check(f"ls /mnt/{root_dev}/{initrd_path}")
        barebox.run_check(f"global.bootm.initrd={initrd_path}")
    except KeyError:
        pass

    barebox.run_check(f"global.bootm.image={kernel_path}")
    barebox.run_check(f"global.bootm.root_dev=/dev/{root_dev}")
    barebox.run_check("global.bootm.appendroot=1")
    # Speed up subsequent runs a bit
    barebox.run_check("global linux.bootargs.noapparmor=apparmor=0")

    # Boot the kernel - it should use EFI stub by default
    with strategy.boot_kernel(bootm=True) as shell:
        shell.run_check("grep -q apparmor=0 /proc/cmdline")

        initrd_freed = any("Freeing initrd memory"
                           in line for line in get_journalctl(shell, 'initrd'))
        assert initrd_freed, "initrd was not loaded or freed"

        # Verify we booted to shell
        dmesg = get_journalctl(shell, 'efi')

        uefi_not_found = re.search("efi: UEFI not found.",
                                   "\n".join(dmesg)) is not None

        if efiloader:
            test_efi_kernel_no_warn(shell)
            test_expected_efi_messages(shell, env)
            test_efi_systab(shell, env)
            test_efivars_filesystem_not_empty(shell)

            assert not uefi_not_found, \
                   "EFI stub was not used despite global.bootm.efi=required"
        else:
            # Verify that EFI was NOT used
            assert uefi_not_found, \
                   "EFI stub was used despite global.bootm.efi=disabled"


@pytest.mark.lg_feature(['bootable', 'efi'])
def test_efi_kernel_no_warn(shell):
    stdout, stderr, ret = shell.run("journalctl -k --no-pager --grep efi -o cat -p warning")
    assert stdout == []
    assert stderr == []


@pytest.mark.lg_feature(['bootable', 'efi'])
def test_expected_efi_messages(shell, env):
    dmesg = get_journalctl(shell, 'efi')

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
