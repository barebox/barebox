Boot related globalvars are reverted after booting
--------------------------------------------------

``global.bootm.*``, ``global.linux.*``, ``global.of.*`` and
``global.boot.watchdog_timeout`` are now snapshotted before each boot
entry and each bootm and restored afterwards, so changes made while
booting no longer leak into subsequent boot attempts.

Boot entries therefore no longer need to put Kernel command line options
into ``global.linux.bootargs.dyn.*``. The boot scripts shipped with the
default environment have dropped the ``.dyn`` infix.

``global.linux.bootargs.dyn.*`` variables are no longer cleared after a
boot entry returns, but restored to their previous values. Values set
in this namespace before calling ``boot`` are thus kept and passed to
all boot entries instead of only the first one. Settings meant for all
boot entries should be put into a different variable, e.g.
``global.linux.bootargs.base``, as ``.dyn`` no longer has a special
meaning.
