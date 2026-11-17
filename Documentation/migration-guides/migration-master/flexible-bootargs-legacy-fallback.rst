Legacy bootargs variable ignored with CONFIG_FLEXIBLE_BOOTARGS
--------------------------------------------------------------

With ``CONFIG_FLEXIBLE_BOOTARGS`` enabled, the kernel command line used to
fall back to the legacy ``bootargs`` environment variable whenever the
concatenation of all ``global.linux.bootargs.*`` variables came out empty.
This fallback is gone, only the global variables are used now.

The fallback was already mostly unreachable: once a boot entry had run,
``global.linux.bootargs.dyn.ip`` and ``global.linux.bootargs.dyn.root`` were
registered and the separator between the two empty variables made the
concatenation non-empty.

Set ``global.linux.bootargs.base`` instead of ``bootargs``, or disable
``CONFIG_FLEXIBLE_BOOTARGS`` to keep using the legacy variable.

``global.linux.mtdparts.*`` and ``global.linux.blkdevparts.*`` are now
passed to the kernel even if no ``global.linux.bootargs.*`` variable has a
value. Previously, they were dropped along with taking the fallback.
