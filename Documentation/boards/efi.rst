barebox on (U)EFI
=================

barebox can be built as an EFI application for X86 PCs. This makes
barebox a bootloader running on PC type hardware. In EFI jargon barebox
would be a EFI shell. Due to the barebox :ref:`bootloader_spec` support
it can act as a replacement for gummiboot.

For accessing hardware the EFI drivers and abstractions are used. barebox
has several drivers which merely map to the underlying EFI layers. A plain
barebox binary provides access to the screen and keyboard. The EFI System
partition (:term:`ESP`) is available under ``/boot``, additional partitions may
be available as ``/efi*``. Networking may be available if the BIOS provides
the necessary drivers, but most likely you'll have to download/compile
network drivers yourself, see below.

Depending on the ``CONFIG_64BIT`` option either a ia32 binary or a x86_64
binary is built. Due to the lack of 32bit UEFI testing hardware only the
x86_64 binary currently is tested.

Building barebox for EFI
------------------------

Use the following to build barebox for EFI:

.. code-block:: sh

  export ARCH=x86
  make efi_defconfig
  make

The resulting EFI image is ``barebox.efi`` (or the barebox-flash-image link).

Running barebox on EFI systems
------------------------------

The simplest way to run barebox on a USB memory stick. (U)EFI only supports
FAT filesystems, so make sure you either have a FAT16 or FAT32 filesystem on
the memory stick. Put ``barebox.efi`` into the ``EFI/BOOT/`` directory and
name it ``BOOTx64.EFI`` on 64bit architectures and ``BOOTIA32.EFI`` on 32bit
architectures. Switching to USB boot in the BIOS should then be enough to
start barebox via USB. Some BIOSes allow to specify a path to a binary to
be executed, others have a "start UEFI shell" entry which executes
EFI/Shellx64.efi on the :term:`ESP`. This can be a barebox binary aswell.

Running EFI barebox on qemu
^^^^^^^^^^^^^^^^^^^^^^^^^^^

barebox can be started in qemu with OVMF http://www.linux-kvm.org/page/OVMF.

OVMF is part of several distributions and can be installed with the package
management system. qemu needs the OVMF.fd from the OVMF package file as
argument to the -pflash option. As qemu needs write access to that file it's
necessary to make a copy first.

To start it create a USB memory stick like above and execute:

.. code-block:: sh

  qemu-system-x86_64 -pflash OVMF.fd -nographic /dev/sdx

A plain VFAT image will work aswell, but in this case the UEFI BIOS won't
recognize it as ESP and ``/boot`` won't be mounted.

Loading EFI applications
------------------------

EFI supports loading applications aswell as drivers. barebox does not differentiate
between both. Both types can be simply executed by typing the path on the command
line. When an application/driver returns barebox iterates over the handle database
and will initialize all new devices.

Applications
^^^^^^^^^^^^

barebox itself and also the Linux Kernel are EFI applications. This means both
can be directly executed. On other architectures when barebox is executed from
another barebox it means the barebox binary will be replaced. EFI behaves
differently, here different barebox instances will be nested, so exiting barebox
means passing control to the calling instance. Note that currently the :ref:`command_reset`
command will pass the control to the calling instance rather than resetting
the CPU. This may change in the future.

Although the Linux Kernel can be directly executed one should use the :ref:`command_bootm`
command. Only the bootm command passes the Kernel commandline to the Kernel.

Drivers
^^^^^^^

EFI is modular and drivers can be loaded during runtime. Many drivers are
included in the BIOS already, but some have to be loaded during runtime,
for example it's common that network drivers are not included in the BIOS.

Drivers can be loaded under barebox simply by executing them:

.. code-block:: sh

  barebox:/ /boot/network-drivers/0001-SnpDxe.efi

Should the drivers instanciate new devices these are automatically registered
after the driver has been loaded.

Simple Network Protocol (SNP)
-----------------------------

The Simple Network Protocol provides a raw packet interface to the EFI
network drivers. Each device which supports SNP shows up as a regular
network device under barebox. To use SNP the BIOS must have the SNP
protocol and the network driver installed. For getting the SNP protocol
follow the instruction in :ref:`efi_building_edk2`. Network drivers for
the common Intel Network devices can be found here:

https://downloadcenter.intel.com/Detail_Desc.aspx?agr=Y&DwnldID=19186

Once instantiated the EFI drivers take some time to bring up the link, so
it's best to only load the network drivers when needed. This can be
archieved with the following script to put under ``/env/network/eth0-discover``:

.. code-block:: sh

  #!/bin/sh

  for i in /boot/network-drivers/*; do
          $i;
  done

This script will load the drivers in ``/boot/network-drivers/`` in alphabetical
order.

**NOTE** Loading the network drivers only works when loaded in the
correct order. First the SNP driver must be loaded and then the network device
driver. Otherwise the drivers will load without errors, but no devices will be
instantiated. For making the order sure the driver names can be prepended with
a number:

.. code-block:: sh

  /boot/network-drivers/0001-SnpDxe.efi
  /boot/network-drivers/0002-E6208X3.EFI

It is currently not known whether this is a limitation in EFI or a bug in
barebox.

EFI File IO Interface
---------------------

EFI itself has filesystem support. At least the :term:`ESP` will be mounted by the
EFI core already. The :term:`ESP` is mounted to ``/boot`` under barebox, other devices
are mounted to ``/efi<no>`` in no particular order.

Block IO Protocol
-----------------

EFI provides access to block devices with the Block IO Protocol. This can
be used to access raw block devices under barebox and also to access filesystems
not supported by EFI. The block devices will show up as ``/dev/disk<diskno>.<partno>``
under barebox and can be accessed like any other device:

.. code-block:: sh

  mount /dev/disk0.1 -text4 /mnt

Care must be taken that a partition is only accessed either via the Block IO Protocol *or*
the File IO Interface. Doing both at the same time will most likely result in data
corruption on the partition

EFI device paths
----------------

In EFI each device can be pointed to using a device path. Device paths have multiple
components. The toplevel component on X86 systems will be the PCI root complex, on
other systems this can be the physical memory space. Each component will now descrive
how to find the child component on the parent bus. Additional device path nodes can
describe network addresses or filenames on partitions. Device paths have a binary
representation and a clearly defined string representation. These characteristics make
device paths suitable for describing boot entries. barebox could use device paths
to store the reference to kernels on boot media. Also device paths could be used to
pass a root filesystem to the Kernel.

Currently device paths are only integrated into barebox in a way that each EFI device
has a device parameter ``devpath`` which contains its device path:

.. code-block:: sh

  barebox:/ echo ${handle-00000000d0012198.devpath}
  pci_root(0)/Pci(0x1d,0x0)/Usb(0x1,0x0)/Usb(0x2,0x0)


EFI variables
-------------

EFI has support for variables which are exported via the EFI Variable Services. EFI variables
are identified by a 64bit GUID and a name. EFI variables can have arbitrary binary values, so
they are not compatible with barebox shell variables which can only have printable content.
Support for these variables is not yet complete in barebox. barebox contains the efivarfs which
has the same format as the Linux Kernels efivarfs. It can be mounted with:

.. code-block:: sh

  mkdir efivarfs
  mount -tefivarfs none /efivarfs

In efivarfs each variable is represented by a file named <varname>-<guid>. Access to EFI variables
is currently readonly. Since the variables have binary content using :ref:`command_md` is often
more suitable than :ref:`command_cat`.

EFI driver model and barebox
----------------------------

The EFI driver model is based around handles and protocols. A handle is an opaque
cookie that represents a hardware device or a software object. Each handle can have
multiple protocols attached to it. A protocol is a callable interface and is defined
by a C struct containing function pointers. A protocol is identified by a 64bit GUID.
Common examples for protocols are DEVICE_PATH, DEVICE_IO, BLOCK_IO, DISK_IO,
FILE_SYSTEM, SIMPLE_INPUT or SIMPLE_TEXT_OUTPUT. Every handle that implements the
DEVICE_PATH protocol is registered as device in barebox. The structure can be best
seen in the ``devinfo`` output of such a device:

.. code-block:: sh

  barebox:/ devinfo handle-00000000cfaed198
  Driver: efi-snp
  Bus: efi
  Protocols:
    0: a19832b9-ac25-11d3-9a2d-0090273fc14d
    1: 330d4706-f2a0-4e4f-a369-b66fa8d54385
    2: e5dd1403-d622-c24e-8488-c71b17f5e802
    3: 34d59603-1428-4429-a414-e6b3b5fd7dc1
    4: 0e1ad94a-dcf4-11db-9705-00e08161165f
    5: 1aced566-76ed-4218-bc81-767f1f977a89
    6: e3161450-ad0f-11d9-9669-0800200c9a66
    7: 09576e91-6d3f-11d2-8e39-00a0c969723b
    8: 51dd8b21-ad8d-48e9-bc3f-24f46722c748
  Parameters:
    devpath: pci_root(0)/Pci(0x1c,0x3)/Pci(0x0,0x0)/Mac(e03f4914f157)

The protocols section in the output shows the different protocols this
handle implements. One of this Protocols (here the first) is the Simple
Network Protocol GUID:

.. code-block:: c

  #define EFI_SIMPLE_NETWORK_PROTOCOL_GUID \
    EFI_GUID( 0xA19832B9, 0xAC25, 0x11D3, 0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D )

Matching between EFI devices and drivers is done based on the Protocol GUIDs, so
whenever a driver GUID matches one of the GUIDs a device imeplements the drivers
probe function is called.

.. _efi_building_edk2:

Building EDK2
-------------

Additional drivers may be needed from the EDK2 package. For example to
use Networking in barebox not only the network device drivers are needed,
but also the Simple Network Protocol driver, SnpDxe.efi. This is often
not included in the BIOS, but can be compiled from the EDK2 package.

Here is only a quick walkthrough for building edk2, there are more elaborated
HOWTOs in the net, for example on http://tianocore.sourceforge.net/wiki/Using_EDK_II_with_Native_GCC.

.. code-block:: sh

  git clone git://github.com/tianocore/edk2.git
  cd edk2
  make -C BaseTools
  . edksetup.sh

At least the following lines in ``Conf/target.txt`` should be edited::

  ACTIVE_PLATFORM = MdeModulePkg/MdeModulePkg.dsc
  TARGET_ARCH = X64
  TOOL_CHAIN_TAG = GCC48
  MAX_CONCURRENT_THREAD_NUMBER = 4

The actual build is started with invoking ``build``. After building
``Build/MdeModule/DEBUG_GCC48/X64/SnpDxe.efi`` should exist.

**NOTE** As of this writing (July 2014) the following patch was needed to
compile EDK2.

.. code-block:: diff

  diff --git a/MdeModulePkg/Universal/DebugSupportDxe/X64/AsmFuncs.S b/MdeModulePkg/Universal/DebugSupportDxe/X64/AsmFuncs.S
  index 9783ec6..13fc06c 100644
  --- a/MdeModulePkg/Universal/DebugSupportDxe/X64/AsmFuncs.S
  +++ b/MdeModulePkg/Universal/DebugSupportDxe/X64/AsmFuncs.S
  @@ -280,7 +280,7 @@ ExtraPushDone:

                   mov     %ds, %rax
                   pushq   %rax
  -                movw    %es, %rax
  +                mov     %es, %rax^M
                   pushq   %rax
                   mov     %fs, %rax
                   pushq   %rax

