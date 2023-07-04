Talks and Lectures
==================

This is a collection of talks held about barebox use and development
at different technical conferences. The most recent overview talk
is from 2020:

Beyond 'Just' Booting: Barebox Bells and Whistles
-------------------------------------------------

Ahmad Fatoum, Embedded Linux Conference - Europe 2020
`[slides] <https://elinux.org/images/9/9d/Barebox-bells-n-whistles.pdf>`__
`[video] <https://www.youtube.com/watch?v=fru1n54s2W4>`__

   Porting barebox to a new STM32MP1 board and a general discussion
   of design choices like multi-image, VFS, POSIX/Linux API,
   fail-safe updates, boot fall-back mechanisms, etc.

Besides older overview talks, there's a number of talks held
about different aspects of barebox use.
These are listed here in reverse chronological order.

DOOM portieren für Einsteiger - Heavy Metal auf Bare Metal (German)
-------------------------------------------------------------------

Ahmad Fatoum, FrOSCon 2021
`[slides] <https://programm.froscon.de/2021/system/event_attachments/attachments/000/000/622/original/heavy-metal-on-bare-metal.pdf>`__
`[video] <https://media.ccc.de/v/froscon2021-2687-doom_portieren_fur_einsteiger>`__

  "DOOM as a boot splash. How, why and how to get it on your nearest
  home appliance". A (German) walkthrough on how to leverage barebox
  APIs to run DOOM on any hardware supported by barebox.

Initializing RISC-V: A Guided Tour for ARM Developers
-----------------------------------------------------

Rouven Czerwinski & Ahmad Fatoum, Embedded Linux Conference 2021
`[slides] <https://elinux.org/images/8/80/Initializing-riscv.pdf>`__
`[video] <https://www.youtube.com/watch?v=70oYYuflFLs>`__

  A guide through the RISC-V architecture and some of its ISA extensions
  and a walkthrough of the barebox port to the Beagle-V Starlight.

From Reset Vector to Kernel - Navigating the ARM Matryoshka
-----------------------------------------------------------

Ahmad Fatoum, FOSDEM 2021
`[slides & video] <https://archive.fosdem.org/2021/schedule/event/from_reset_vector_to_kernel/>`__

  A walkthrough of NXP i.MX8M bootstrap. From Boot ROM through barebox to Linux.

Booting your i.MX processor secure and implementing i.MX8 secure boot in barebox
--------------------------------------------------------------------------------

Rouven Czerwinski, `Stratum 0 Talk 2019 <https://stratum0.org/wiki/Vortr%C3%A4ge/Vorbei#2019>`__
`[video] <https://www.youtube.com/watch?v=ZUGLEulZLWM>`__

  A walkthrough of NXP i.MX8MQ high assurance boot with barebox.

Porting Barebox to the Digi CC-MX6UL SBC Pro (German)
-----------------------------------------------------

Rouven Czerwinski, `Stratum 0 Live-Hacking 2019 <https://stratum0.org/wiki/Vortr%C3%A4ge/Vorbei#2019>`__
`[video] <https://www.youtube.com/watch?v=FIwF6GfmsWM>`__

  Live-coding a barebox port to a new i.MX6UL board while
  explaining the details (in German).

Remote update adventures with RAUC, Yocto and Barebox
-----------------------------------------------------

Patrick Boettcher, `Embedded Recipes 2019 <https://embedded-recipes.org/2019/remote-update-adventures-with-rauc-yocto-and-barebox/>`__
`[video] <https://www.youtube.com/watch?v=hS3Fjf7fuHM>`__

  Remote update and redundant boot of Embedded Linux devices
  in the field with RAUC and barebox bootchooser.

Verified Boot: From ROM to Userspace
------------------------------------

Marc Kleine-Budde, Embedded Linux Conference - Europe 2016
`[slides] <https://elinux.org/images/f/f8/Verified_Boot.pdf>`__
`[video] <https://www.youtube.com/watch?v=lkFKtCh2SaU>`__

  Using FOSS components, including barebox, for a cryptographically
  secured boot chain on NXP i.MX6 SoCs.

Booting Linux Made Easy: A Barebox Update
-----------------------------------------

Robert Schwebel, `FOSDEM 2014 <https://archive.fosdem.org/2014/schedule/event/_booting_linux_made_easy:_a_barebox_update/>`__
`[video] <https://www.youtube.com/watch?v=p-mHAQaJQcM>`__

  An overview talk on barebox use in embedded Linux systems.

Barebox and Bootloader Specification
------------------------------------

Sascha Hauer, Embedded Linux Conference - Europe 2013
`[slides] <https://elinux.org/images/9/90/Barebox-elce2013-bootloaderspec.pdf>`__
`[video] <https://www.youtube.com/watch?v=Z8FcIGXox_Y>`__

  The bootloader specification and its support in barebox.

Barebox Bootloader
------------------

Sascha Hauer, Embedded Linux Conference - Europe 2012
`[slides] <https://elinux.org/images/6/6b/PRE-20121108-1-Barebox.pdf>`__
`[video] <https://www.youtube.com/watch?v=oY8BjCEt_p8>`__

  An update on barebox development progress with a discussion of newly
  implemented features in the preceding three years.

Barebox: Booting Linux Fast and Fancy
-------------------------------------

Robert Schwebel & Sascha Hauer, Embedded Linux Conference - Europe 2010
`[slides] <https://elinux.org/images/8/89/ELCE-2010-Barebox-Booting-Linux-Fast-and-Fancy.pdf>`__

  Boot time optimization while using barebox.

U-Boot-v2
---------

Sascha Hauer & Marc Kleine-Budde, Embedded Linux Conference 2009
`[slides] <https://elinux.org/images/9/90/Hauer-U_BootV2.pdf>`__

  Early barebox (still named U-Boot v2 back then) presentation on
  the motivation for the fork and the niceties made possible by it.
