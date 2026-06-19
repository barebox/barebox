Release v2026.06.0
==================

global.linux.bootargs.* appending order
---------------------------------------

If barebox was configured to automatically generate any of the ``root``,
``rootwait``, ``earlycon``, ``systemd.machine_id``, ``systemd.hostname``
or ``barebox.security.policy`` kernel command line options, they will be
appended onto the final kernel command line
:ref:`**after** all other options <bootargs_concat_order>`.

Removal of global.env.autoprobe
-------------------------------

The global.env.autoprobe variable introduced with v2025.02.0 is removed and
now replaced with CONFIG_ENV_HANDLING_AUTOPROBE. It has never worked. If you
want to load a barebox environment based on its partition UUID enable
CONFIG_ENV_HANDLING_AUTOPROBE.