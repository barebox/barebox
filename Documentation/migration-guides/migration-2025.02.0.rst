Release v2025.02.0
==================

Environment Auto-Probe
----------------------

A new magic variable :ref:`global.env.autoprobe <magicvar_global_env_autoprobe>`
now controls whether barebox
will automatically probe known block devices for a GPT barebox environment
partition if no environment was specified by other means.

The variable defaults to ``0`` (false) if ``CONFIG_INSECURE`` is disabled.

The most secure option remains disabling ``CONFIG_ENV_HANDLING`` completely
or, starting with v2025.10.0, using the security policy support for runtime
access control.

Zynq 7000
---------

The MMC controller numbering is now fixed: ``mmc0`` & ``mmc1`` for
``&sdhci0`` and ``&sdhci1`` respectively.
