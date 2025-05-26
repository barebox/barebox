Release v2025.06.0
==================

Configuration options
---------------------

* ``CONFIG_USB_ONBOARD_HUB`` has been renamed to ``CONFIG_USB_ONBOARD_DEV``
  for compatibility with Linux.

Board code
----------

The C ``char`` data type is now always unsigned. This may affect code
assuming ``char`` to be signed as was the default on x86, mips and kvx.
