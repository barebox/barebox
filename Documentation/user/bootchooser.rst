.. _bootchooser:

Barebox Bootchooser
===================

In many cases embedded systems are laid out redundantly with multiple
kernels and multiple root file systems. The *bootchooser* framework provides
the building blocks to model different use cases without the need to start
from scratch over and over again.

*Bootchooser* works on abstract boot targets, each with a set of properties
and implements an algorithm which selects the highest priority target to boot.

Making *bootchooser* work requires a fixed set of configuration parameters
and a storage backend for saving status information.
Currently supported storage backends are either the barebox *state* framework
or nv variables (fallback only, not meant for production, because it's not
power-fail safe).

*Bootchooser* itself is executed as a normal barebox boot target, i.e. one
can start it via::

  boot bootchooser

or by e.g. setting ``boot.default`` to ``bootchooser``.

.. note:: As ``boot.default`` accepts multiple values, it can also be used to
  specify a fallback boot target in case bootchooser fails booting, e.g.
  ``bootchooser recovery``.

Bootchooser Targets
-------------------

A *bootchooser* boot target represents one target that barebox can boot. The
known targets are set in ``global.bootchooser.targets``. A target consists of a
set of variables in the ``global.bootchooser.<targetname>`` namespace. The
following configuration variables describe a *bootchooser* boot target:

``global.bootchooser.<targetname>.boot``
  This controls what barebox actually boots for this boot target. This string can
  contain anything that the :ref:`boot <command_boot>` command understands. If
  unset, the boot script ``/env/boot/<targetname>`` is called.

``global.bootchooser.<targetname>.default_attempts``
  The default number of attempts that a boot target shall be tried before skipping it.
  Defaults to ``bootchooser.default_attempts``, see below.
``global.bootchooser.<targetname>.default_priority``
  The default priority of a boot target.
  Defaults to ``global.bootchooser.default_priority``, see below.

The *bootchooser* algorithm generally only starts boot targets that have a ``priority``
> 0 and a ``remaining_attempts`` counter > 0.

.. _bootchooser,algorithm:

The Bootchooser Algorithm
-------------------------

The *bootchooser* algorithm is very simple. It works with two variables per boot target
and some optional flags. The variables are the ``remaining_attempts`` counter that
tells how many times the boot target will be started. The other variable is the ``priority``,
the boot target with the highest ``priority`` will be used first, a zero ``priority``
means the boot target is disabled.

When booting, *bootchooser* starts the boot target with the highest ``priority`` that
has a non-zero ``remaining_attempts`` counter. With every start of a boot target the
``remaining_attempts`` counter of this boot target is decremented by one.
This behavior assures that one can retry booting a target a limited number of
times to handle temporary issues (such as power outage) and optionally allows
booting a fallback in case of a permanent failure.
To indicate a successful boot, one must explicitly reset the remaining
attempts counter. See `Marking a Boot as Successful`_.

The bootchooser algorithm aborts when all enabled targets (priority > 0) have
no remaining attempts left.

To prevent ending up in an unbootable system after a number of failed boot
attempts, there is also a built-in mechanism to reset the counters to their defaults,
controlled by the ``global.bootchooser.reset_attempts`` variable.
Alternatively, counting down the remaining attempts can be disabled by
locking bootchooser boot attempts.
This is done by defining a (32-bit) ``attempts_locked`` variable in the
bootstate and setting its value to ``1`` (usually from userspace).

In scenarios where the system is rebootet too frequently (after the ``remaining_attempts``
counter is decremented, but before it got incremented again after a successful boot) and falls
back to the other boot target, the ``attempts_locked`` variable can be used to avoid this behavior
Bootchooser is prevented from decrementing the ``remaining_attempts`` counter and falling back
to the other target. It comes with the trade-off that a slot, that becomes broken
over time, it won't be detected anymore and will be booted indefinitely.

The variable affects all targets, is optional and its absence is
interpreted as ``0``, meaning that attempts are decremented normally.

The ``attempts_locked`` value does not influence the decision on which target
to boot if any, only whether to decrement the attempts when booting.

If ``global.bootchooser.retry`` is enabled (set to ``1``), the bootchooser
algorithm will iterate through all valid boot targets (and decrease their
counters) until one succeeds or none is left.
If it is disabled only one attempt will be made for each bootchooser call.

Marking a Boot as Successful
############################

While the bootchooser algorithm handles attempts decrementation, retries and
selection of the right boot target itself, it cannot decide if the system
booted successfully on its own.

In case only the booted system itself knows when it is in a good state,
it can report this to bootchooser from Linux userspace using the
*barebox-state* tool from the dt-utils_ package.::

  barebox-state [-n <state variable set>] -s [<prefix>.]<target>.remaining_attempts=<reset-value>
  barebox-state -n system_state -s bootstate.system1.remaining_attempts=3
  barebox-state -s system1.remaining_attempts=3

Alternatively barebox can be configured to mark the last boot successful based
on the :ref:`reset reason <reset_reason>` (i.e. != WDG) using the
:ref:`bootchooser command <command_bootchooser>`::

  bootchooser -s

This will reset the ``remaining_attempts`` counter of the *last chosen* slot to
its default value (``reset_attempts``).

Another option is to use ``attempts_locked``. Normally this should be controlled from
Linux userspace using the *barebox-state* tool, i.e.::

  barebox-state -s  bootstate.attempts_locked=1

It can also be locked via the :ref:`bootchooser command <command_bootchooser>`::

  bootchooser -l

or unlocked::

  bootchooser -L


.. _dt-utils: https://git.pengutronix.de/cgit/tools/dt-utils

General Bootchooser Options
---------------------------

In addition to the boot target options described above, *bootchooser* has some general
options not specific to any boot target.

``global.bootchooser.disable_on_zero_attempts``
  Boolean flag. If set to 1, *bootchooser* disables a boot target (sets priority
  to 0) whenever the remaining attempts counter reaches 0. Defaults to 0.
``global.bootchooser.default_attempts``
  The default number of attempts that a boot target shall be tried before skipping
  it, used when not overwritten with the boot target specific variable of the same
  name. Defaults to 3.
``global.bootchooser.default_priority``
  The default priority of a boot target when not overwritten with the target
  specific variable of the same name. Defaults to 1.
``global.bootchooser.reset_attempts``
  A space-separated list of conditions (checked during bootchooser start) that
  shall cause the ``remaining_attempts`` counters of all enabled targets to be
  reset. Possible values:

  * empty: Counters will never be reset (default).
  * ``power-on``: If a power-on reset (``$global.system.reset="POR"``) is detected.
    Happens after a power cycle.
  * ``reset``: If a generic reset (``$global.system.reset="RST"``) is detected.
  * ``all-zero``: If the ``remaining_attempts`` counters of all enabled targets
    are zero.
``global.bootchooser.reset_priorities``
  A space-separated list of conditions (checked during bootchooser start) that
  shall cause the ``priority``  of all boot targets to be reset. Possible values:

  * empty: Priorities will never be reset (default).
  * ``all-zero``: If all boot targets have zero ``priority``.
``global.bootchooser.retry``
  If set to 1, *bootchooser* retries booting until one succeeds or no more valid
  boot targets exist.
  Otherwise the ``boot`` command will return with an error after the first failed
  boot target. Defaults to 0.
``global.bootchooser.state_prefix``
  If set, this makes *bootchooser* use the *state* framework as backend for
  storing run-time data and defines the name of the state instance to use, see
  :ref:`below <bootchooser,state_framework>`. Defaults to an empty string.
``global.bootchooser.targets``
  Space-separated list of boot targets that are used. For each entry in the list
  a corresponding set of variables must exist in the chosen *bootchooser* storage
  backend.
  Defaults to an empty string.

.. _bootchooser,setup_example:

Setup Example
-------------

We want to set up a redundant machine with two bootable systems within one shared
memory, here a NAND type flash memory with a UBI partition. We have a 512 MiB NAND
type flash, to be used only for the root filesystem. The devicetree doesn't
define any partition, because we want to run one UBI partition with two volumes
for the redundant root filesystems on this flash memory.

.. code-block:: text

   nand@0 {
      [...]
   };

In order to configure this machine the following steps can be used:

.. code-block:: sh

   ubiformat /dev/nand0 -y
   ubiattach /dev/nand0
   ubimkvol /dev/nand0.ubi root_filesystem_1 256MiB
   ubimkvol /dev/nand0.ubi root_filesystem_2 0

The last command creates a volume which fills the remaining available space
on the NAND type flash memory, which will be most of the time smaller than
256 MiB due to factory bad blocks and lost data blocks for UBI's management.

After this preparation we can find two devices in ``/dev``:

- ``nand0.ubi.root_filesystem_1``
- ``nand0.ubi.root_filesystem_2``

These two devices can now be populated with their filesystem content. In our
example here we additionally assume, that these root filesystems contain a Linux
kernel with its corresponding devicetree via boot spec (refer to
:ref:`Bootloader Spec <bootloader_spec>` for further details).

Either device can be booted with the :ref:`boot <command_boot>` command command,
and thus can be used by *bootchooser* and we can start to configure the
*bootchooser* variables.

The following example shows how to initialize two boot targets, ``system1`` and
``system2``. Both boot from a UBIFS on ``nand0``, the former has a priority of
21 and boots from the volume ``root_filesystem_1`` whereas the latter has a
priority of 20 and boots from the volume ``root_filesystem_2``.

.. code-block:: sh

  # initialize target 'system1'
  nv bootchooser.system1.boot=nand0.ubi.root_filesystem_1
  nv bootchooser.system1.default_attempts=3
  nv bootchooser.system1.default_priority=21

  # initialize target 'system2'
  nv bootchooser.system2.boot=nand0.ubi.root_filesystem_2
  nv bootchooser.system2.default_attempts=3
  nv bootchooser.system2.default_priority=20

  # make targets known
  nv bootchooser.targets="system1 system2"

  # retry until one target succeeds
  nv bootchooser.retry=1

  # First try bootchooser, when no targets remain boot from network
  nv boot.default="bootchooser net"

.. note:: This example is for testing only, normally the NV variables would be
   initialized directly by files in the default environment, not with a script.

The run-time values are stored in environment variables as well. Alternatively,
they can be stored in a *state* variable set instead. Refer to
:ref:`using the state framework <bootchooser,state_framework>` for further
details.

Scenarios
---------

This section describes some scenarios that can be handled by bootchooser. All
scenarios assume multiple boot targets that can be booted, where 'multiple' is
anything higher than one.

Scenario 1
##########

System description:

- System with multiple boot targets
- One recovery system

Requirements:

- System shall always boot without user interaction.
- Staying in the bootloader is not an option.

In this scenario a boot target is started for the configured number of remaining
attempts. If it cannot be started successfully, the next boot target is chosen.
This repeats until no bootchooser boot targets are left to start, then the
recovery system is booted.

If all boot target's remaining attempts or priorities are 0 during bootchooser
start, the procedure repeats.

Settings
^^^^^^^^
- ``global.bootchooser.reset_attempts="all-zero"``
- ``global.bootchooser.reset_priorities="all-zero"``
- ``global.bootchooser.disable_on_zero_attempts=0``
- ``global.bootchooser.retry=1``
- ``global.boot.default="bootchooser recovery"``
- Userspace marks as good.

Deployment
^^^^^^^^^^

#. barebox or flash robot fills all boot targets with valid systems.
#. The all-zero settings will lead to automatically enabling the boot targets,
   no default settings are needed here.

Recovery
^^^^^^^^

Recovery will only be called if none of the boot targets are startable.
As long as one boot target is startable, *bootchooser* will never fall through
to the recovery boot target.

Could be a recovery system or barebox script.

Scenario 2
##########

System description:

- A system with multiple boot targets
- One recovery system

Requirements:

- Boot targets that were booted three times unsuccessfully shall never be booted
  again (except after update or user interaction).

Settings
^^^^^^^^

- ``global.bootchooser.reset_attempts=""``
- ``global.bootchooser.reset_priorities=""``
- ``global.bootchooser.disable_on_zero_attempts=0``
- ``global.bootchooser.retry=1``
- ``global.boot.default="bootchooser recovery"``
- Userspace marks as good.

Deployment
^^^^^^^^^^

#. barebox or flash robot fills all boot targets with valid systems.
#. barebox or flash robot marks boot targets as good or *state* contains non-zero
   defaults for the remaining_attempts/priorities.

Recovery
^^^^^^^^

Recovery system or barebox script to be started after *bootchooser* found no
bootable targets.

Scenario 3
##########

System description:

- A system with multiple boot targets
- One recovery system

Requirements:

- All enabled boot targets shall be tried after a power-on reset.
- Booting a boot target unsuccessfully three times shall disable it.

Settings
^^^^^^^^

- ``global.bootchooser.reset_attempts="power-on"``
- ``global.bootchooser.reset_priorities=""``
- ``global.bootchooser.disable_on_zero_attempts=1``
- ``global.bootchooser.retry=1``
- ``global.boot.default="bootchooser recovery"``
- Userspace marks as good.

Deployment
^^^^^^^^^^

#. barebox or flash robot fills all boot targets with valid systems.
#. barebox or flash robot marks boot targets as good.

Recovery
^^^^^^^^

Recovery system or barebox script to be started after *bootchooser* found no
bootable targets.

.. _bootchooser,state_framework:

Using the *State* Framework as Backend for Run-Time Variable Data
-----------------------------------------------------------------

Usually *bootchooser* modifies its data in global variables which are
connected to :ref:`non volatile variables <config_device>`.

Alternatively the :ref:`state_framework` can be used for this data, which
allows to store this data redundantly in some kind of persistent memory.

In order to let *bootchooser* use the *state* framework for its storage
backend, configure the ``bootchooser.state_prefix`` variable with the *state*
variable set instance name.

Usually a generic *state* variable set in the devicetree is defined like this
(refer to :ref:`barebox,state` for more details):

.. code-block:: text

   some_kind_of_state {
      [...]
   };

At barebox run-time this will result in a *state* variable set instance called
*some_kind_of_state*. You can also store variables unrelated to *bootchooser* (a
serial number, MAC address, …) in it.

Extending this *state* variable set by information required by *bootchooser*
is simply done by adding so called 'boot targets' and optionally one ``last_chosen``
node. It then looks like:

.. code-block:: text

   some_kind_of_state {
     [...]
     boot_target_1 {
         [...]
     };
     boot_target_2 {
         [...]
     };
   };

It could makes sense to store the result of the last *bootchooser* operation
in the *state* variable set as well. In order to do so, add a node with the name
``last_chosen`` to the *state* variable set. *bootchooser* will use it if present.
The *state* variable set definition then looks like:

.. code-block:: text

   some_kind_of_state {
     [...]
     boot_target_1 {
         [...]
     };
     boot_target_2 {
         [...]
     };
     last_chosen {
         reg = <offset 0x4>;
         type = "uint32";
     };
   };

The ``boot_target_*`` names shown above aren't variables themselves (like the other
variables in the *state* variable set), they are named containers instead, which
are used to group variables specific to *bootchooser*.

A 'boot target' container has the following fixed content:

.. code-block:: text

   some_boot_target {
          #address-cells = <1>;
          #size-cells = <1>;

          remaining_attempts {
              [...]
              default = <some value>; /* -> read note below */
          };

          priority {
              [...]
              default = <some value>; /* -> read note below */
          };
   };

.. important:: Since each variable in a *state* variable set requires a ``reg``
   property, the value of its ``reg`` property must be unique, e.g. the offsets
   must be consecutive from a global point of view, as they describe the
   storage layout in the backend memory.

So, ``remaining_attempts`` and ``priority`` are required variable nodes and are
used to setup the corresponding run-time environment variables in the
``global.bootchooser.<targetname>`` namespace.

.. important:: It is important to provide a ``default`` value for each variable
   for the case when the *state* variable set backend memory is uninitialized.
   This is also true if default values through *bootchooser's* environment
   variables are defined (e.g. ``bootchooser.default_attempts``,
   ``bootchooser.default_priority`` and their corresponding boot target specific
   variables). The former ones are forwarded to *bootchooser* to make a
   decision, the latter ones are used by *bootchooser* to make a decision
   the next time.

Example
#######

For this example we use the same system and its setup described in
:ref:`setup example <bootchooser,setup_example>`. The resulting devicetree
content for the *state* variable set looks like:

.. code-block:: text

   system_state {
        [...]
        system1 {
             #address-cells = <1>;
             #size-cells = <1>;
             remaining_attempts@0 {
                 reg = <0x0 0x4>;
                 type = "uint32";
                 default = <3>;
             };
             priority@4 {
                 reg = <0x4 0x4>;
                 type = "uint32";
                 default = <20>;
             };
        };

        system2 {
             #address-cells = <1>;
             #size-cells = <1>;
             remaining_attempts@8 {
                 reg = <0x8 0x4>;
                 type = "uint32";
                 default = <3>;
             };
             priority@c {
                 reg = <0xc 0x4>;
                 type = "uint32";
                 default = <21>;
             };
        };

        last_chosen@10 {
             reg = <0x10 0x4>;
             type = "uint32";
        };
   };

.. important:: While the ``system1/2`` nodes suggest a different namespace inside the
   *state* variable set, the actual variable's ``reg``-properties and their offset
   part are always relative to the whole *state* variable set and thus must be
   consecutive globally.

To make *bootchooser* use the so called ``system_state`` *state* variable set
instead of the NV run-time environment variables, we just set:

.. code-block:: text

   global.bootchooser.state_prefix=system_state

.. note:: Its a good idea to keep the ``bootchooser.<targetname>.default_priority``
   and ``bootchooser.<targetname>.default_attempts`` values in sync with the
   corresponding default values in the devicetree.

Using NV Run-Time Variable Data
-------------------------------

.. note:: Using NV variables as bootchooser's storage is only meant for
   evluation purposes, not for production. It is not power-fail safe.

The following run-time variables are needed. Unlike the configuration
variables their values are automatically updated by the *bootchooser* algorithm:

``nv.bootchooser.<targetname>.priority``
  The current ``priority`` of the boot target. Higher numbers have higher priorities.
  A ``priority`` of 0 means the boot target is disabled and won't be started.
``nv.bootchooser.<targetname>.remaining_attempts``
  The ``remaining_attempts`` counter. Only boot targets with a ``remaining_attempts``
  counter > 0 are started.
``nv.bootchooser.last_chosen``
  *bootchooser* sets this to the boot target that was chosen on last boot (index).

Updating systems
----------------

Updating a boot target is the same among the different scenarios. It is assumed
that the update is done under a running Linux system which can be one of the
regular *bootchooser* boot targets or a dedicated recovery system. For the
regular *bootchooser* boot targets updating is done like:

- Disable the inactive (e.g. not used right now) boot target by setting its
  ``priority`` to 0.
- Update the inactive boot target.
- Set ``remaining_attempts`` of the inactive boot target to nonzero.
- Enable the inactive boot target by setting its ``priority`` to a higher value
  than any other boot target (including the used one right now).
- Reboot.
- If necessary update the now inactive, not yet updated boot target the same way.

One way of updating systems is using RAUC_ which integrates well with *bootchooser*
in barebox.

.. _RAUC: https://rauc.readthedocs.io/en/latest/
