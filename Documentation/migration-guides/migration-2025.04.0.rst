Release v2025.04.0
==================

Boot overrides
--------------

It's no longer possible to override
:ref:`global.bootm.image <magicvar_global_bootm_image>` with
the new ``boot -o`` syntax. This override led to issues when
substituting wildly different boot image formats.

This feature will likely be added back in future in a more
conservative form.
