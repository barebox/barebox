i.MX GPIOs
----------

Reading output GPIOs now returns the configured output level instead
of reading back the input register. This aligns us with what Linux
is doing, but may falsify readings of single-ended GPIOs that have
the SION bit configured.
