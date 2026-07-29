# SPDX-License-Identifier: GPL-2.0-only
import pytest
import os
import argparse
from labgrid.exceptions import NoDriverFoundError
from scripts import qemu_interactive
from test.py import helper


def transition_to_barebox(request, strategy):
    try:
        strategy.transition('barebox')
    except Exception as e:
        # If a normal strategy transition fails, there's no point in
        # continuing the test. Let's print stderr and exit.
        capmanager = request.config.pluginmanager.getplugin("capturemanager")
        with capmanager.global_and_fixture_disabled():
            _, stderr = capmanager.read_global_capture()
            pytest.exit(f"{type(e).__name__}(\"{e}\"). Standard error:\n{stderr}",
                        returncode=3)


@pytest.fixture(scope='function')
def barebox(request, strategy, target):
    transition_to_barebox(request, strategy)
    return target.get_driver('BareboxDriver')


@pytest.fixture(scope='function')
def shell(strategy, target):
    strategy.transition('shell')
    return target.get_driver('ShellDriver')


@pytest.fixture(scope="session")
def barebox_config(request, strategy, target):
    transition_to_barebox(request, strategy)
    command = target.get_driver("BareboxDriver")
    return helper.get_config(command)


lg_env = lg_builddir = None


def pytest_configure(config):
    removed_mode = getattr(config.option, 'qemu_interactive_removed', None)
    if removed_mode is not None:
        pytest.exit(qemu_interactive.replacement_message(removed_mode),
                    returncode=2)

    global lg_env
    global lg_builddir

    lg_builddir = qemu_interactive.resolve_lg_builddir()
    lg_env = config.option.lg_env
    if lg_env is None:
        lg_env = os.environ.get('LG_ENV')
    if lg_env is None:
        if lg_env := qemu_interactive.guess_lg_env(lg_builddir):
            os.environ['LG_ENV'] = lg_env


def pytest_report_header(config):
    report = []
    if lg_builddir is not None:
        report += [f"Build diectory: {lg_builddir}"]
    if lg_env is not None:
        report += [f"Labgrid Environment: {lg_env}"]
    return "\n".join(report)


def pytest_addoption(parser):
    parser.addoption('--interactive', action='store_const',
                     const='--interactive', dest='qemu_interactive_removed',
                     help=argparse.SUPPRESS)
    parser.addoption('--dry-run', action='store_const',
                     const='--dry-run', dest='qemu_interactive_removed',
                     help=argparse.SUPPRESS)
    parser.addoption('--dump-dtb', action='store_const',
                     const='--dump-dtb', dest='qemu_interactive_removed',
                     help=argparse.SUPPRESS)

    group = parser.getgroup('barebox-qemu', 'barebox QEMU options')
    qemu_interactive.register_shared_options(group)


def _pytest_exit(message, returncode=1):
    pytest.exit(message, returncode=returncode)


@pytest.fixture(scope="session")
def strategy(request, target, pytestconfig):  # noqa: max-complexity=30
    try:
        strategy = target.get_driver("Strategy")
    except NoDriverFoundError as e:
        pytest.exit(e)

    qemu_interactive.apply_shared_options(
        strategy, target, pytestconfig.option, interactive=False,
        fail=_pytest_exit)

    return strategy


@pytest.fixture(scope="session")
def testfs(strategy, env):
    if "testfs" not in env.get_target_features():
        pytest.skip("testfs not supported on this platform")

    path = os.path.join(os.environ["LG_BUILDDIR"], "testfs")
    return path
