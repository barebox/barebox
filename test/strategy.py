import enum

import attr

from labgrid import target_factory, step
from labgrid.strategy import Strategy, StrategyError

class Status(enum.Enum):
    unknown = 0
    off = 1
    barebox = 2

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

    def __attrs_post_init__(self):
        super().__attrs_post_init__()

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
