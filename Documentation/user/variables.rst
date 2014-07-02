Variables
=========

We already have seen some variables in :ref:`device_parameters`. But
there is more to variables.

.. _global_device:

The ``global`` device
---------------------

The ``global`` device is a special purpose device. It only exists as a
storage for global variables. Some global variables are created internally
in barebox (see :ref:`magicvars`). Additional variables can be created with
the :ref:`command_global` command::

  global myvar

This creates the ``global.myvar`` variable which now can be used like any
other variable. You can also directly assign a value during creation::

  global myvar1=foobar

**NOTE:** creating a variable with ``global myvar1=foobar`` looks very similar
to assigning a value with ``global.myvar1=foobar``, but the latter fails when
a variable has not been created before.

.. _magicvars:

Magic variables
---------------

Some variables have special meanings and influence the behaviour
of barebox. Most but not all of them are consolidated in the :ref:`global_device`.
Since it's hard to remember which variables these are and if the current
barebox has support for them the :ref:`command_magicvar` command can print a list
of all variables with special meaning along with a short description::

  barebox:/ magicvar
  OPTARG                           optarg for hush builtin getopt
  PATH                             colon separated list of pathes to search for executables
  PS1                              hush prompt
  armlinux_architecture            ARM machine ID
  armlinux_system_rev              ARM system revision
  armlinux_system_serial           ARM system serial
  automount_path                   mountpath passed to automount scripts
  bootargs                         Linux Kernel parameters
  bootsource                       The source barebox has been booted from
  bootsource_instance              The instance of the source barebox has been booted from
  global.boot.default              default boot order
  ...

