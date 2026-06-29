#!/usr/bin/env python3
# /// script
# requires-python = ">=3.9"
# dependencies = ["labgrid"]
# ///
# SPDX-License-Identifier: GPL-2.0-only

import argparse
import glob
import os
import re
import subprocess
import sys


MODE_INTERACTIVE = "qemu_interactive"
MODE_DRY_RUN = "qemu_dry_run"
MODE_DUMP_DTB = "qemu_dump_dtb"


class QemuInteractiveError(Exception):
    def __init__(self, message, returncode=1):
        super().__init__(message)
        self.returncode = returncode


def _add_option(parser, *opts, **attrs):
    if hasattr(parser, "addoption"):
        return parser.addoption(*opts, **attrs)

    return parser.add_argument(*opts, **attrs)


def _assignment(arg):
    return arg.split("=", 1)


def register_shared_options(parser):
    _add_option(parser, "--graphic", "--graphics", action="store_true",
                dest="qemu_graphics", help="enable QEMU graphics output")
    _add_option(parser, "--rng", action="count", dest="qemu_rng",
                help="instantiate Virt I/O random number generator")
    _add_option(parser, "--console", action="count", dest="qemu_console",
                default=0,
                help="Pass an extra console (Virt I/O or ns16550_pci) to emulated barebox")
    _add_option(parser, "--fs", action="append", dest="qemu_fs",
                default=[], metavar="[tag=]DIR", type=_assignment,
                help="Pass directory trees to emulated barebox. Can be specified more than once")
    _add_option(parser, "--blk", action="append", dest="qemu_block",
                default=[], metavar="FILE",
                help="Pass block device to emulated barebox. Can be specified more than once")
    _add_option(parser, "--usbblk", action="append", dest="qemu_usbblock",
                default=[], metavar="FILE",
                help="Pass USB block device to emulated barebox. Can be specified more than once")
    _add_option(parser, "--env", action="append", dest="qemu_fw_cfg",
                type=_assignment, default=[],
                metavar="[envpath=]content | [envpath=]@filepath",
                help="Pass barebox environment files to barebox. Can be specified more than once")
    _add_option(parser, "--qemu", dest="qemu_arg",
                nargs=argparse.REMAINDER, default=[],
                help="Pass all remaining options to QEMU as is")
    _add_option(parser, "--bootarg", action="append", dest="bootarg",
                default=[],
                help="Pass boot arguments to barebox for debugging purposes")
    _add_option(parser, "--port-forward", metavar="PORT", action="append",
                dest="qemu_port", default=[],
                help="Forward incoming TCP or UDP connections on specified PORT")


def register_interactive_options(parser):
    _add_option(parser, "--lg-env", dest="lg_env", metavar="LG_ENV",
                help="labgrid environment config file")

    if hasattr(parser, "add_mutually_exclusive_group"):
        mode_parser = parser.add_mutually_exclusive_group()
    else:
        mode_parser = parser

    _add_option(mode_parser, "--dry-run", action="store_const",
                const=MODE_DRY_RUN, default=MODE_INTERACTIVE,
                dest="qemu_mode",
                help="print the QEMU command line instead of executing it")
    _add_option(mode_parser, "--dump-dtb", action="store_const",
                const=MODE_DUMP_DTB, dest="qemu_mode",
                help="run QEMU only to dump the device tree")


def repo_root():
    return os.path.realpath(os.path.join(os.path.dirname(__file__), os.pardir))


def resolve_lg_builddir():
    if "LG_BUILDDIR" not in os.environ:
        if "KBUILD_OUTPUT" in os.environ:
            os.environ["LG_BUILDDIR"] = os.environ["KBUILD_OUTPUT"]
        elif os.path.isdir("build"):
            os.environ["LG_BUILDDIR"] = os.path.realpath("build")
        else:
            os.environ["LG_BUILDDIR"] = os.getcwd()

    os.environ["LG_BUILDDIR"] = os.path.realpath(os.environ["LG_BUILDDIR"])
    return os.environ["LG_BUILDDIR"]


def get_config_name(builddir):
    try:
        with open(os.path.join(builddir, ".config")) as config:
            for line in config:
                match = re.match(r'^CONFIG_NAME="(.*)"$', line)
                if match:
                    return match.group(1)
    except OSError:
        pass

    return None


def guess_lg_env(builddir=None):
    if builddir is None:
        builddir = os.environ.get("LG_BUILDDIR")
    if builddir is None:
        return None

    config_name = get_config_name(builddir)
    if config_name is None:
        return None

    matches = glob.glob(os.path.join(repo_root(), "test", "*",
                                     f"{config_name}.yaml"))
    if matches:
        return matches[0]

    return None


def resolve_lg_env(lg_env=None):
    if lg_env is None:
        lg_env = os.environ.get("LG_ENV")
    if lg_env is None:
        lg_env = guess_lg_env()

    if lg_env is not None:
        os.environ["LG_ENV"] = lg_env

    return lg_env


def _get_option(options, name, default=None):
    if isinstance(options, dict):
        return options.get(name, default)

    return getattr(options, name, default)


def _get_list_option(options, name):
    value = _get_option(options, name, [])
    if value is None:
        return []
    return list(value)


def _fail(message, returncode=1):
    raise QemuInteractiveError(message, returncode)


def _append_qemu_args(strategy, fail, *args):
    if strategy.qemu is None:
        fail("Qemu option supplied for non-Qemu target")

    for arg in args:
        strategy.console.extra_args += " " + arg


def _append_qemu_bootargs(strategy, fail, args):
    if strategy.qemu is None:
        fail("Qemu option supplied for non-Qemu target")
    if strategy.console.boot_args is None:
        strategy.console.boot_args = ""
    strategy.console.boot_args += " ".join(args)


def apply_shared_options(strategy, target, options, *, interactive, fail=None):  # noqa: max-complexity=30
    if fail is None:
        fail = _fail

    try:
        main = target.env.config.data["targets"]["main"]
    except KeyError:
        main = {}

    features = list(main.get("features", []))
    yaml_env = dict(main.get("env", {}))
    qemu_driver = main.get("drivers", {}).get("QEMUDriver")
    qemu_bin = None

    if qemu_driver is not None:
        qemu_bin = qemu_driver.get("qemu_bin")
        features.append("qemu")

    virtio = None

    if "virtio-mmio" in features:
        virtio = "device"
        _append_qemu_args(strategy, fail, '-global virtio-mmio.force-legacy=false')
    if "virtio-pci" in features:
        virtio = "pci,disable-modern=off"
        features.append("pci")

    qemu_rng = _get_option(options, "qemu_rng", 0) or 0
    for _ in range(qemu_rng):
        if virtio:
            _append_qemu_args(strategy, fail, "-device", f"virtio-rng-{virtio}")

    qemu_console = _get_option(options, "qemu_console", 0) or 0
    for i in range(qemu_console):
        if virtio and i == 0:
            _append_qemu_args(
                strategy, fail,
                "-device", f"virtio-serial-{virtio}",
                "-chardev", f"pty,id=virtcon{i}",
                "-device", f"virtconsole,chardev=virtcon{i},name=console.virtcon{i}"
            )
            continue

        # ns16550 serial driver only works with x86 so far
        if 'pci' in features:
            _append_qemu_args(
                strategy, fail,
                "-chardev", f"pty,id=pcicon{i}",
                "-device", f"pci-serial,chardev=pcicon{i}"
            )
        else:
            fail("barebox currently supports only a single extra virtio console\n")

    if "qemu" in features:
        if not _get_option(options, "qemu_graphics", False):
            graphics = '-nographic'
        elif qemu_bin == "qemu-system-x86_64":
            graphics = '-device isa-vga'
        elif 'pci' in features:
            graphics = '-device VGA'
        elif virtio:
            graphics = '-vga none -device ramfb'
            graphics += f' -device virtio-keyboard-{virtio}'
        else:
            fail("--graphics unsupported for target\n")

        if graphics is not None and not interactive:
            graphics += ' -display none'

        _append_qemu_args(strategy, fail, graphics)

    for i, blk in enumerate(_get_list_option(options, "qemu_block")):
        if virtio:
            _append_qemu_args(
                strategy, fail,
                "-drive", f"if=none,format=raw,id=hd{i},file={blk}",
                "-device", f"virtio-blk-{virtio},drive=hd{i}"
            )
        else:
            fail("--blk unsupported for target\n")

    for i, blk in enumerate(_get_list_option(options, "qemu_usbblock")):
        _append_qemu_args(
            strategy, fail,
            "-drive", f"if=none,format=raw,id=usbstorage{i},file={blk}",
            "-device", f"usb-storage,drive=usbstorage{i},bus=usb-bus.0,removable=on"
        )

    envopts = {}

    for i, fw_cfg in enumerate(_get_list_option(options, "qemu_fw_cfg")):
        value = fw_cfg.pop()
        envpath = fw_cfg.pop() if fw_cfg else f"data/fw_cfg{i}"

        envopts[envpath] = value

    for envpath, value in {**yaml_env, **envopts}.items():
        if virtio:
            if isinstance(value, str) and value.startswith('@'):
                source = f"file='{value[1:]}'"
            else:
                source = f"string='{value}'"

            _append_qemu_args(
                strategy, fail,
                '-fw_cfg', f'name=opt/org.barebox.env/{envpath},{source}'
            )
        else:
            fail("env unsupported for target\n")

    bootargs = _get_list_option(options, "bootarg")
    if bootargs:
        _append_qemu_bootargs(strategy, fail, bootargs)

    for arg in _get_list_option(options, "qemu_arg"):
        _append_qemu_args(strategy, fail, arg)

    qemu_nic = "user,id=net0"

    for port in _get_list_option(options, "qemu_port"):
        qemu_nic += f",hostfwd=udp:127.0.0.2:{port}-:{port}"
        qemu_nic += f",hostfwd=tcp:127.0.0.2:{port}-:{port}"

    qemu_fs = [list(fs) for fs in _get_list_option(options, "qemu_fs")]

    if "testfs" in features:
        if not any(fs and fs[0] == "testfs" for fs in qemu_fs):
            testfs_path = os.path.join(os.environ["LG_BUILDDIR"], "testfs")
            qemu_fs.append(["testfs", testfs_path])
            os.makedirs(testfs_path, exist_ok=True)
            qemu_nic += f",tftp={testfs_path}"

    if "qemu" in features:
        _append_qemu_args(strategy, fail, "-nic", qemu_nic)

    for i, fs in enumerate(qemu_fs):
        if virtio:
            path = fs.pop()
            tag = fs.pop() if fs else f"fs{i}"

            _append_qemu_args(
                strategy, fail,
                "-fsdev", f"local,security_model=none,id=fs{i},path={path}",
                "-device", f"virtio-9p-{virtio},id=fs{i},fsdev=fs{i},mount_tag={tag}"
            )
        else:
            fail("--fs unsupported for target\n")


def replacement_message(mode):
    return (f"pytest {mode} is no longer supported. Use "
            "scripts/qemu_interactive.py instead.")


def _split_qemu_args(argv):
    for i, arg in enumerate(argv):
        if arg in ("--qemu", "--"):
            return argv[:i], argv[i + 1:]
        if arg.startswith("--qemu="):
            return argv[:i], [arg.split("=", 1)[1]] + argv[i + 1:]

    return argv, []


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Start a barebox labgrid QEMU target interactively",
        epilog="Raw QEMU arguments may follow either --qemu or --."
    )

    register_interactive_options(
        parser.add_argument_group("qemu_interactive.py options"))
    register_shared_options(parser.add_argument_group("QEMU options"))
    parser.add_argument("lg_env_pos", nargs="*", metavar="LG_ENV",
                        help="labgrid environment config file")

    return parser


def parse_args(argv=None):
    if argv is None:
        argv = sys.argv[1:]

    parser = build_arg_parser()
    argv, qemu_args = _split_qemu_args(list(argv))
    args = parser.parse_args(argv)

    if len(args.lg_env_pos) > 1:
        parser.error("only one positional labgrid environment may be specified")

    if args.lg_env_pos:
        if args.lg_env is not None:
            parser.error("labgrid environment specified both positionally and with --lg-env")
        args.lg_env = args.lg_env_pos[0]

    del args.lg_env_pos
    args.qemu_arg = qemu_args

    return args


def quote_cmd(cmd):
    quoted = map(lambda s: s if s.find(" ") == -1 else "'" + s + "'", cmd)
    return " ".join(quoted)


def build_qemu_command(strategy, mode):
    if strategy.qemu is None:
        raise QemuInteractiveError(f"Can't enter {mode} for non-QEMU target")

    if mode != MODE_DRY_RUN:
        strategy.transition("off")

    if mode == MODE_DUMP_DTB:
        strategy.qemu.machine += f",dumpdtb={strategy.target.name}.dtb"

    cmd = strategy.qemu.get_qemu_base_args()
    cmd.extend(["-serial", "mon:stdio", "-trace", "file=/dev/null"])

    return cmd


def run_qemu(strategy, mode):
    if mode not in (MODE_INTERACTIVE, MODE_DRY_RUN, MODE_DUMP_DTB):
        raise QemuInteractiveError(
            "Can only force to: qemu_dry_run, qemu_interactive, qemu_dump_dtb")

    cmd = build_qemu_command(strategy, mode)
    command = "running: \n{}\n".format(quote_cmd(cmd))

    if mode == MODE_DRY_RUN:
        print(command, end="")
        return 0

    with open("/dev/tty", "r+b", buffering=0) as tty:
        tty.write(bytes(command, "UTF-8"))
        return subprocess.run(cmd, stdin=tty, stdout=tty, stderr=tty).returncode


def main(argv=None):
    from labgrid import Environment
    from labgrid.exceptions import NoDriverFoundError

    args = parse_args(argv)
    env = None

    try:
        resolve_lg_builddir()
        lg_env = resolve_lg_env(args.lg_env)
        if lg_env is None:
            raise QemuInteractiveError(
                "no labgrid environment specified; pass --lg-env, "
                "a positional LG_ENV, set LG_ENV, or build a config with CONFIG_NAME",
                2)

        env = Environment(lg_env)
        target = env.get_target("main")
        if target is None:
            raise QemuInteractiveError("labgrid environment has no main target")

        try:
            strategy = target.get_driver("Strategy")
        except NoDriverFoundError as e:
            raise QemuInteractiveError(str(e)) from e

        apply_shared_options(strategy, target, args,
                             interactive=args.qemu_mode != MODE_DUMP_DTB)

        return run_qemu(strategy, args.qemu_mode)
    except QemuInteractiveError as e:
        print(f"error: {e}", file=sys.stderr)
        return e.returncode
    finally:
        if env is not None:
            env.cleanup()


if __name__ == "__main__":
    sys.exit(main())
