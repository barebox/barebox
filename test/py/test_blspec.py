# SPDX-License-Identifier: GPL-2.0-or-later

import os
import shutil
import subprocess
import pytest
from pathlib import Path
from .helper import of_get_property, skip_disabled


OVERLAY = """/dts-v1/;
/plugin/;

&{{/chosen}} {{
	barebox,blspec-overlay{n} = <0x1>;
	barebox,blspec-overlay-last = <{n}>;
}};
"""

ENTRY = """title blspec overlay test
version 1
linux /barebox-dt-2nd.img
devicetree-overlay /overlay1.dtbo /overlay2.dtbo
devicetree-overlay /overlay3.dtbo
"""


@pytest.fixture(scope="module")
def blspec_entry(barebox_config, testfs):
    skip_disabled(barebox_config, "CONFIG_BLSPEC", "CONFIG_OF_OVERLAY")

    builddir = Path(os.environ['LG_BUILDDIR'])
    rootdir = Path(testfs) / "blspec"
    entries = rootdir / "loader" / "entries"
    entries.mkdir(parents=True, exist_ok=True)

    dtc = builddir / "scripts" / "dtc" / "dtc"
    try:
        shutil.copy(builddir / "images" / "barebox-dt-2nd.img", rootdir)

        for n in range(1, 4):
            subprocess.run([dtc if dtc.exists() else "dtc",
                            "-I", "dts", "-O", "dtb",
                            "-o", str(rootdir / f"overlay{n}.dtbo")],
                           input=OVERLAY.format(n=n).encode(), check=True)
    except FileNotFoundError as e:
        pytest.skip(f"Skip blspec tests due to missing dependency: {e}")

    (entries / "overlays.conf").write_text(ENTRY)

    return "/mnt/9p/testfs/blspec/loader/entries/overlays.conf"


def test_blspec_overlays(barebox, strategy, blspec_entry):
    _, _, returncode = barebox.run(f"ls {blspec_entry}")
    if returncode != 0:
        pytest.xfail("skipping test due to missing blspec entry")

    with strategy.boot_barebox(blspec_entry) as barebox:
        for n in range(1, 4):
            assert of_get_property(barebox, f"/chosen/barebox,blspec-overlay{n}") == 0x1, \
                   f"overlay{n}.dtbo listed in the blspec entry was not applied"

        assert of_get_property(barebox, "/chosen/barebox,blspec-overlay-last") == 3, \
               "overlays were not applied in the order they are listed"
