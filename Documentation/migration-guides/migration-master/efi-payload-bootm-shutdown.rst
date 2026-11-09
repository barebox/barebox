EFI applications booted via bootm no longer return to barebox
-------------------------------------------------------------

When barebox runs as EFI payload, :ref:`command_bootm` now shuts down
barebox before starting any EFI application, not only EFI-stubbed Linux
kernels. If the application returns, barebox exits to the firmware instead
of returning to the shell or trying the next boot entry.

EFI applications that are expected to return, like an EFI shell, should be
executed directly from the shell instead.
