Release v2025.08.0
==================

W^X on ARM
----------

``CONFIG_ARM_MMU_PERMISSIONS=y`` is now the default and instructs barebox to map
its memory regions with more restricted permissions: Data is no longer executable
and code as well as read-only data is no longer writable.

This can lead to breakage in code that had invalid assumptions beforehand,
e.g. code expecting on-chip SRAMs to be executable or bogus code casting away
const. Please report to upstream any issues that are resolved by disabling
``CONFIG_ARM_MMU_PERMISSIONS``, so they can be properly fixed.

Bootchooser
-----------

Bootchooser now interprets a top-level ``attempts_locked`` variable.
For more information, see :ref:`attempts lock feature <bootchooser,attempts_lock>`

Board support
-------------

The hardcoded Vexpress partition layout for barebox, its environment and state
have been changed. The first 5M of the NOR flash should now be reserved for
barebox use. barebox will, by default, fix up the new partition layout into
the kernel device tree.
