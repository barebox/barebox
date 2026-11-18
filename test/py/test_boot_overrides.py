# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from .helper import skip_disabled
from .test_boot_globalvar_scope import write_bootscript

OVERRIDE = "/tmp/override-image"
BASE = "/tmp/base-image"
OUTER = "/tmp/outer-image"


def make_image(barebox, path):
    # bootm wants a full page of a known image type. There's no handler for
    # ELF images here, but it only fails after reporting the image it boots
    barebox.run_check(f"echo -o {path} image")
    for _ in range(6):
        barebox.run_check(f"cat -o {path}.tmp {path} {path}")
        barebox.run_check(f"cat -o {path} {path}.tmp {path}.tmp")
    barebox.run_check(f"rm {path}.tmp")
    barebox.run_check(f"mw -b -d {path} 0 0x7f 0x45 0x4c 0x46")


def booted(stdout):
    return [line for line in stdout if line.startswith("Loading ")]


@pytest.fixture(scope="function")
def override_env(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_BOOT_OVERRIDE", "CONFIG_CMD_ECHO",
                  "CONFIG_CMD_CAT", "CONFIG_CMD_MW")
    # bootm refuses the fake images before reporting what it loads
    if "CONFIG_BOOTM_FORCE_SIGNED_IMAGES" in barebox_config:
        pytest.skip("bootm only boots signed FIT images")

    make_image(barebox, BASE)
    make_image(barebox, OVERRIDE)
    make_image(barebox, OUTER)

    write_bootscript(barebox, "ovr-target", [
        f"global.bootm.image={BASE}",
    ])
    write_bootscript(barebox, "ovr-nested", [
        "boot ovr-target",
    ])
    write_bootscript(barebox, "ovr-nested-override", [
        f"boot -o bootm.image={OVERRIDE} ovr-target",
    ])
    write_bootscript(barebox, "ovr-bootm", [
        f"bootm {BASE}",
    ])

    yield

    barebox.run(f"rm {BASE} {OVERRIDE} {OUTER}")
    barebox.run("rm /env/boot/ovr-target /env/boot/ovr-nested "
                "/env/boot/ovr-nested-override /env/boot/ovr-bootm")


@pytest.mark.usefixtures("override_env")
def test_override_entry(barebox):
    stdout, _, _ = barebox.run("boot ovr-target")
    assert any(BASE in line for line in booted(stdout))

    stdout, _, _ = barebox.run(f"boot -o bootm.image={OVERRIDE} ovr-target")
    assert any(OVERRIDE in line for line in booted(stdout))


@pytest.mark.usefixtures("override_env")
def test_override_nested_boot(barebox):
    stdout, _, _ = barebox.run(f"boot -o bootm.image={OVERRIDE} ovr-nested")
    assert any(OVERRIDE in line for line in booted(stdout))
    assert not any(BASE in line for line in booted(stdout))


@pytest.mark.usefixtures("override_env")
def test_override_outer_takes_precedence(barebox):
    stdout, _, _ = barebox.run(f"boot -o bootm.image={OUTER} ovr-nested-override")
    assert any(OUTER in line for line in booted(stdout))
    assert not any(OVERRIDE in line for line in booted(stdout))


@pytest.mark.usefixtures("override_env")
def test_override_bootm_command(barebox):
    stdout, _, _ = barebox.run(f"boot -o bootm.image={OVERRIDE} ovr-bootm")
    assert any(OVERRIDE in line for line in booted(stdout))
    assert not any(BASE in line for line in booted(stdout))


@pytest.mark.usefixtures("override_env")
def test_override_ends_with_boot(barebox):
    # the override must not stick to a later bootm outside of the boot command
    barebox.run(f"boot -o bootm.image={OVERRIDE} ovr-target")

    stdout, _, _ = barebox.run(f"bootm {BASE}")
    assert any(BASE in line for line in booted(stdout))
