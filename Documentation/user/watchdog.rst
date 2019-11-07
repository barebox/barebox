Watchdog Support
================

Barebox Watchdog Functionality
------------------------------

In some cases we are not able to influence the hardware design anymore or while
developing one needs to be able to feed the watchdog to disable it from within
the bootloader. For these scenarios barebox provides the watchdog framework
with the following functionality and at least ``CONFIG_WATCHDOG`` should be
enabled.

Polling
~~~~~~~

Watchdog polling/feeding allows to feed the watchdog and keep it running on one
side and to not reset the system on the other side. It is needed on hardware
with short-time watchdogs. For example the Atheros ar9331 watchdog has a
maximal timeout of 7 seconds, so it may reset even on netboot.
Or it can be used on systems where the watchdog is already running and can't be
disabled, an example for that is the watchdog of the i.MX2 series.
This functionally can be seen as a threat, since in error cases barebox will
continue to feed the watchdog even if that is not desired. So, depending on
your needs ``CONFIG_WATCHDOG_POLLER`` can be enabled or disabled at compile
time. Even if barebox was built with watchdog polling support, it is not
enabled by default. To start polling from command line run:

.. code-block:: console

  wdog0.autoping=1

**NOTE** Using this feature might have the effect that the watchdog is
effectively disabled. In case barebox is stuck in a loop that includes feeding
the watchdog, then the watchdog will never trigger. Only use this feature
during development or when a bad watchdog design (Short watchdog timeout
enabled as boot default) doesn't give you another choice.

The poller interval is not configurable, but fixed at 500ms and the watchdog
timeout is configured by default to the maximum of the supported values by
hardware. To change the timeout used by the poller, run:

.. code-block:: console

  wdog0.timeout_cur=7

To read the current watchdog's configuration, run:

.. code-block:: console

  devinfo wdog0

The output may look as follows where ``timeout_cur`` and ``timeout_max`` are
measured in seconds:

.. code-block:: console

  barebox@DPTechnics DPT-Module:/ devinfo wdog0
  Parameters:
    autoping: 1 (type: bool)
    timeout_cur: 7 (type: uint32)
    timeout_max: 10 (type: uint32)

Use barebox' environment to persist these changes between reboots:

.. code-block:: console

  nv dev.wdog0.autoping=1
  nv dev.wdog0.timeout_cur=7

Boot Watchdog Timeout
~~~~~~~~~~~~~~~~~~~~~

With this functionality barebox may start a watchdog or update the timeout of
an already-running one, just before kicking the boot image. It can be
configured temporarily via

.. code-block:: console

  global boot.watchdog_timeout=10

or persistently by

.. code-block:: console

  nv boot.watchdog_timeout=10

where the used value again is measured in seconds.

On a system with multiple watchdogs, the watchdog with the highest positive
priority is the one affected by the ``boot.watchdog_timeout`` parameter.  If
multiple watchdogs share the same priority, only one will be started.
