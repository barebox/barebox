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

Enabling remote control support
-------------------------------

To get remote control support barebox has to be compiled with
CONFIG_RATP and CONFIG_CONSOLE_RATP enabled. Optionally CONFIG_FS_RATP
can also be enabled.

Running the bbremote tool
-------------------------

The bbremote host tool is written in python. To run it python2 has to be
installed with the following additional packages:

+----------------+---------------------+
| python package | Debian package name |
+================+=====================+
| crcmod         | python-crcmod       |
+----------------+---------------------+
| enum           | python-enum         |
+----------------+---------------------+
| enum34         | python-enum34       |
+----------------+---------------------+

If your distribution does not provide aforementioned packages, you can
use 'pip' in order to install the dependencies localy to your user
account via:

.. code-block:: sh

  pip install --user crcmod enum enum34

configuring bbremote
^^^^^^^^^^^^^^^^^^^^

bbremote needs the port and possibly the baudrate to access the remote
barebox. The port can be configured with the ``--baudrate`` option or
with the ``BBREMOTE_PORT`` environment variable. The port can either be
the device special file if it's a local port or if it's a remote port a
string of the form: ``rfc2217://host:port``. The baudrate can be given
with the ``--baudrate`` option or the ``BBREMOTE_BAUDRATE`` environment
variable. For the rest of this document it is assumed that ``bbremote``
has been configured using environment variables.

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

