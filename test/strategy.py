# SPDX-License-Identifier: GPL-2.0-only

import enum
import os
import socket
import tempfile
import time

import attr
import pytest
import re
from contextlib import contextmanager
import pexpect

from labgrid import target_factory, step, driver
from labgrid.strategy import Strategy, StrategyError
from labgrid.util import labgrid_version
from labgrid.util.timeout import Timeout

match = re.match(r'^(\d+?)\.', labgrid_version())
if match is None or int(match.group(1)) < 25:
    pytest.exit(f"Labgrid has version v{labgrid_version()}, "
                f"but barebox test suite requires at least v25.")


class Status(enum.Enum):
    unknown = 0
    off = 1
    on = 2
    barebox = 3
    shell = 4


@target_factory.reg_driver
@attr.s(eq=False)
class BareboxTestStrategy(Strategy):
    """BareboxTestStrategy - Strategy to switch to barebox"""
    bindings = {
        "power": "PowerProtocol",
        "console": "ConsoleProtocol",
        "barebox": "BareboxDriver",
        "shell": {"ShellDriver", None},
    }

    status = attr.ib(default=Status.unknown)
    init_commands = attr.ib(
        factory=list,
        validator=attr.validators.deep_iterable(
            member_validator=attr.validators.instance_of(str),
            iterable_validator=attr.validators.instance_of(list)))
    qemu_writel = attr.ib(
        factory=dict,
        validator=attr.validators.deep_mapping(
            key_validator=attr.validators.instance_of(int),
            value_validator=attr.validators.instance_of(int),
            mapping_validator=attr.validators.instance_of(dict)))
    qemu = None
    qtest_dir = None

    def __attrs_post_init__(self):
        super().__attrs_post_init__()
        if isinstance(self.console, driver.QEMUDriver):
            self.qemu = self.console
        elif self.qemu_writel:
            raise StrategyError("qemu_writel needs a QEMU target")

    @step(args=['status'])
    def transition(self, status, *, step):
        if not isinstance(status, Status):
            status = Status[status]
        if status == Status.unknown:
            raise StrategyError("can not transition to {}".format(status))
        elif status == self.status:
            step.skip("nothing to do")
            return  # nothing to do
        elif status == Status.off:
            self.target.deactivate(self.barebox)
            if self.shell:
                self.target.deactivate(self.shell)
            self.target.deactivate(self.console)
            self.target.activate(self.power)
            self.power.off()
        elif status == Status.on:
            self.transition(Status.off)  # pylint: disable=missing-kwoa
            self.target.activate(self.console)
            self.power_cycle()
        elif status == Status.barebox:
            self.transition(Status.on)  # pylint: disable=missing-kwoa
            # interrupt barebox
            self.activate_barebox()
        elif status == Status.shell:
            self._boot_kernel("")
        else:
            raise StrategyError(
                "no transition found from {} to {}".
                format(self.status, status)
            )
        self.status = status

    def power_cycle(self):
        if not self.qemu_writel:
            self.power.cycle()
            return

        # Write after the reset, but before the CPUs run
        self.power.off()
        self.qemu_write_registers(self.qemu.monitor_command)
        self.power.on()

    def qemu_write_registers(self, monitor_command):
        """Write qemu_writel to the stopped machine via qtest"""
        if self.qtest_dir is None:
            self.qtest_dir = tempfile.TemporaryDirectory(prefix="barebox-qtest-")
        path = os.path.join(self.qtest_dir.name, "qtest")

        # QEMU only replies via the first qtest object, so reuse it
        chardevs = monitor_command("query-chardev")
        if not any(chardev["label"] == "qtest" for chardev in chardevs):
            monitor_command("chardev-add", {"id": "qtest", "backend": {
                "type": "socket", "data": {"server": True, "wait": False, "addr": {
                    "type": "unix", "data": {"path": path}}}}})
            monitor_command("object-add", {"qom-type": "qtest", "id": "qtest",
                                           "chardev": "qtest", "log": "none"})

        with socket.socket(socket.AF_UNIX) as sock:
            sock.connect(path)
            with sock.makefile("rw") as qtest:
                for addr, value in self.qemu_writel.items():
                    qtest.write(f"writel {addr:#x} {value:#x}\n")
                    qtest.flush()
                    reply = qtest.readline()
                    if reply != "OK\n":
                        raise StrategyError(f"writel {addr:#x}: {reply!r}")

    def activate_barebox(self):
        self.target.activate(self.barebox)
        for cmd in self.init_commands:
            self.barebox.run_check(cmd)

    def barebox_bootm(self, image=None):
        self.barebox._run(f"global.loglevel={self.barebox.saved_log_level}",
                          adjust_log_level=False)
        if image:
            self.console.sendline(f"bootm -v {image}")
        else:
            self.console.sendline("bootm -v")

    @contextmanager
    def boot_barebox(self, boottarget=None, bootm=False):
        self.transition(Status.barebox)

        try:
            self.barebox.run("global.bootm.efi=disabled # if it exists")
            if bootm:
                self.barebox_bootm(boottarget)
            else:
                self.barebox.boot(boottarget)

            self.target.deactivate(self.barebox)
            self.target.activate(self.barebox)
            yield self.barebox
        finally:
            self.target.deactivate(self.barebox)
            self.power_cycle()
            self.activate_barebox()

    def skip_cdrom_installer(self):
        # Wait for installer to boot. Send Enter every cycle to dismiss
        # GRUB menu and any installer prompts. The screen TUI has a clock
        # in the status bar that makes pattern matching unreliable, so
        # avoid matching on it.
        timeout = Timeout(float(self.shell.login_timeout))

        while not timeout.expired:
            self.console.sendline("")
            index, _, _, _ = self.console.expect(
                ["Starting system log daemon", pexpect.TIMEOUT],
                timeout=self.shell.await_login_timeout)
            if index == 0:
                break

        # Switch to screen window 3 (shell) and disable the status bar.
        # Retry until the shell prompt appears.
        while not timeout.expired:
            self.console.sendline("\x013")
            time.sleep(0.5)
            self.console.sendline("\x01:hardstatus ignore")
            time.sleep(0.5)
            self.console.sendline("")
            index, _, _, _ = self.console.expect(
                [self.shell.prompt, pexpect.TIMEOUT],
                timeout=self.shell.await_login_timeout)
            if index == 0:
                break

    def _boot_kernel(self, boottarget=None, bootm=False, skip_cdrom_installer=True):
        self.transition(Status.barebox)

        if bootm:
            self.barebox_bootm(boottarget)
        else:
            self.barebox.boot(boottarget)

        self.barebox.await_boot()

        if skip_cdrom_installer:
            self.skip_cdrom_installer()

        self.target.activate(self.shell)

    @contextmanager
    def boot_kernel(self, boottarget=None, bootm=False, skip_cdrom_installer=True):
        self.transition(Status.barebox)

        try:
            self._boot_kernel(boottarget=boottarget, bootm=bootm,
                              skip_cdrom_installer=skip_cdrom_installer)
            yield self.shell
        finally:
            self.target.deactivate(self.shell)
            self.power_cycle()
            self.activate_barebox()

