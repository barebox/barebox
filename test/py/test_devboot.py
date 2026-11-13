# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from .helper import skip_disabled
from .test_boot_globalvar_scope import write_bootscript, globals_now
from .test_boot_overrides import make_image, booted

FETCHDIR = "/tmp/devboot"


@pytest.fixture(scope="function")
def devboot_env(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_BOOT_DEVBOOT")

    saved = {var: barebox.run_check(f"echo ${{global.{var}}}")[0]
             for var in ("user", "hostname", "net.fetchdir", "boot.devboot",
                         "bootm.appendroot")}
    pwd = barebox.run_check("pwd")[0]

    barebox.run(f"mkdir {FETCHDIR}")
    barebox.run_check(f"global.net.fetchdir={FETCHDIR}")
    barebox.run_check("global.user=scope")
    barebox.run_check("global.hostname=test")
    barebox.run_check("global.bootm.appendroot=0")
    barebox.run_check(f"echo -o {FETCHDIR}/scope-devboot-test "
                      "'global linux.bootargs.extra=extra=1'")
    barebox.run_check(f"echo -a {FETCHDIR}/scope-devboot-test "
                      "'global.bootm.appendroot=1'")
    barebox.run_check(f"echo -a {FETCHDIR}/scope-devboot-test "
                      "'devboot_local=leak'")
    write_bootscript(barebox, "devboot-target", [
        "global.bootm.image=/does/not/exist",
        "echo target-sees ${global.linux.bootargs.extra} "
        "${global.bootm.appendroot}",
    ])
    write_bootscript(barebox, "devboot-nested", [
        "boot devboot-target",
    ])

    yield pwd

    for var, val in saved.items():
        barebox.run_check(f"global.{var}=\"{val}\"")
    barebox.run(f"rm -r {FETCHDIR}")
    barebox.run("rm /env/boot/devboot-target /env/boot/devboot-nested")


def assert_reverted(barebox, pwd):
    after = globals_now(barebox)
    assert "linux.bootargs.extra" not in after
    assert after["bootm.appendroot"] == "0"
    assert barebox.run_check("pwd") == [pwd]
    assert barebox.run_check("echo [$devboot_local]") == ["[]"]


def sourced(stdout):
    return sum("devboot: Sourcing " in line for line in stdout)


def test_devboot_option(barebox, devboot_env):
    stdout, _, ret = barebox.run("boot -D devboot-target devboot-target")
    assert ret != 0
    assert sourced(stdout) == 1
    # the devboot script's variables are visible in, and survive, each
    # boot target...
    assert stdout.count("target-sees extra=1 1") == 2
    # ...and are reverted afterwards
    assert_reverted(barebox, devboot_env)


def test_devboot_globalvar(barebox, devboot_env):
    barebox.run_check("global.boot.devboot=1")

    stdout, _, _ = barebox.run("boot devboot-target")
    assert sourced(stdout) == 1
    assert "target-sees extra=1 1" in stdout
    assert_reverted(barebox, devboot_env)


def test_devboot_command(barebox, devboot_env):
    stdout, _, _ = barebox.run("devboot devboot-target devboot-target")
    assert sourced(stdout) == 1
    assert stdout.count("target-sees extra=1 1") == 2
    assert_reverted(barebox, devboot_env)


def test_devboot_nested(barebox, devboot_env):
    # a boot script booting other entries doesn't source the script again
    stdout, _, _ = barebox.run("boot -D devboot-nested")
    assert sourced(stdout) == 1
    assert "target-sees extra=1 1" in stdout
    assert_reverted(barebox, devboot_env)


def test_devboot_nested_override(barebox, barebox_config, devboot_env):
    skip_disabled(barebox_config, "CONFIG_CMD_CAT", "CONFIG_CMD_MW")

    base = "/tmp/devboot-base"
    make_image(barebox, base)
    make_image(barebox, f"{FETCHDIR}/devboot-override")
    barebox.run_check(f"echo -a {FETCHDIR}/scope-devboot-test "
                      "devboot_image=devboot-override")
    write_bootscript(barebox, "devboot-target", [
        f"global.bootm.image={base}",
    ])

    stdout, _, _ = barebox.run("boot -D devboot-nested")
    barebox.run_check(f"rm {base}")

    assert sourced(stdout) == 1
    # relative to global.net.fetchdir, which is the working directory
    assert any("'devboot-override'" in line for line in booted(stdout))
    assert_reverted(barebox, devboot_env)


def test_devboot_dryrun(barebox, devboot_env):
    stdout, _, ret = barebox.run("boot -D -d devboot-target")
    assert ret == 0
    assert sourced(stdout) == 1
    assert "Would run /env/boot/devboot-target" in stdout
    assert_reverted(barebox, devboot_env)


def test_devboot_missing_script(barebox, devboot_env):
    barebox.run_check("global.hostname=nohost")

    stdout, _, ret = barebox.run("boot -D devboot-target")
    assert ret != 0
    assert any("Neither" in line for line in stdout)
    assert not any("Booting entry" in line for line in stdout)
    assert_reverted(barebox, devboot_env)
