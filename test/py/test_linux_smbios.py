# SPDX-License-Identifier: GPL-2.0-only

import pytest

from .helper import ensure_debian_iso

fetchdir = "/mnt/9p/testfs"


@pytest.fixture(scope="module")
def debian_iso(env, testfs):
    result = ensure_debian_iso(env, testfs)
    if result is None:
        pytest.skip("Debian ISO not found")
    return result


@pytest.mark.lg_feature(['bootable', 'smbios', 'testfs'])
def test_smbios3_tables_present(shell):
    _, _, ret = shell.run("test -e /sys/firmware/dmi/tables/smbios_entry_point")
    assert ret == 0, "SMBIOS entry point not found"

    [stdout], _, ret = shell.run("wc -c </sys/firmware/dmi/tables/DMI")
    assert ret == 0

    size = int(stdout)
    assert size > 0, "SMBIOS DMI table is empty"

    shell.run_check("echo _SM3_ >/tmp/sm3")
    stdout, _, ret = shell.run("cmp --bytes 5 /tmp/sm3 /sys/firmware/dmi/tables/smbios_entry_point")
    assert stdout == []
    assert ret == 0, "SMBIOS entry point is not SMBIOS 3.x"


@pytest.mark.lg_feature(['bootable', 'smbios', 'testfs'])
def test_smbios_contains_barebox(shell):
    """
    Search raw SMBIOS/DMI tables for a barebox vendor string.
    This avoids dmidecode and relies on simple string matching.
    """
    # The DMI table is binary; strings are still ASCII embedded
    stdout, _, ret = shell.run("grep -a barebox /sys/firmware/dmi/tables/DMI")
    assert len(stdout) > 0, "barebox not found in SMBIOS/DMI tables"
