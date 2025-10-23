Release v2025.11.0
==================

i.MX GPIOs
----------

Reading output GPIOs now returns the configured output level instead
of reading back the input register. This aligns us with what Linux
is doing, but may falsify readings of single-ended GPIOs that have
the SION bit configured.

Board support
-------------

Karo TX6X
^^^^^^^^^

The barebox update handler for this SoM no longer unconditionally updates
/dev/mmc3.boot0, but instead it now updates the inactive boot partition
on /dev/mmc3 and then sets it as active allowing for power-fail safety.
