Sandbox
=======

barebox can be run as a simulator on your host to check and debug new non
hardware related features.

Building barebox for simulation
-------------------------------

The barebox sandbox can be built with the host compiler:

.. code-block:: sh

  ARCH=sandbox make sandbox_defconfig
  ARCH=sandbox make

Running the sandbox
-------------------

Once you compile barebox for the sandbox, you can run it with:

.. code-block:: console

  $ barebox [<OPTIONS>]

Available sandbox invocation options include:

  ``-m``, ``--malloc=<size>``

    Start sandbox with a specified malloc-space <size> in bytes.

  ``-i <file>``

    Map a <file> to barebox. This option can be given multiple times. The <file>s
    will show up as ``/dev/fd0`` ... ``/dev/fdX`` in the barebox simulator.

    How the file is mapped in barebox can be controlled by a number of flags:

    * ``,ro``: The host file is mapped read-only

    * ``,blkdev``: The host file is to be mapped as block device. This is the
      default when passing block devices from the host. The file's size must
      be a multiple of the barebox sector size of 512 bytes.

    * ``,cdev``: The host file is mapped as character device. This is the default,
      unless the the host file is a block device.

    Multiple options can be appended if they don't clash. Literal commas within the
    file path can be escaped with a backslash. Example: ``-i './0\,0.hdimg,blkdev,ro'``.

  ``-e <file>``

    Map <file> to barebox. With this option <file>s are mapped as
    ``/dev/env0`` ...  ``/dev/envX`` and thus are used as default environment.
    A clean file generated with ``dd`` will do to get started with an empty environment.

  ``-O <file>``

    Register <file> as a console capable of doing stdout. <file> can be a
    regular file or a FIFO.

  ``-I <file>``

    Register <file> as a console capable of doing stdin. <file> can be a regular
    file or a FIFO.

  ``-x``, ``--xres <res>``

    Specify SDL width.

  ``-y``, ``--yres <res>``

    Specify SDL height.

To terminate barebox and return to the calling shell, the poweroff command is
suitable.
