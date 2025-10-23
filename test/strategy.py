# SPDX-License-Identifier: GPL-2.0-only

import enum

import attr
import pytest
import subprocess
import re

from labgrid import target_factory, step, driver
from labgrid.strategy import Strategy, StrategyError
from labgrid.util import labgrid_version

match = re.match(r'^(\d+?)\.', labgrid_version())
if match is None or int(match.group(1)) < 25:
    pytest.exit(f"Labgrid has version v{labgrid_version()}, "
                f"but barebox test suite requires at least v25.")


class Status(enum.Enum):
    unknown = 0
    off = 1
    barebox = 2
    qemu_dry_run = 3
    qemu_interactive = 4
    qemu_dump_dtb = 5


@target_factory.reg_driver
@attr.s(eq=False)
class BareboxTestStrategy(Strategy):
    """BareboxTestStrategy - Strategy to switch to barebox"""
    bindings = {
        "power": "PowerProtocol",
        "console": "ConsoleProtocol",
        "barebox": "BareboxDriver",
    }

    status = attr.ib(default=Status.unknown)
    qemu = None

    def __attrs_post_init__(self):
        super().__attrs_post_init__()
        if isinstance(self.console, driver.QEMUDriver):
            self.qemu = self.console

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
            self.target.deactivate(self.console)
            self.target.activate(self.power)
            self.power.off()
        elif status == Status.barebox:
            self.transition(Status.off)  # pylint: disable=missing-kwoa
            self.target.activate(self.console)
            # cycle power
            self.power.cycle()
            # interrupt barebox
            self.target.activate(self.barebox)
        else:
            raise StrategyError(
                "no transition found from {} to {}".
                format(self.status, status)
            )
        self.status = status

    def force(self, state):
        self.transition(Status.off)  # pylint: disable=missing-kwoa

        if state.startswith("qemu_"):
            if state == "qemu_dump_dtb":
                self.qemu.machine += f",dumpdtb={self.target.name}.dtb"

            cmd = self.qemu.get_qemu_base_args()

            cmd.append("-serial")
            cmd.append("mon:stdio")
            cmd.append("-trace")
            cmd.append("file=/dev/null")

            with open("/dev/tty", "r+b", buffering=0) as tty:
                tty.write(bytes("running: \n{}\n".format(quote_cmd(cmd)), "UTF-8"))
                if state == "qemu_dry_run":
                    pytest.exit('Dry run. Terminating.')
                subprocess.run(cmd, stdin=tty, stdout=tty, stderr=tty)

            pytest.exit('Interactive session terminated')
        else:
            pytest.exit('Can only force to: qemu_dry_run, qemu_interactive, qemu_dump_dtb')

    def append_qemu_args(self, *args):
        if self.qemu is None:
            pytest.exit('Qemu option supplied for non-Qemu target')
        for arg in args:
            self.console.extra_args += " " + arg

    def append_qemu_bootargs(self, args):
        if self.qemu is None:
            pytest.exit('Qemu option supplied for non-Qemu target')
        if self.console.boot_args is None:
            self.console.boot_args = ""
        self.console.boot_args += " ".join(args)


def quote_cmd(cmd):
    quoted = map(lambda s : s if s.find(" ") == -1 else "'" + s + "'", cmd)
    return " ".join(quoted)
