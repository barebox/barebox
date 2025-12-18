# SPDX-License-Identifier: GPL-2.0-only

import pytest


@pytest.mark.lg_feature(['bootable', 'smbios'])
def test_smbios3_tables_present(shell):
    _, _, ret = shell.run("test -e /sys/firmware/dmi/tables/smbios_entry_point")
    assert ret == 0, "SMBIOS entry point not found"

    [stdout], _, ret = shell.run("stat -c '%s' /sys/firmware/dmi/tables/DMI")
    assert ret == 0

    size = int(stdout)
    assert size > 0, "SMBIOS DMI table is empty"

    [stdout], _, ret = shell.run("dd if=/sys/firmware/dmi/tables/smbios_entry_point bs=1 count=5 2>/dev/null")
    assert ret == 0
    assert stdout == "_SM3_", "SMBIOS entry point is not SMBIOS 3.x"


@pytest.mark.lg_feature(['bootable', 'smbios'])
def test_smbios_contains_barebox(shell):
    """
    Search raw SMBIOS/DMI tables for a barebox vendor string.
    This avoids dmidecode and relies on simple string matching.
    """
    # The DMI table is binary; strings are still ASCII embedded
    stdout, _, ret = shell.run("strings /sys/firmware/dmi/tables/DMI | grep -i barebox")
    assert len(stdout) > 0, "barebox not found in SMBIOS/DMI tables"
