# SPDX-License-Identifier: GPL-2.0-or-later

import os
import shutil
import subprocess
import pytest
from .helper import skip_disabled, filter_errors

TESTNODE = "/barebox-fit-overlay-test"

# name: (compatible, value of the order property)
OVERLAYS = {
    "a": (None, "a"),
    "b": ("barebox,fit-overlay-test-b", "b"),
    "c": ("barebox,fit-overlay-test-c", "c"),
    "kernel": (None, "kernel"),
}

ITS = """/dts-v1/;

/ {
	description = "global.of.overlay.path test";
	#address-cells = <1>;

	images {
		kernel {
			data = /incbin/("kernel.bin");
			type = "kernel";
			arch = "arm64";
			os = "linux";
			compression = "none";
			hash-1 {
				algo = "sha256";
			};
		};
%s	};

	configurations {
		/* has a kernel, so it is not an overlay configuration */
		conf-kernel {
			kernel = "kernel";
			fdt = "overlay-kernel";
		};
%s	};
};
"""

ITS_OVERLAY = """		overlay-%s {
			data = /incbin/("%s.dtbo");
			type = "flat_dt";
			arch = "arm64";
			compression = "none";
			hash-1 {
				algo = "sha256";
			};
		};
"""

ITS_CONFIG = """		conf-%s {
			fdt = "overlay-%s";
		};
"""


def mkdtso(name, compatible, order):
    compat = f'\tcompatible = "{compatible}";\n' if compatible else ""

    return f"""/dts-v1/;
/plugin/;

/ {{
{compat}	fragment@0 {{
		target-path = "/";
		__overlay__ {{
			{TESTNODE[1:]} {{
				applied-{name};
				order = "{order}";
			}};
		}};
	}};
}};
"""


def run(cmd, **kwargs):
    subprocess.run(cmd, check=True, **kwargs)


@pytest.fixture(scope="module")
def overlay_fit(testfs):
    path = os.path.join(testfs, "of-overlay")
    os.makedirs(path, exist_ok=True)

    images = configs = ""
    for name, (compatible, order) in OVERLAYS.items():
        with open(os.path.join(path, f"{name}.dtso"), "w") as f:
            f.write(mkdtso(name, compatible, order))

        if name != "kernel":
            images += ITS_OVERLAY % (name, name)
            configs += ITS_CONFIG % (name, name)

    images += ITS_OVERLAY % ("kernel", "kernel")

    with open(os.path.join(path, "overlays.its"), "w") as f:
        f.write(ITS % (images, configs))
    with open(os.path.join(path, "kernel.bin"), "wb") as f:
        f.write(b"not a kernel")

    try:
        for name in OVERLAYS:
            run(["dtc", "-I", "dts", "-O", "dtb", "-o", f"{name}.dtbo",
                 f"{name}.dtso"], cwd=path)
        run(["mkimage", "-f", "overlays.its", "overlays.fit"], cwd=path,
            stdout=subprocess.DEVNULL)
    except FileNotFoundError as e:
        shutil.rmtree(path)
        pytest.skip(f"missing dependency: {e}")

    # Change a property name in overlay-b, so its hash no longer matches
    with open(os.path.join(path, "overlays.fit"), "rb") as f:
        fit = f.read()
    assert fit.count(b"applied-b") == 1
    with open(os.path.join(path, "tampered.fit"), "wb") as f:
        f.write(fit.replace(b"applied-b", b"applied-x"))

    yield "/mnt/9p/testfs/of-overlay"

    shutil.rmtree(path)


@pytest.fixture(scope="function")
def overlay_vars(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_FITIMAGE", "CONFIG_OF_OVERLAY",
                  "CONFIG_CMD_OF_DUMP")

    yield

    barebox.run_check('global.of.overlay.path=""')
    barebox.run_check('global.of.overlay.pattern="*"')
    barebox.run_check('global.of.overlay.compatible=""')


def fixed_testnode(barebox):
    """Return the applied overlays and the order property of the test node
    in the device tree a kernel would be started with"""
    stdout, _, returncode = barebox.run(f"of_dump -F -p {TESTNODE}")
    if returncode != 0:
        return set(), None, stdout

    applied = {line[len("applied-"):-1] for line in stdout
               if line.startswith("applied-")}
    order = next((line.split('"')[1] for line in stdout
                  if line.startswith("order = ")), None)

    return applied, order, stdout


def test_of_overlay_fit(barebox, overlay_vars, overlay_fit):
    barebox.run_check(f"global.of.overlay.path={overlay_fit}/overlays.fit")

    applied, order, stdout = fixed_testnode(barebox)

    assert filter_errors(stdout) == [], stdout
    # conf-kernel is skipped, the others are applied in FIT order
    assert applied == {"a", "b", "c"}, stdout
    assert order == "c", stdout

    # Only the device tree passed to the kernel is changed
    _, _, returncode = barebox.run(f"of_dump {TESTNODE}")
    assert returncode != 0


def test_of_overlay_fit_pattern(barebox, overlay_vars, overlay_fit):
    barebox.run_check(f"global.of.overlay.path={overlay_fit}/overlays.fit")

    # The pattern matches on the configuration name
    barebox.run_check('global.of.overlay.pattern="conf-a overlay-b *-c"')
    applied, order, stdout = fixed_testnode(barebox)
    assert applied == {"a", "c"}, stdout
    assert order == "c", stdout

    barebox.run_check('global.of.overlay.pattern="conf-kernel"')
    applied, _, stdout = fixed_testnode(barebox)
    assert applied == set(), stdout


def test_of_overlay_fit_compatible(barebox, overlay_vars, overlay_fit):
    barebox.run_check(f"global.of.overlay.path={overlay_fit}/overlays.fit")

    # Overlays without a compatible are always applied
    barebox.run_check("global.of.overlay.compatible=barebox,fit-overlay-test-b")
    applied, order, stdout = fixed_testnode(barebox)
    assert applied == {"a", "b"}, stdout
    assert order == "b", stdout


def test_of_overlay_fit_bad_hash(barebox, overlay_vars, overlay_fit):
    barebox.run_check(f"global.of.overlay.path={overlay_fit}/tampered.fit")

    # overlay-b sets applied-x instead of applied-b in tampered.fit
    applied, _, stdout = fixed_testnode(barebox)
    assert "x" not in applied, stdout
    assert any("hash BAD" in line for line in stdout), stdout
