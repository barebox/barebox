.. _automount:

Automount
=========

barebox supports automatically mounting filesystems when a path is first
accessed. This is handled with the :ref:`command_automount` command. With automounting
it is possible to separate the configuration of a board from actually using
filesystems. The filesystems (and the devices they are on) are only probed
when necessary, so barebox does not lose time probing unused devices.

Typical usage is for accessing the TFTP server. To set up an automount for a
TFTP server, the following is required::

  mkdir -p /mnt/tftp
  automount /mnt/tftp 'ifup -a && mount -t tftp $global.net.server /mnt/tftp'

This creates an automountpoint on ``/mnt/tftp``. Whenever this directory is accessed,
the command ``ifup eth0 && mount -t tftp $eth0.serverip /mnt/tftp`` is executed.
It will bring up the network device using :ref:`command_ifup` and mount a TFTP filesystem
using :ref:`command_mount`.

Usually the above automount command is executed from an init script in ``/env/init/automount``.
With the above, files on the TFTP server can be accessed without configuration::

  cp /mnt/tftp/linuximage /image

This automatically detects a USB mass storage device and mounts the first
partition to ``/mnt/fat``::

  mkdir -p /mnt/fat
  automount -d /mnt/fat 'usb && [ -e /dev/disk0.0 ] && mount /dev/disk0.0 /mnt/fat'

To see a list of currently registered automountpoints use ``automount -l``.
