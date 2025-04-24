.. _versioning:

barebox Artifact Versioning
===========================

In addition to the usual barebox release (e.g. ``v2025.03.0``), the
version number can be extended to encode integration-specific version
information:

* When built from git, ``scripts/setlocalversion`` will factor in
  git revision information into the version string.
* The ``EXTRAVERSION =`` in the top-level ``Makefile`` can be used
  to add a suffix to the version. This is useful if patches are applied
  on top of the tarball release.
* The build host can set the ``BUILDSYSTEM_VERSION`` environment variable
  prior to executing ``make`` to encode a board support package version.
  This is useful to encode information about built-in environment
  and firmware.

Query from barebox
^^^^^^^^^^^^^^^^^^

When ``CONFIG_BANNER`` is enabled, the version information will be printed
to the console. From the shell, there is the
:ref:`version command <command_version>` for interactive use and the
``global.version`` and ``global.buildsystem.version`` :ref:`magicvars`
for use in scripts.

Query from OS
^^^^^^^^^^^^^

The barebox version (formatted as ``barebox-$version``) can be queried
after boot by different means:

* If the OS is booted with device tree, barebox will fixup a
  ``/chosen/barebox-version`` property into the kernel device tree with
  the version string. Under Linux, this can be accessed at:

 * ``/sys/firmware/devicetree/base/chosen/barebox-version``
 * ``/proc/device-tree/base/chosen/barebox-version``

* If the system is booted through barebox as EFI application (payload),
  a ``LoaderInfo`` EFI variable with the systemd vendor GUID will
  be set to the version string. Under Linux, the string is shown in
  ``bootctl`` output

Query without booting
^^^^^^^^^^^^^^^^^^^^^

If the barebox boot medium is known, ``bareboximd`` can be used
to read the barebox :ref:`imd`, provided that barebox was
compiled with ``CONFIG_IMD=y`` (and ``CONFIG_IMD_TARGET=y`` for
the target tool)::

  linux$ bareboximd /dev/mmc2.boot0 -t release
  2025.03.0-20250403-1

  barebox$ imd /dev/mmc2.boot0 -t release
  2025.03.0-20250403-1
