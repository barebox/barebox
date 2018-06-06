Default environment version 2
=============================

barebox stores its environment files under the top-level ``/env/``
directory, where most of the runtime configuration scripts are located.
This environment is comparable to a tar archive which is unpacked from
a storage medium during startup. If for whatever reason the environment
cannot be loaded from a storage medium, a compiled-in default environment
is used instead.

The environment is not automatically stored on the storage medium when a file
under ``/env/`` is changed; rather, this has to be done manually using the
:ref:`command_saveenv` command.

There are two sets of generic environment files which can be used. The older
version (version one) should not be used for new boards and is not described here
(even though there are still numerous board definitions that use it).
All new boards should use defaultenv-2 exclusively.

The default environment is composed from different directories during compilation::

  defaultenv/defaultenv-2-base   -> base files
  defaultenv/defaultenv-2-dfu    -> overlay for DFU
  defaultenv/defaultenv-2-menu   -> overlay for menus
  arch/$ARCH/boards/<board>/env  -> board specific overlay

The content of the above directories is applied one after another. If the
same file exists in a later overlay, it will overwrite the preceding one.

Note that not all of the above directories will necessarily be
included in your default environment, it depends on your barebox
configuration settings. You can see the configuration variables
and their respective included directories in ``defaultenv/Makefile``:

.. code-block:: make

  bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW) += defaultenv-2-base
  bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_MENU) += defaultenv-2-menu
  bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_DFU) += defaultenv-2-dfu
  bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC) += defaultenv-1

/env/bin/init
-------------

This script is executed by the barebox startup code after initialization.
In defaultenv-2, this script will define and set a number of global
variables, followed by sourcing all of the scripts in ``/env/init/`` with::

  for i in /env/init/*; do
          . $i
  done

This script is also responsible for defining the boot timeout value
(by default, three seconds), then printing the timeout prompt for the user.
Be careful making changes to this script: since it is executed before any user
intervention, it might lock the system.

/env/init/
----------

The ``/env/init/`` directory is the location for startup scripts. The scripts
in this directory will be executed in alphabetical order by the
``/env/bin/init`` script described earlier.

/env/boot/
----------

The ``/env/boot/`` directory contains boot entry scripts. The :ref:`command_boot`
command treats the files in this directory as possible boot targets.
See :ref:`booting_linux` for more details.

/env/config
-----------

This file contains some basic configuration settings. It can be edited using
the :ref:`command_edit` command. Typical content:

.. code-block:: sh

  #!/bin/sh

  # change network settings in /env/network/eth0
  # change mtd partition settings and automountpoints in /env/init/*

  #global.hostname=

  # set to false if you do not want to have colors
  #global.allow_color=true

  # user (used for network filenames)
  #global.user=none

  # timeout in seconds before the default boot entry is started
  #global.autoboot_timeout=3

  # key to abort autoboot. Supported options are: "any" and "ctrl-c"
  #global.autoboot_abort_key=any

  # list of boot entries. These are executed in order until one
  # succeeds. An entry can be:
  # - a filename in /env/boot/
  # - a full path to a directory. All files in this directory are
  #   treated as boot files and executed in alphabetical order
  #global.boot.default=net

  # base bootargs
  #global.linux.bootargs.base="console=ttyS0,115200"

When changing this file remember to do a ``saveenv`` to make the change
persistent. Also it may be necessary to manually ``source /env/config`` before
the changes take effect.

/env/network/
-------------

This contains the configuration files for the network interfaces. Typically
there will be a file ``eth0`` with a content like this:

.. code-block:: sh

  #!/bin/sh

  # ip setting (static/dhcp)
  ip=dhcp
  global.dhcp.vendor_id=barebox-${global.hostname}

  # static setup used if ip=static
  ipaddr=
  netmask=
  gateway=
  serverip=

  # MAC address if needed
  #ethaddr=xx:xx:xx:xx:xx:xx

  # put code to discover eth0 (i.e. 'usb') to /env/network/eth0-discover

  exit 0
