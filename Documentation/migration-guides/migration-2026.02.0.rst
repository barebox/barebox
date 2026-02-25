Release v2026.02.0
==================

CONFIG_SHELL_NONE
-----------------

If there's nothing to do for a shell-less barebox, it will now attempt
to poweroff the system instead of busy-looping indefinitely.
This changes behavior for systems that rely on a watchdog to reset
a hanging barebox in this situation. If this breaks anything for you,
please reach out.
