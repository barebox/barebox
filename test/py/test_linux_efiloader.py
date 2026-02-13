# SPDX-License-Identifier: GPL-2.0-only
# Boot to Linux via EFI CDROM installer image

import re
import pytest

from .helper import ensure_debian_iso

fetchdir = "/mnt/9p/testfs"


@pytest.fixture(scope="module")
def debian_iso(env, testfs):
    result = ensure_debian_iso(env, testfs)
    if result is None:
        pytest.skip("Debian ISO not found")
    return result


def get_dmesg(shell, grep=None):
    cmd = 'dmesg'
    if grep is not None:
        cmd += f" | grep '{grep}'"
    stdout, _, ret = shell.run(cmd)
    assert ret == 0
    return stdout


@pytest.mark.lg_feature(['bootable', 'efi', 'testfs'])
@pytest.mark.parametrize('efiloader', [False, True])
def test_boot_manual_with_initrd(strategy, barebox, env, efiloader, debian_iso):
    """Test booting Debian kernel directly without GRUB"""

    barebox.run_check(f"global.bootm.efi={'required' if efiloader else 'disabled'}")

    def get_option(strategy, opt):
        config = strategy.target.env.config
        return config.get_target_option(strategy.target.name, opt)

    try:
        kernel_path = get_option(strategy, "bootm.image")
    except KeyError:
        pytest.fail("Feature bootable enabled, but bootm.image option missing.")

    kernel_path = f"{fetchdir}/{kernel_path}"
    [kernel_path] = barebox.run_check(f"ls {kernel_path}")

    try:
        initrd_path = get_option(strategy, "bootm.initrd")
        initrd_path = f"{fetchdir}/{initrd_path}"
        [initrd_path] = barebox.run_check(f"ls {initrd_path}")
        barebox.run_check(f"global.bootm.initrd={initrd_path}")
    except KeyError:
        pass

    barebox.run_check(f"global.bootm.image={kernel_path}")
    # Speed up subsequent runs a bit
    barebox.run_check("global linux.bootargs.noapparmor=apparmor=0")

    # Boot the kernel - it should use EFI stub by default
    with strategy.boot_kernel(bootm=True) as shell:
        shell.run_check("grep -q apparmor=0 /proc/cmdline")

        initrd_freed = any("Freeing initrd memory"
                           in line for line in get_dmesg(shell, 'initrd'))
        assert initrd_freed, "initrd was not loaded or freed"

        # Verify we booted to shell
        dmesg = get_dmesg(shell, 'efi')

        uefi_not_found = re.search("efi: UEFI not found.",
                                   "\n".join(dmesg)) is not None

        if efiloader:
            check_efi_kernel_no_warn(shell)
            check_expected_efi_messages(shell, env)
            check_efi_systab(shell, env)
            check_efivars_filesystem_not_empty(shell)

            assert not uefi_not_found, \
                   "EFI stub was not used despite global.bootm.efi=required"
        else:
            # Verify that EFI was NOT used
            assert uefi_not_found, \
                   "EFI stub was used despite global.bootm.efi=disabled"


def check_efi_kernel_no_warn(shell):
    stdout, stderr, _ = shell.run("dmesg -r | grep '<[0-4]>.*\\<efi\\>'")
    assert stdout == []
    assert stderr == []


def check_expected_efi_messages(shell, env):
    dmesg = get_dmesg(shell, 'efi')

    expected_patterns = [
        r"efi:\s+EFI v2\.8 by barebox",
        r"efivars:\s+Registered efivars operations",
    ]

    for pattern in expected_patterns:
        assert re.search(pattern, "\n".join(dmesg)), \
               f"Missing expected EFI message: {pattern}"


def check_efi_systab(shell, env):
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


def check_efivars_filesystem_not_empty(shell):
    shell.run("mount -t efivarfs efivarfs /sys/firmware/efi/efivars")
    stdout, _, ret = shell.run("ls -1 /sys/firmware/efi/efivars")
    assert ret == 0

    assert len(stdout), "EFI variables directory is empty"
