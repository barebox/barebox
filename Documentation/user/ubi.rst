UBI/UBIFS support
=================

barebox has both UBI and UBIFS support. For handling UBI barebox has commands similar to
the Linux commands :ref:`command_ubiformat`, :ref:`command_ubiattach`, :ref:`command_ubidetach`,
:ref:`command_ubimkvol` and :ref:`command_ubirmvol`.

The following examples assume we work on the first UBI device. Replace ``ubi0`` with
the appropriate number when you have multiple UBI devices.

The first step for preparing a pristine Flash for UBI is to :ref:`command_ubiformat` the
device:

.. code-block:: sh

  ubiformat /dev/nand0.root

If you intend to use a device with UBI you should always use ``ubiformat`` instead of plain
:ref:`command_erase`. ``ubiformat`` will make sure the erasecounters are preserved and also
:ref:`ubi_fastmap` won't work when a flash is erased with ``erase``

**NOTE:** when using the :ref:`ubi_fastmap` feature make sure that the UBI is attached and detached
once after using ``ubiformat``. This makes sure the Fastmap is written.

After a device has been formatted it can be attached with :ref:`command_ubiattach`.

.. code-block:: sh

  ubiattach /dev/nand0.root

This will create the controlling node ``/dev/ubi0`` and also register all volumes present
on the device as ``/dev/ubi0.<volname>``. When freshly formatted there won't be any volumes
present. A volume can be created with:

.. code-block:: sh

  ubimkvol /dev/ubi0 root 0

The first parameter is the controlling node. The second parameter is the name of the volume.
In this case the volume can be found under ``/dev/ubi.root``. The third parameter contains
the size. A size of zero means that all available space shall be used.

The next step is to write a UBIFS image to the volume. The image must be created on a host using
the ``mkfs.ubifs`` command. ``mkfs.ubifs`` requires several arguments for describing the
flash layout. Values for these arguments can be retrieved from a ``devinfo ubi`` under barebox:

.. code-block:: sh

  barebox@Phytec pcm970:/ devinfo ubi0
  Parameters:
    peb_size: 16384
    leb_size: 15360
    vid_header_offset: 512
    min_io_size: 512
    sub_page_size: 512
    good_peb_count: 3796
    bad_peb_count: 4
    max_erase_counter: 0
    mean_erase_counter: 0
    available_pebs: 3713
    reserved_pebs: 83

To build a UBIFS image for this device the following command is suitable:

.. code-block:: sh

  mkfs.ubifs --min-io-size=512 --leb-size=15360 --max-leb-cnt=4096 -r rootdir \
	/tftpboot/root.ubifs

The ``--max-leb-cnt`` parameter specifies the maximum number of logical erase blocks
the UBIFS image can ever have. For this particular device a number of 3713 would be
enough. If the image shall be used for multiple boards the maximim peb count of all
boards must be used.

The UBIFS image can be transferred to the board for example with TFTP:

.. code-block:: sh

  cp /mnt/tftp/root.ubifs /dev/ubi0.root

Finally it can be mounted using the :ref:`command_mount` command:

.. code-block:: sh

  mkdir -p /mnt/ubi
  mount -t ubifs /dev/ubi0.root /mnt/ubi

The second time the UBIFS is mounted the above can be simplified to:

.. code-block:: sh

  ubiattach /dev/nand0.root
  mount -t ubifs /dev/ubi0.root /mnt/ubi

Mounting the UBIFS can also be made transparent with the automount command.
With this helper script in ``/env/bin/automount-ubi:``:

.. code-block:: sh

  #!/bin/sh

  if [ ! -e /dev/ubi0 ]; then
	ubiattach /dev/nand0 || exit 1
  fi

  mount -t ubifs /dev/ubi0.root $automount_path


The command ``automount -d /mnt/ubi/ '/env/bin/automount-ubi'`` will automatically
attach the UBI device and mount the UBIFS image to ``/mnt/ubi`` whenever ``/mnt/ubi``
is first accessed. The automount command can be added to ``/env/init/automount`` to
execute it during startup.

.. _ubi_fastmap:

UBI Fastmap
-----------

When attaching UBI to a flash device the UBI code has to scan all eraseblocks on the
flash. Since this can take some time the Fastmap feature has been introduced. It has
been merged in Linux 3.7. barebox has support for the Fastmap feature, but to use
it some care must be taken. The Fastmap feature reduces scanning time by adding
informations to one of the first blocks of a flash. For technical details see
http://www.linux-mtd.infradead.org/doc/ubi.html#L_fastmap. Since the Fastmap can
only live near the beginning of a flash the Fastmap code relies on finding a free
eraseblock there. The above example command make that sure, but Fastmap is incompatible
with creating a UBI image on a host and directly flashing the UBI image to the
raw NAND/NOR device. In this case the Fastmap code will not find a free eraseblock
and the following message will occur during ``ubidetach``:

.. code-block:: sh

  UBI error: ubi_update_fastmap: could not find any anchor PEB
  UBI warning: ubi_update_fastmap: Unable to write new fastmap, err=-28

The Fastmap is first written after a ``ubidetach``, so it's important to attach/detach
a UBI volume after using ``ubiformat``.

