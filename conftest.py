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


def get_enabled_arch(config):
    # Get the absolute path to the directory containing this script
    base_dir = os.path.dirname(os.path.abspath(__file__))

    # Path to the 'arch' directory relative to this script
    arch_dir = os.path.join(base_dir, "arch")

    if not os.path.isdir(arch_dir):
        return None

    # Optional mapping from directory names to config key suffixes
    arch_map = {"powerpc": "ppc"}

    for entry in os.listdir(arch_dir):
        path = os.path.join(arch_dir, entry)
        if os.path.isdir(path):
            key_suffix = arch_map.get(entry, entry).upper()
            config_key = f"CONFIG_{key_suffix}"
            if config.get(config_key):
                return entry

    return None


def guess_lg_env():
    config_file = helper.open_config_file(os.environ['LG_BUILDDIR'] + "/.config")
    config = helper.parse_config(config_file)
    if not config or not config.get('CONFIG_NAME'):
        return None
    arch = get_enabled_arch(config)
    if not arch:
        return None
    filename = os.path.join("test", arch, f"{config['CONFIG_NAME']}.yaml")
    if os.path.exists(filename):
        return filename
    return None


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

    lg_env = config.option.lg_env
    if lg_env is None:
        lg_env = os.environ.get('LG_ENV')
    if lg_env is None:
        if lg_env := guess_lg_env():
            os.environ['LG_ENV'] = lg_env


def pytest_addoption(parser):
    def assignment(arg):
            return arg.split('=', 1)

    parser.addoption('--interactive', action='store_const', const='qemu_interactive',
        dest='lg_initial_state',
        help=('(for debugging) skip tests and just start Qemu interactively'))
    parser.addoption('--dry-run', action='store_const', const='qemu_dry_run',
        dest='lg_initial_state',
        help=('(for debugging) skip tests and just print Qemu command line'))
    parser.addoption('--dump-dtb', action='store_const', const='qemu_dump_dtb',
        dest='lg_initial_state',
        help=('(for debugging) skip tests and just dump the Qemu device tree'))
    parser.addoption('--graphic', '--graphics', action='store_true', dest='qemu_graphics',
        help=('enable QEMU graphics output'))
    parser.addoption('--rng', action='count', dest='qemu_rng',
        help=('instantiate Virt I/O random number generator'))
    parser.addoption('--console', action='count', dest='qemu_console', default=0,
        help=('Pass an extra console (Virt I/O or ns16550_pci) to emulated barebox'))
    parser.addoption('--fs', action='append', dest='qemu_fs',
        default=[], metavar="[tag=]DIR", type=assignment,
        help=('Pass directory trees to emulated barebox. Can be specified more than once'))
    parser.addoption('--blk', action='append', dest='qemu_block',
        default=[], metavar="FILE",
        help=('Pass block device to emulated barebox. Can be specified more than once'))
    parser.addoption('--env', action='append', dest='qemu_fw_cfg',
        default=[], metavar="[envpath=]content | [envpath=]@filepath", type=assignment,
        help=('Pass barebox environment files to barebox. Can be specified more than once'))
    parser.addoption('--qemu', dest='qemu_arg', nargs=argparse.REMAINDER, default=[],
        help=('Pass all remaining options to QEMU as is'))
    parser.addoption('--bootarg', action='append', dest='bootarg', default=[],
        help=('Pass boot arguments to barebox for debugging purposes'))

@pytest.fixture(scope="session")
def strategy(request, target, pytestconfig):
    try:
        strategy = target.get_driver("Strategy")
    except NoDriverFoundError as e:
        pytest.exit(e)

    try:
        main = target.env.config.data["targets"]["main"]
        features = main["features"]
        qemu_bin = main["drivers"]["QEMUDriver"]["qemu_bin"]
    except KeyError:
        features = []
        qemu_bin = None

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

    if qemu_bin is not None:
        if not pytestconfig.option.qemu_graphics:
            graphics = '-nographic'
        elif qemu_bin == "qemu-system-x86_64":
            graphics = '-device isa-vga'
        elif 'pci' in features:
            graphics = '-device VGA'
        elif virtio:
            graphics = '-vga none -device ramfb'
        else:
            pytest.exit("--graphics unsupported for target\n", 1)

        strategy.append_qemu_args(graphics)

    for i, blk in enumerate(pytestconfig.option.qemu_block):
        if virtio:
            strategy.append_qemu_args(
                "-drive", f"if=none,format=raw,id=hd{i},file={blk}",
                "-device", f"virtio-blk-{virtio},drive=hd{i}"
            )
        else:
            pytest.exit("--blk unsupported for target\n", 1)

    for i, fw_cfg in enumerate(pytestconfig.option.qemu_fw_cfg):
        if virtio:
            value = fw_cfg.pop()
            envpath = fw_cfg.pop() if fw_cfg else f"data/fw_cfg{i}"

            if value.startswith('@'):
                source = f"file='{value[1:]}'"
            else:
                source = f"string='{value}'"

            strategy.append_qemu_args(
                '-fw_cfg', f'name=opt/org.barebox.env/{envpath},{source}'
            )
        else:
            pytest.exit("--env unsupported for target\n", 1)

    if len(pytestconfig.option.bootarg) > 0:
        strategy.append_qemu_bootargs(pytestconfig.option.bootarg)

    for arg in pytestconfig.option.qemu_arg:
        strategy.append_qemu_args(arg)

    for i, fs in enumerate(pytestconfig.option.qemu_fs):
        if virtio:
            path = fs.pop()
            tag = fs.pop() if fs else f"fs{i}"

            strategy.append_qemu_args(
                "-fsdev", f"local,security_model=mapped,id=fs{i},path={path}",
                "-device", f"virtio-9p-{virtio},id=fs{i},fsdev=fs{i},mount_tag={tag}"
            )
        else:
            pytest.exit("--fs unsupported for target\n", 1)

    state = request.config.option.lg_initial_state
    if state is not None:
        strategy.force(state)

    return strategy
