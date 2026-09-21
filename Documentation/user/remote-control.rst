barebox remote control
======================

barebox remote control is for controlling barebox from a remote host via
scripts. The barebox console is designed for human interaction,
controlling it from a script is very error prone since UARTs do not
offer reliable communication. Usually a tool like 'expect' is used for
this purpose which uses its own language to communicate with the remote
partner. The barebox remote control offers an alternative. barebox
commands can be integrated into regular shell scripts running on the
host:

.. code-block:: sh

  bbremote --port /dev/ttyUSB0 run "ls"

Additionally files can be transferred from/to barebox and a regular
console offers interactive access to barebox on flawy serial
connections.

In addition to the bbremote tool provided with barebox, other third
party tools exist that use the same RATP based protocol to communicate
with the barebox instance, e.g. the libratp-barebox C library and the
ratp-barebox-cli tool:

  https://github.com/aleksander0m/libratp-barebox

Enabling remote control support
-------------------------------

To get remote control support barebox has to be compiled with
CONFIG_RATP and CONFIG_CONSOLE_RATP enabled. Optionally CONFIG_FS_RATP
can also be enabled for file transfers and CONFIG_RATP_CMD_GPIO and
CONFIG_RATP_CMD_I2C for the GPIO and I2C subcommands.

barebox switches into RATP mode by itself when the shell reads the
start of a RATP packet, so nothing has to be prepared on the target.
When bbremote closes the connection, the console returns to normal.

Running the bbremote tool
-------------------------

The bbremote host tool is written in python. To run it python3 has to be
installed with the following additional packages:

+----------------+---------------------+
| python package | Debian package name |
+================+=====================+
| crcmod         | python3-crcmod      |
+----------------+---------------------+
| pyserial       | python3-serial      |
+----------------+---------------------+

If your distribution does not provide aforementioned packages, you can
use 'pip' in order to install the dependencies localy to your user
account via:

.. code-block:: sh

  python -m pip install --user crcmod pyserial

configuring bbremote
^^^^^^^^^^^^^^^^^^^^

bbremote needs the port and possibly the baudrate to access the remote
barebox. The port can be configured with the ``--port`` option or with
the ``BBREMOTE_PORT`` environment variable. The port can either be the
device special file if it's a local port or if it's a remote port a
string of the form: ``rfc2217://host:port``. The baudrate can be given
with the ``--baudrate`` option or the ``BBREMOTE_BAUDRATE`` environment
variable. For the rest of this document it is assumed that ``bbremote``
has been configured using environment variables.

Every invocation opens its own RATP connection and closes it again when
it is done. The connection a killed bbremote leaves behind can make the
next one fail while it resets the stale link, so scripts are better off
passing ``--wait``, which retries until the target answers.

running commands on the target
------------------------------

``bbremote`` can be used to run arbitrary commands on the remote
barebox:

.. code-block:: sh

  bbremote run "echo huhu"
  huhu

The bbremote exit status will be 0 if the remote command exited
successfully, 1 if the remote command failed and 127 if there was a
communication error.

**NOTE** It is possible to put the output into a shell variable for
further processing, like ``RESULT=$(bbremote run "echo huhu")``.
However, this string may contain unexpected messages from drivers and
the like because currently we cannot filter out driver messages and
messages to stderr.

ping
----

This is a simple ping test.

.. code-block:: sh

  bbremote ping
  pong

getenv
------

.. code-block:: sh

  bbremote getenv global.version
  2015.12.0-00150-g81cd49f

interactive console
-------------------

The bbremote tool also offers a regular interactive console to barebox.
This is especially useful for flawy serial connections.

.. code-block:: sh

  bbremote console
  barebox@Phytec phyFLEX-i.MX6 Quad Carrier-Board:/ ls
  .      ..     dev    env    mnt

**NOTE** To terminate resulting Barebox console session press 'Ctrl-T'

**NOTE** You can also send 'ping' request to the target without
closing console session by pressint 'Ctrl-P'

transferring files
------------------

With the bbremote tool it's possible to transfer files both from the
host to barebox and from barebox to the host. Using the ``--export``
option to bbremote a directory can be specified to export to barebox.
This can be mounted on barebox using the regular mount command using
``-t ratpfs`` as filesystem type.

.. code-block:: sh

  bbremote --export=somedir console
  mkdir -p /ratpfs; mount -t ratpfs none /ratpfs
  ls /ratpfs

``--export`` works with ``run`` as well, which is the easier way to
script a transfer:

.. code-block:: sh

  bbremote --export=somedir run "mount -t ratpfs none /ratpfs; cp /ratpfs/zImage /tmp/"

The filesystem is served by the bbremote process, so the mount only
lives as long as that invocation. The target may write to it as well,
which is how files are copied back to the host.

reading and writing memory
--------------------------

``md`` and ``mw`` read and write a file or device on the target without
going through the shell. Both take the path, the offset within it and
the size or the data to write, the latter as a hex string:

.. code-block:: sh

  bbremote md /dev/mem 0x100 16
  00000000000000000000000000000000
  bbremote mw /dev/mem 0x100 deadbeef
  4 bytes written

The offset is transferred in 16 bits, so only the first 64 KiB of a
file are reachable this way.

GPIOs and I2C
-------------

With CONFIG_RATP_CMD_GPIO and CONFIG_RATP_CMD_I2C barebox serves GPIO
and I2C requests as well. The GPIO number is the one ``gpioinfo``
prints, the direction is 0 for input and 1 for output:

.. code-block:: sh

  bbremote gpio-set-direction 499 1 1
  bbremote gpio-get-value 499
  1
  bbremote gpio-set-value 499 0

The I2C subcommands take bus, device address, register, flags and the
size to read or the data to write. Bit 0 of the flags selects a 16 bit
register address, bit 1 selects a plain master transfer without a
register address:

.. code-block:: sh

  bbremote i2c-read 0 0x50 0x10 0 4
  bbremote i2c-write 0 0x50 0x10 0 affe

resetting the target
--------------------

.. code-block:: sh

  bbremote reset

This is the equivalent of the ``reset`` command: barebox shuts down
cleanly and restarts. ``--force`` skips the shutdown and restarts right
away.
