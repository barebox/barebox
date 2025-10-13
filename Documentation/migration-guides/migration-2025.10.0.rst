Release v2025.10.0
==================

Rename in /dev
--------------

The i.MX SNVS device file is now simply called ``snvs`` instead of the
previous unwieldy name derived from device tree,
e.g., ``/dev/30370000.snvs@30370000:snvs-lpgpr.of0``.

EEPROMs that are pointed at by a device tree alias do no longer have
an extra 0 at the end, e.g., ``/dev/eeprom00`` has become ``/dev/eeprom0``.

AM62L DT Bindings
-----------------

The SCMI clock IDs for the AM62L have changed in ARM Trusted Firmware,
because the old assignment was not conforming to spec.

barebox now requires TF-A to contain commit
229d03adf ("PENDING: feat(ti): add missing scmi pds").
