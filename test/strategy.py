import enum

import attr
import pytest
import subprocess
import os
import sys

from labgrid import target_factory, step, driver
from labgrid.strategy import Strategy, StrategyError

class Status(enum.Enum):
    unknown = 0
    off = 1
    barebox = 2
    qemu_dry_run = 3
    qemu_interactive = 4

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

        if state == "qemu_dry_run" or state == "qemu_interactive":
            cmd = self.get_qemu_base_args()

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
            pytest.exit('Can only force to: qemu_dry_run, qemu_interactive')

    def get_qemu_base_args(self):
        if self.qemu is None:
            pytest.exit('interactive mode only supported with QEMUDriver')

        try:
            # https://github.com/labgrid-project/labgrid/pull/1212
            cmd = self.qemu.get_qemu_base_args()
        except AttributeError:
            self.qemu.on_activate()
            orig = self.qemu._cmd
            cmd = []

            list_iter = enumerate(orig)
            for i, opt in list_iter:
                if opt == "-S":
                    continue
                opt2 = double_opt(opt, orig, i)
                if (opt2.startswith("-chardev socket,id=serialsocket") or
                   opt2 == "-serial chardev:serialsocket" or
                   opt2 == "-qmp stdio"):
                    # skip over two elements at once
                    next(list_iter, None)
                    continue

                cmd.append(opt)

        return cmd

def quote_cmd(cmd):
    quoted = map(lambda s : s if s.find(" ") == -1 else "'" + s + "'", cmd)
    return " ".join(quoted)

def double_opt(opt, orig, i):
    if opt == orig[-1]:
        return opt
    return " ".join([opt, orig[i + 1]])
