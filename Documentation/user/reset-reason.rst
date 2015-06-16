.. _reset_reason:

Reset Reason
------------

To handle a device in a secure and safe manner many applications are using
a watchdog or other ways to reset a system to bring it back into life if it
hangs or crashes somehow.

In these cases the hardware restarts and runs the bootloader again. Depending on
the root cause of the hang or crash, the bootloader sometimes should not just
re-start the main system again. Maybe it should do some kind of recovery instead.
For example it should wait for another update (for the case the cause of a
crash is a failed update) or should start into a fall back system instead.

In order to handle failing systems gracefully the bootloader needs the
information why it runs. This is called the "reset reason". It is provided by
the global variable ``system.reset`` and can be used in scripts via
``$global.system.reset``.

The following values can help to detect the reason why the bootloader runs:

* ``unknown``: the software wasn't able to detect the reset cause or there
  isn't support for this feature at all.
* ``POR`` (Power On Reset): a cold start. The power of the system
  was switched on. This is a regular state and nothing to worry about.
* ``RST`` (ReSeT): a warm start. The user has triggered a reset somehow. This
  is a regular state and nothing to worry about.
* ``WDG`` (WatchDoG): also some kind of warm start, but triggered by a watchdog
  unit. It depends on the application if this reason signals a regular state
  and therefore nothing to worry about, or if this state was entered by a hanging
  or crashed system and must implicitly be handled.
* ``WKE`` (WaKEup): a mixture of cold and warm start. The system is woken up
  from some state of suspend. This is a regular state and nothing to worry
  about.
* ``JTAG``: an external JTAG based debugger has triggered the reset.
* ``THERM`` (THERMal): some SoCs are able to detect if they got reset in
  response to an overtemperature event. This can be a regular state and nothing
  to worry about (the reset has brought the system back into a safe state) or
  must implicitly be handled.
* ``EXT`` (EXTernal): some SoCs have special device pins for external reset
  signals other than the ``RST`` one. Application specific how to handle this
  state.

It depends on your board/SoC and its features if the hardware is able to detect
these reset reasons. Most of the time only ``POR`` and ``RST`` are supported
but often ``WDG`` as well.
