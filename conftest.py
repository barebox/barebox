import pytest
import os
import argparse
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

@pytest.fixture(scope="session")
def barebox_config(request, strategy, target):
    transition_to_barebox(request, strategy)
    command = target.get_driver("BareboxDriver")
    return helper.get_config(command)

def pytest_configure(config):
    if 'LG_BUILDDIR' not in os.environ:
        if 'KBUILD_OUTPUT' in os.environ:
            os.environ['LG_BUILDDIR'] = os.environ['KBUILD_OUTPUT']
        elif os.path.isdir('build'):
            os.environ['LG_BUILDDIR'] = os.path.realpath('build')
        else:
            os.environ['LG_BUILDDIR'] = os.getcwd()

    if os.environ['LG_BUILDDIR'] is not None:
        os.environ['LG_BUILDDIR'] = os.path.realpath(os.environ['LG_BUILDDIR'])

def pytest_addoption(parser):
    parser.addoption('--interactive', action='store_const', const='qemu_interactive',
        dest='lg_initial_state',
        help=('(for debugging) skip tests and just start Qemu interactively'))
    parser.addoption('--dry-run', action='store_const', const='qemu_dry_run',
        dest='lg_initial_state',
        help=('(for debugging) skip tests and just print Qemu command line'))
    parser.addoption('--rng', action='count', dest='qemu_rng',
        help=('instantiate Virt I/O random number generator'))
    parser.addoption('--console', action='count', dest='qemu_console', default=0,
        help=('Pass an extra console (Virt I/O or ns16550_pci) to emulated barebox'))
    parser.addoption('--blk', action='append', dest='qemu_block',
        default=[], metavar="FILE",
        help=('Pass block device to emulated barebox. Can be specified more than once'))
    parser.addoption('--qemu', dest='qemu_arg', nargs=argparse.REMAINDER, default=[],
        help=('Pass all remaining options to QEMU as is'))

@pytest.fixture(scope="session")
def strategy(request, target, pytestconfig):
    try:
        strategy = target.get_driver("Strategy")
    except NoDriverFoundError as e:
        pytest.exit(e)

    try:
        features = target.env.config.data["targets"]["main"]["features"]
    except KeyError:
        features = []

    virtio = None

    if "virtio-mmio" in features:
        virtio = "device"
    if "virtio-pci" in features:
        virtio = "pci,disable-modern=off"
        features.append("pci")

    if virtio and pytestconfig.option.qemu_rng:
        for i in range(pytestconfig.option.qemu_rng):
            strategy.append_qemu_args("-device", f"virtio-rng-{virtio}")

    for i in range(pytestconfig.option.qemu_console):
        if virtio and i == 0:
            strategy.append_qemu_args(
                "-device", f"virtio-serial-{virtio}",
                "-chardev", f"pty,id=virtcon{i}",
                "-device", f"virtconsole,chardev=virtcon{i},name=console.virtcon{i}"
            )
            continue

        # ns16550 serial driver only works with x86 so far
        if 'pci' in features:
            strategy.append_qemu_args(
                "-chardev", f"pty,id=pcicon{i}",
                "-device", f"pci-serial,chardev=pcicon{i}"
            )
        else:
            pytest.exit("barebox currently supports only a single extra virtio console\n", 1)

    for i, blk in enumerate(pytestconfig.option.qemu_block):
        if virtio:
            strategy.append_qemu_args(
                "-drive", f"if=none,format=raw,id=hd{i},file={blk}",
                "-device", f"virtio-blk-{virtio},drive=hd{i}"
            )
        else:
            pytest.exit("--blk unsupported for target\n", 1)

    for arg in pytestconfig.option.qemu_arg:
        strategy.append_qemu_args(arg)

    state = request.config.option.lg_initial_state
    if state is not None:
        strategy.force(state)

    return strategy
