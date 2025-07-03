# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from .helper import globalvars_set, devinfo, getparam_int, \
                    getstate_int, getenv_int


def fit_name(suffix):
    return f"/mnt/9p/testfs/barebox-{suffix}.fit"


def generate_bootscript(barebox, command, name="test"):
    barebox.run_check(f"echo -o /env/boot/{name} '#!/bin/sh'")
    barebox.run_check(f"echo -a /env/boot/{name} '{command}'")
    return name


def assert_state(barebox, s0p, s0a, s1p, s1a):
    info = devinfo(barebox, "state")

    assert getparam_int(info, "bootstate.system0.priority") == s0p
    assert getparam_int(info, "bootstate.system0.remaining_attempts") == s0a
    assert getparam_int(info, "bootstate.system1.priority") == s1p
    assert getparam_int(info, "bootstate.system1.remaining_attempts") == s1a


def assert_boot(barebox, system_num, s0p, s0a, s1p, s1a):
    expected = system_num
    unexpected = system_num ^ 1

    stdout, _, _ = barebox.run("boot bootchooser")
    assert f"Test is booting system{expected}" in stdout
    assert f"Test is booting system{unexpected}" not in stdout

    assert_state(barebox, s0p, s0a, s1p, s1a)


@pytest.fixture(scope="function")
def bootchooser(barebox, env):
    if 'barebox-state' not in env.get_target_features():
        return None

    system0 = generate_bootscript(barebox, "echo Test is booting system0",
                                  "system0")
    system1 = generate_bootscript(barebox, "echo Test is booting system1",
                                  "system1")

    globals_chg = {
        "bootchooser.default_attempts": "3",
        "bootchooser.retry": "0",
        "bootchooser.state_prefix": "state.bootstate",
        "bootchooser.targets": "system0 system1",
    }

    globals_new = {
        "bootchooser.system0.boot": system0,
        "bootchooser.system0.default_priority": "20",
        "bootchooser.system1.boot": system1,
        "bootchooser.system1.default_priority": "21"
    }

    globalvars_set(barebox, globals_chg)
    globalvars_set(barebox, globals_new, create=True)

    barebox.run_check("bootchooser -a default -p default")
    return True


def test_bootchooser(barebox, env, bootchooser):
    if bootchooser is None:
        pytest.xfail("barebox-state feature not specified")

    assert_state(barebox, s0p=20, s0a=3, s1p=21, s1a=3)

    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=2)
    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=1)
    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=2, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=1, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=0, s1p=21, s1a=0)

    barebox.run_check("bootchooser -a default -p default")

    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=2)
    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=1)
    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=2, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=1, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=0, s1p=21, s1a=0)


def test_bootchooser_lock_command(barebox, env, bootchooser):
    if bootchooser is None:
        pytest.xfail("barebox-state feature not specified")

    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=2)

    stdout = barebox.run_check("bootchooser -i")
    assert 'Locking of boot attempt counter: disabled' in stdout[-3:]
    assert getstate_int(barebox, "attempts_locked") == 0

    barebox.run_check("bootchooser -l")

    stdout = barebox.run_check("bootchooser -i")
    assert 'Locking of boot attempt counter: enabled' in stdout[-3:]
    assert getstate_int(barebox, "attempts_locked") == 0

    for i in range(4):
        assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=2)

    barebox.run_check("bootchooser -L")

    stdout = barebox.run_check("bootchooser -i")
    assert 'Locking of boot attempt counter: disabled' in stdout[-3:]
    assert getstate_int(barebox, "attempts_locked") == 0

    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=1)
    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=2, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=1, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=0, s1p=21, s1a=0)


def test_bootchooser_lock_state(barebox, env, bootchooser):
    if bootchooser is None:
        pytest.xfail("barebox-state feature not specified")

    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=2)

    stdout = barebox.run_check("bootchooser -i")
    assert 'Locking of boot attempt counter: disabled' in stdout[-3:]
    assert getstate_int(barebox, "attempts_locked") == 0

    barebox.run_check("state.bootstate.attempts_locked=1")
    assert getenv_int(barebox, "state.dirty") == 1
    stdout = barebox.run_check("state -s")
    assert getenv_int(barebox, "state.dirty") == 0

    stdout = barebox.run_check("bootchooser -i")
    assert 'Locking of boot attempt counter: enabled' in stdout[-3:]
    assert getstate_int(barebox, "attempts_locked") == 1

    for i in range(4):
        assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=2)

    assert getenv_int(barebox, "state.dirty") == 0

    barebox.run_check("state.bootstate.attempts_locked=0")

    stdout = barebox.run_check("bootchooser -i")
    assert 'Locking of boot attempt counter: disabled' in stdout[-3:]
    assert getstate_int(barebox, "attempts_locked") == 0

    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=1)
    assert_boot(barebox, 1, s0p=20, s0a=3, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=2, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=1, s1p=21, s1a=0)
    assert_boot(barebox, 0, s0p=20, s0a=0, s1p=21, s1a=0)
