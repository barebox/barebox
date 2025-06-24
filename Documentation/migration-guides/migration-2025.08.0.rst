Release v2025.08.0
==================

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
