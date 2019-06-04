.. index:: ubootvarfs (filesystem)

.. _filesystems_ubootvarfs:

U-Boot environment filesystem
=============================

barebox supports accessing U-Boot environment contents as a regular
filesystems in both read and write modes.  U-Boot environment data
(ubootvar) device supports automount, so no explicit mount command
should be necessary and accessing the environment should be as easy
as:

.. code-block:: console

  barebox:/ ls -l /mnt/ubootvar0

However the filesystem can be explicitly mounted with the following
command:

.. code-block:: console

  barebox:/ mount -t ubootvarfs /dev/device /mnt/path

**NOTE** Current implementation of the filesystem driver uses lazy
synchronization, any changes made to the environment will not be
written to the medium until the filesystem is unmounted (will happen
automatically on Barebox shutdown)
