Barebox Bootchooser
===================

In many cases embedded systems are layed out redundantly with multiple
kernels and multiple root file systems. The bootchooser framework provides
the building blocks to model different use cases without the need to start
from scratch over and over again.

The bootchooser works on abstract boot targets, each with a set of properties
and implements an algorithm which selects the highest priority target to boot.

Bootchooser Targets
-------------------

A bootchooser target represents one target that barebox can boot. It consists
of a set of variables in the ``global.bootchooser.<targetname>`` namespace. The
following configuration variables are needed to describe a bootchooser target:

``global.bootchooser.<targetname>.boot``
  This controls what barebox actually boots for this target. This string can contain
  anything that the :ref:`boot <command_boot>` command understands.

``global.bootchooser.<targetname>.default_attempts``
  The default number of attempts that a target shall be tried starting.
``global.bootchooser.<targetname>.default_priority``
  The default priority of a target.


Additionally the following runtime variables are needed. Unlinke the configuration
variables these are automatically changed by the bootchooser algorithm:

``global.bootchooser.<targetname>.priority``
  The current priority of the target. Higher numbers have higher priorities. A priority
  of 0 means the target is disabled and won't be started.
``global.bootchooser.<targetname>.remaining_attempts``
  The remaining_attempts counter. Only targets with a remaining_attempts counter > 0
  are started.

The bootchooser algorithm generally only starts targets that have a priority
> 0 and a remaining_attempts counter > 0.

The Bootchooser Algorithm
-------------------------

The bootchooser algorithm is very simple. It works with two variables per target
and some optional flags. The variables are the remaining_attempts counter that
tells how many times the target will be started. The other variable is the priority,
the target with the highest priority will be used first, a zero priority means
the target is disabled.

When booting, bootchooser starts the target with the highest priority that has a
nonzero remaining_attempts counter. With every start of a target the remaining
attempts counter of this target is decremented by one. This means every targets
remaining_attempts counter reaches zero sooner or later and the target won't be
booted anymore. To prevent that, the remaining_attempts counter must be reset to
its default. There are different flags in the bootchooser which control resetting
the remaining_attempts counter, controlled by the ``global.bootchooser.reset_attempts``
variable. It holds a list of space separated flags. Possible values are:

- ``power-on``: The remaining_attempts counters of all enabled targets are reset
  after a power-on reset (``$global.system.reset="POR"``). This means after a power
  cycle all targets will be tried again for the configured number of retries
- ``all-zero``: The remaining_attempts counters of all enabled targets are reset
  when none of them has any remaining_attempts left.

Additionally the remaining_attempts counter can be reset manually using the
:ref:`command_bootchooser` command. This allows for custom conditions under which
a system is marked as good.
In case only the booted system itself knows when it is in a good state, the
barebox-state tool from the dt-utils_ package can used to reset the remaining_attempts
counter from the currently running system.

.. _dt-utils: http://git.pengutronix.de/?p=tools/dt-utils.git;a=summary

Bootchooser General Options
---------------------------

Additionally to the target options described above, bootchooser has some general
options not specific to any target.

``global.bootchooser.disable_on_zero_attempts``
  Boolean flag. if 1, bootchooser disables a target (sets priority to 0) whenever the
  remaining attempts counter reaches 0.
``global.bootchooser.default_attempts``
  The default number of attempts that a target shall be tried starting, used when not
  overwritten with the target specific variable of the same name.
``global.bootchooser.default_priority``
  The default priority of a target when not overwritten with the target specific variable
  of the same name.
``global.bootchooser.reset_attempts``
  A space separated list of events that cause bootchooser to reset the
  remaining_attempts counters of each target that has a non zero priority. possible values:
  * empty:  counters will never be reset``
  * power-on: counters will be reset after power-on-reset
  * all-zero: counters will be reset when all targets have zero remaining attempts
``global.bootchooser.reset_priorities``
  A space separated list of events that cause bootchooser to reset the priorities of
  all targets. Possible values:
  * empty: priorities will never be reset
  * all-zero: priorities will be reset when all targets have zero priority
``global.bootchooser.retry``
  If 1, bootchooser retries booting until one succeeds or no more valid targets exist.
``global.bootchooser.state_prefix``
  Variable prefix when bootchooser used with state framework as backend for storing runtime
  data, see below.
``global.bootchooser.targets``
  Space separated list of targets that are used. For each entry in the list a corresponding
  set of ``global.bootchooser.<name>``. variables must exist.
``global.bootchooser.last_chosen``
  bootchooser sets this to the target that was chosen on last boot (index)

Using the State Framework as Backend for Runtime Variable Data
--------------------------------------------------------------

Normally the data that is modified by the bootchooser during runtime is stored
in global variables (backed with NV). Alternatively the :ref:`state_framework`
can be used for this data, which allows to store this data redundantly
and in small EEPROM spaces. See :ref:`state_framework` to setup the state framework.
During barebox runtime each state instance will create a device
(usually named 'state' when only one is used) with a set of parameters. Set
``global.bootchooser.state_prefix`` to the name of the device and optionally the
namespace inside this device. For example when your state device is called 'state'
and inside that the 'bootchooser' namespace is used for describing the targets,
then set ``global.bootchooser.state_prefix`` to ``state.bootchooser``.

Example
-------

The following example shows how to initialize two targets, 'system0' and 'system1'.
Both boot from an UBIFS on nand0, the former has a priority of 21 and boots from
the volume 'system0' whereas the latter has a priority of 20 and boots from
the volume 'system1'.

.. code-block:: sh

  # initialize target 'system0'
  nv bootchooser.system0.boot=nand0.ubi.system0
  nv bootchooser.system0.default_attempts=3
  nv bootchooser.system0.default_priority=21

  # initialize target 'system1'
  nv bootchooser.system1.boot=nand0.ubi.system1
  nv bootchooser.system1.default_attempts=3
  nv bootchooser.system1.default_priority=20

  # make targets known
  nv bootchooser.targets="system0 system1"

  # retry until one target succeeds
  nv bootchooser.retry="true"

  # First try bootchooser, when no targets remain boot from network
  nv boot.default="bootchooser net"

Note that this example is for testing, normally the NV variables would be
initialized directly by files in the default environment, not with a script.

Scenarios
---------

This section describes some scenarios that can be solved with bootchooser. All
scenarios assume multiple slots that can be booted, where 'multiple' is anything
higher than one.

Scenario 1
##########

A system that shall always boot without user interaction. Staying in the bootloader
is not an option. In this scenario a target is started for the configured number
of remaining attempts. If it cannot successfully be started, the next target is chosen.
This happens until no targets are left to start, then all remaining attempts are
reset to their defaults and the first target is tried again.

Settings
^^^^^^^^
- ``global.bootchooser.reset_attempts="all-zero"``
- ``global.bootchooser.reset_priorities="all-zero"``
- ``global.bootchooser.disable_on_zero_attempts=0``
- ``global.bootchooser.retry=1``
- ``global.boot.default="bootchooser recovery"``
- Userspace marks as good

Deployment
^^^^^^^^^^

#. barebox or flash robot fills all slots with valid systems.
#. The all-zero settings will lead to automatically enabling the slots, no
   default settings are needed here.

Recovery
^^^^^^^^

Recovery will only be called when all targets are not startable (That is, no valid
Kernel found or read failure). Once a target is startable (A valid kernel is found
and started) Bootchooser will never fall through to the recovery target.

Scenario 2
##########

A system with multiple slots, a slot that was booted three times without success
shall never be booted again (except after update or user interaction).

Settings
^^^^^^^^

- ``global.bootchooser.reset_attempts=""``
- ``global.bootchooser.reset_priorities=""``
- ``global.bootchooser.disable_on_zero_attempts=0``
- ``global.bootchooser.retry=1``
- ``global.boot.default="bootchooser recovery"``
- Userspace marks as good

Deployment
^^^^^^^^^^

#. barebox or flash robot fills all slots with valid systems
#. barebox or flash robot marks slots as good or state contains non zero
   defaults for the remaining_attempts / priorities

Recovery
^^^^^^^^
done by 'recovery' boot target which is booted after the bootchooser falls through due to
the lack of bootable targets. This target can be:
- A system that will be booted as recovery
- A barebox script that will be started

Scenario 3
##########

A system with multiple slots and one recovery system. Booting a slot three times
without success disables it. A power cycle shall not be counted as failed boot.

Settings
^^^^^^^^

- ``global.bootchooser.reset_attempts="power-on"``
- ``global.bootchooser.reset_priorities=""``
- ``global.bootchooser.disable_on_zero_attempts=1``
- ``global.bootchooser.retry=1``
- ``global.boot.default="bootchooser recovery"``
- Userspace marks as good

Deployment
^^^^^^^^^^

- barebox or flash robot fills all slots with valid systems
- barebox or flash robot marks slots as good

Recovery
^^^^^^^^

Done by 'recovery' boot target which is booted after the bootchooser falls through
due to the lack of bootable targets. This target can be:
- A system that will be booted as recovery
- A barebox script that will be started

Updating systems
----------------

Updating a slot is the same among the different scenarios. It is assumed that the
update is done under a running Linux system which can be one of the regular bootchooser
slots or a dedicated recovery system. For the regular slots updating is done like:

- Set the priority of the inactive slot to 0.
- Update the inactive slot
- Set priority of the inactive slot to a higher value than the active slot
- Set remaining_attempts of the inactive slot to nonzero
- Reboot
- If necessary update the now inactive, not yet updated slot the same way

One way of updating systems is using RAUC_ which integrates well with the bootchooser
in barebox.

.. _RAUC: https://rauc.readthedocs.io/en/latest/ RAUC (
