.. _reboot_mode:

Reboot Mode
-----------

To simplify debugging, many BootROMs sample registers that survive
a warm reset to customize the boot. These registers can e.g. indicate
that boot should happen from a different boot medium.

Likewise, many bootloaders reuse such registers, or if unavailable,
non-volatile memory to determine whether the OS requested a special
reboot mode, e.g. rebooting into an USB recovery mode. This is
common on Android systems.

barebox implements the upstream device tree bindings for
`reboot-modes <https://www.kernel.org/doc/Documentation/devicetree/bindings/power/reset/reboot-mode.txt>`_
to act upon reboot mode protocols specified in the device tree.

The device tree nodes list a number of reboot modes along with a
magic value for each. On reboot, an OS implementing the binding
would take the reboot command's argument and match it against the
modes in the device tree. If a match is found the associated magic
is written to the location referenced in the device tree node.

User API
~~~~~~~~

Devices registered with the reboot mode API gain two parameters:

 - ``$dev_of_reboot_mode.prev`` (read-only): The reboot mode that was
   set previous to barebox startup
 - ``$dev_of_reboot_mode.next``: The next reboot mode, for when the
   system is reset

The reboot mode driver core use the alias name if available to name
the device. By convention, this should end with ``.reboot_mode``, e.g.::

	/ {
		aliases {
			gpr.reboot_name = &reboot_name_gpr;
		};
	};

Reboot mode providers have priorities. The provider with the highest
priority has its parameters aliased as ``$global.system.reboot_mode.prev``
and ``$global.system.reboot_mode.next``.

Reset
~~~~~

Reboot modes can be stored on a syscon wrapping general purpose registers
that survives warm resets. If the system instead did reset via an external
power management IC, the registers may lose their value.

If such reboot mode storage is used, users must take care to use the correct
reset provider. In barebox, multiple reset providers may co-exist. They
``reset`` command allows listing and choosing a specific reboot mode.

Disambiguation
~~~~~~~~~~~~~~

Some uses of reboot modes partially overlap with other barebox
functionality. They all ultimately serve different purposes, however.

Comparison to reset reason
---------------------------

The reset reason ``$global.system.reset`` is populated by different drivers
to reflect the hardware cause of a reset, e.g. a watchdog. A reboot mode
describes the OS intention behind a reset, e.g. to fall into a recovery
mode. Reboot modes besides the default ``normal`` mode usually accompany
a reset reason of ``RST`` (because the OS intentionally triggered a reset
to activate the next reboot mode).

Comparison to bootsource
------------------------

``$bootsource`` reflects the current boot's medium as indicated by the
SoC. In cases where the reboot mode is used to communicate with the BootROM,
``$bootsource`` and ``$bootsource_instance`` may describe the same device
as the reboot mode.

For cases, where the communication instead happens between barebox and an OS,
they can be completely different, e.g. ``$bootsource`` may say barebox was
booted from ``spi-nor``, while the reboot mode describes that barebox should
boot the Kernel off an USB flash drive.

Comparison to barebox state
---------------------------

barebox state also allows sharing information between barebox and the OS,
but it does so while providing atomic updates, redundant storage and
optionally wear leveling. In contrast to state, reboot mode is just that:
a mode for a single reboot. barebox clears the reboot mode after reading it,
so this can be reliably used across one reset only.
