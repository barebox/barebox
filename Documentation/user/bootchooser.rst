.. _bootchooser:

Barebox Bootchooser
===================

In many cases embedded systems are layed out redundantly with multiple
kernels and multiple root file systems. The *bootchooser* framework provides
the building blocks to model different use cases without the need to start
from scratch over and over again.

The *bootchooser* works on abstract boot targets, each with a set of properties
and implements an algorithm which selects the highest priority target to boot.

Bootchooser Targets
-------------------

A *bootchooser* boot target represents one target that barebox can boot. It consists
of a set of variables in the ``global.bootchooser.<targetname>`` namespace. The
following configuration variables are needed to describe a *bootchooser* boot target:

``global.bootchooser.<targetname>.boot``
  This controls what barebox actually boots for this boot target. This string can
  contain anything that the :ref:`boot <command_boot>` command understands.

``global.bootchooser.<targetname>.default_attempts``
  The default number of attempts that a boot target shall be tried before skipping it.
``global.bootchooser.<targetname>.default_priority``
  The default priority of a boot target.


Additionally the following run-time variables are needed. Unlike the configuration
variables their values are automatically updated by the *bootchooser* algorithm:

``global.bootchooser.<targetname>.priority``
  The current ``priority`` of the boot target. Higher numbers have higher priorities.
  A ``priority`` of 0 means the boot target is disabled and won't be started.
``global.bootchooser.<targetname>.remaining_attempts``
  The ``remaining_attempts`` counter. Only boot targets with a ``remaining_attempts``
  counter > 0 are started.

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
``remaining_attempts`` counter of this boot target is decremented by one. This means
every boot target's ``remaining_attempts`` counter reaches zero sooner or later and
the boot target won't be booted anymore. To prevent that, the ``remaining_attempts``
counter must be reset to its default. There are different flags in the
*bootchooser* which control resetting the ``remaining_attempts`` counter,
controlled by the ``global.bootchooser.reset_attempts`` variable. It holds a
list of space separated flags. Possible values are:

- empty: counters will never be reset
- ``power-on``: The ``remaining_attempts`` counters of all enabled boot targets are reset
  after a ``power-on`` reset (``$global.system.reset="POR"``). This means after a power
  cycle all boot targets will be tried again for the configured number of retries.
- ``all-zero``: The ``remaining_attempts`` counters of all enabled boot targets are
  reset when none of them has any ``remaining_attempts`` left.

Additionally the ``remaining_attempts`` counter can be reset manually using the
:ref:`bootchoser command <command_bootchooser>`. This allows for custom conditions
under which a system is marked as good.
In case only the booted system itself knows when it is in a good state, the
barebox-state tool from the dt-utils_ package can be used to reset the
``remaining_attempts`` counter from the running system.

.. _dt-utils: http://git.pengutronix.de/?p=tools/dt-utils.git;a=summary

General Bootchooser Options
---------------------------

In addition to the boot target options described above, *bootchooser* has some general
options not specific to any boot target.

``global.bootchooser.disable_on_zero_attempts``
  Boolean flag. If set to 1, *bootchooser* disables a boot target (sets priority
  to 0) whenever the remaining attempts counter reaches 0.
``global.bootchooser.default_attempts``
  The default number of attempts that a boot target shall be tried before skipping
  it, used when not overwritten with the boot target specific variable of the same
  name.
``global.bootchooser.default_priority``
  The default priority of a boot target when not overwritten with the target
  specific variable of the same name.
``global.bootchooser.reset_attempts``
  Already described in :ref:`Bootchooser Algorithm <bootchooser,algorithm>`
``global.bootchooser.reset_priorities``
  A space separated list of events that cause *bootchooser* to reset the priorities of
  all boot targets. Possible values:

  * empty: priorities will never be reset
  * ``all-zero``: priorities will be reset when all targets have zero priority
``global.bootchooser.retry``
  If set to 1, *bootchooser* retries booting until one succeeds or no more valid
  boot targets exist.
  Otherwise the ``boot`` command will return with an error after the first failed
  boot target.
``global.bootchooser.state_prefix``
  Variable prefix when *bootchooser* is used with the *state* framework as backend
  for storing run-time data, see below.
``global.bootchooser.targets``
  Space separated list of boot targets that are used. For each entry in the list
  a corresponding
  set of ``global.bootchooser.<targetname>.<variablename>`` variables must exist.
``global.bootchooser.last_chosen``
  *bootchooser* sets this to the boot target that was chosen on last boot (index).

.. _bootchooser,setup_example:

Setup Example
-------------

We want to set up a redundant machine with two bootable systems within one shared
memory, here a NAND type flash memory with a UBI partition. We have a 512 MiB NAND
type flash, to be used only for the root filesystem. The devicetree doesn't
define any partition, because we want to run one UBI partition with two volumens
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
and thus can be used by the *bootchooser* and we can start to configure the
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

- a system that shall always boot without user interaction
- staying in the bootloader is not an option.

In this scenario a boot target is started for the configured number of remaining
attempts. If it cannot be started successfully, the next boot target is chosen.
This repeats until no boot targets are left to start, then all remaining attempts
are reset to their defaults and the first boot target is tried again.

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

Recovery will only be called when all boot targets are not startable (That is,
no valid kernel found or read failure). Once a boot target is startable (a
valid kernel is found and started) *bootchooser* will never fall through to
the recovery boot target.

Scenario 2
##########

- a system with multiple boot targets
- one recovery system

A boot target that was booted three times without success shall never be booted
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
#. barebox or flash robot marks boot targets as good or *state* contains non zero
   defaults for the remaining_attempts/priorities.

Recovery
^^^^^^^^

Done by 'recovery' boot target which is booted after the *bootchooser* falls
through due to the lack of bootable targets. This boot target can be:

- a system that will be booted as recovery.
- a barebox script that will be started.

Scenario 3
##########

- a system with multiple boot targets
- one recovery system
- a power cycle shall not be counted as failed boot.

Booting a boot target three times without success disables it.

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

Done by 'recovery' boot target which is booted after the *bootchooser* falls
through due to the lack of bootable targets. This target can be:

- a system that will be booted as recovery.
- a barebox script that will be started.

.. _bootchooser,state_framework:

Using the *State* Framework as Backend for Run-Time Variable Data
-----------------------------------------------------------------

Usually the *bootchooser* modifies its data in global variables which are
connected to :ref:`non volatile variables <config_device>`.

Alternatively the :ref:`state_framework` can be used for this data, which
allows to store this data redundantly in some kind of persistent memory.

In order to let the *bootchooser* use the *state* framework for its storage
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

Extending this *state* variable set by information required by the *bootchooser*
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
   This is also true if default values through the *bootchooser's* environment
   variables are defined (e.g. ``bootchooser.default_attempts``,
   ``bootchooser.default_priority`` and their corresponding boot target specific
   variables). The former ones are forwarded to the *bootchooser* to make a
   decision, the latter ones are used by the *bootchooser* to make a decision
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

One way of updating systems is using RAUC_ which integrates well with the *bootchooser*
in barebox.

.. _RAUC: https://rauc.readthedocs.io/en/latest/
