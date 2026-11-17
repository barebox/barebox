# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from .helper import devinfo, skip_disabled


def write_bootscript(barebox, name, lines):
    barebox.run_check(f"echo -o /env/boot/{name} '#!/bin/sh'")
    for line in lines:
        # Variable references must reach the script unexpanded. Neither
        # quoting nor a backslash achieves that here, so splice in the
        # dollar sign from a variable.
        line = line.replace("$", "${dollar}")
        barebox.run_check(f"dollar='$'; echo -a /env/boot/{name} \"{line}\"")


def globals_now(barebox):
    params = devinfo(barebox, "global")["Parameters"]
    # devinfo prints "value (type: ...)", strip the annotation
    return {k: v.split(" (type:")[0].strip('"') for k, v in params.items()}


@pytest.fixture(scope="function")
def scope_globals(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_BOOT", "CONFIG_CMD_GLOBAL",
                  "CONFIG_CMD_ECHO")

    barebox.run_check("global.bootm.image=/preset/image")
    barebox.run_check("global linux.bootargs.preset=preset=1")
    barebox.run_check("global linux.bootargs.removed=removed=1")
    barebox.run_check("global.bootm.appendroot=0")
    old_user = barebox.run_check("echo ${global.user}")[0]

    yield old_user

    barebox.run_check(f"global.user={old_user}")
    barebox.run("global -r linux.bootargs.preset")
    barebox.run("global -r linux.bootargs.removed")
    barebox.run_check("global.bootm.image=")


def test_boot_entry_reverts_globalvars(barebox, scope_globals):
    old_user = scope_globals
    before = globals_now(barebox)

    # boot related globalvars outside of bootm.* and linux.bootargs.*,
    # where the configuration has them
    others = {var: val for var, val in (("of.overlay.path", "/does/not/exist"),
                                        ("linux.bootargs_append", "1"))
              if var in before}

    write_bootscript(barebox, "scope", [
        *(f"global.{var}={val}" for var, val in others.items()),
        "global.bootm.image=/does/not/exist",
        "global.bootm.oftree=/does/not/exist.dtb",
        "global.bootm.appendroot=1",
        "global.linux.bootargs.dyn.root=root=/dev/leak",
        "global linux.bootargs.leak=leak=1",
        "global.linux.bootargs.preset=preset=2",
        "global -r linux.bootargs.removed",
        "global.user=scope-test",
    ])

    # bootm fails on the nonexistent image, so boot returns
    barebox.run("boot scope")

    after = globals_now(barebox)

    # values within the boot scope are restored...
    assert after["bootm.image"] == "/preset/image"
    assert after["bootm.oftree"] == before["bootm.oftree"]
    assert after["bootm.appendroot"] == "0"
    assert after["linux.bootargs.preset"] == "preset=1"
    for var in others:
        assert after[var] == before[var]

    # ...vars created by the boot entry are gone...
    assert "linux.bootargs.leak" not in after
    assert "linux.bootargs.dyn.root" not in after
    assert "linux.bootargs.dyn.ip" not in after

    # ...vars removed by the boot entry are back...
    assert after["linux.bootargs.removed"] == "removed=1"

    # ...and the loadaddrs don't come back as 0xfffffffe after a failed boot
    assert after["bootm.image.loadaddr"] == before["bootm.image.loadaddr"]

    # globalvars outside the boot scope are left alone
    assert after["user"] == "scope-test"
    assert old_user != "scope-test"


@pytest.mark.usefixtures("scope_globals")
def test_boot_entry_scope_nests(barebox):
    # devboot-style: an outer entry sets up bootargs that must be visible
    # in, and survive, the inner boot entry
    write_bootscript(barebox, "scope-inner", [
        "global.bootm.image=/does/not/exist",
        "echo inner-sees ${global.linux.bootargs.outer}",
        "global.linux.bootargs.outer=outer=clobbered",
        "global linux.bootargs.inner=inner=1",
    ])
    write_bootscript(barebox, "scope-outer", [
        "global linux.bootargs.outer=outer=1",
        "boot scope-inner",
        "echo outer-sees ${global.linux.bootargs.outer} "
        "inner=${global.linux.bootargs.inner}",
    ])

    stdout, _, _ = barebox.run("boot scope-outer")
    assert "inner-sees outer=1" in stdout
    assert "outer-sees outer=1 inner=" in stdout

    after = globals_now(barebox)
    assert "linux.bootargs.outer" not in after
    assert "linux.bootargs.inner" not in after
    assert after["bootm.image"] == "/preset/image"
