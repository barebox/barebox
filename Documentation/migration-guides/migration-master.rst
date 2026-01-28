:orphan:

ARM NXP i.MX8MP
---------------

On NXP i.MX8MP the SoC UID was read out wrong. It really is 128bit from which
barebox only read 64bit. barebox now does it correctly, but rolled out devices
might depend on the SoC UID being constant. In that case
CONFIG_ARCH_IMX8MP_KEEP_COMPATIBLE_SOC_UID should be enabled.
