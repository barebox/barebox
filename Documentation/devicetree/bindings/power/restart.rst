System Restart Controllers
==========================

In addition to upstream bindings, following properties are understood:

Optional properties:

- ``restart-priority`` : Overrides the priority set by the driver. Normally,
  the device with the biggest reach should reset the system.
  See :ref:`_system_reset` for more information.

- ``barebox,restart-warm-bootrom`` : Restart will not cause loss to non-volatile
  registers sampled by the bootrom at startup. This is a necessary precondition
  for working :ref:`reboot_mode` communication between barebox and the SoC's
  BootROM.
