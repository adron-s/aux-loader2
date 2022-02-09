# aux-loader v2

This auxiliary bootloader implements the ability to run the OpenWrt Linux kernel(FIT image) from the RouterBOOT.
The thing is that RouterBOOT is able to load ONLY program code in ELF format. To bypass this restriction,
the auxiliary bootloader extends the capabilities of the RouterBOOT - by adding support for loading system from FIT images.

RouterBOOT is located at [ 0xb0000 ... 0xc0000 ]. In the end of this 64k block there is a small area of free space (~10Kb).
Exactly in this free space the auxiliary bootloader code is placed - in order to be in the same 64k block with the RouterBOOT.
Further, by means of binary patches, the calls of the auxiliary loader code are inserted into the decoded RouterBOOT.

Then it's all packed (using [Mikrotik RouterBOOT enc/dec](https://github.com/adron-s/mtik_routerboot_encdec.git)) and the output
is the ready-to-write MTD image with [ modified RouterBOOT + aux-loader ] and [ aux-loader ELF ] - for FIT images ELF wrap and TFTP boot

The auxiliary loader supports four types of launch:

	1 - start as RouterBOOT extension from boot-device: "f - boot Flash Configure Mode"
	  This boot-mode was chosen on purpose as it is not commonly used.
	  In this mode, the auxiliary loader starts the previously writed FIT image from
	    the NOR flash drive (from offset 0x100000 - 1Mb)

	2 - start as RouterBOOT extension, when the RouterBOOT is given the FIT image via TFTP.
	  The RouterBOOT(seeing that this is not ELF) transfers control to the auxiliary loader code.

	3 - start as the separate ELF image via RouterBOOT TFTP.
	  In this mode, the auxiliary loader starts from an ELF image and looks for the FIT image
	    right after its end. That is, to add an auxiliary loader FIT image to the ELF image,
	    you need to do the following command:

	  cat ./aux-loader.elf ./openwrt-mvebu-cortexa72-mikrotik_rb5009-initramfs-fit-uImage.itb > \
	    openwrt-mvebu-cortexa72-mikrotik_rb5009-initramfs-fit-uImage.elf

	4 - Start once via TFTP and next time use boot-device: "f - boot Flash Configure Mode"
	      This is done in point "3 - boot Flash Configure Mode once, then NAND"

So, to expand the functions of the RouterBOOT, we need to somehow write 
[ modified RouterBOOT + aux-loader ](https://github.com/adron-s/aux-loader2/raw/main/releases/2.00-latest/rbt-with-aux-for-mtd5.bin)
to the MTD partition of the target device.

The easiest way to achieve this is to use [ aux-loader.elf ](https://github.com/adron-s/aux-loader2/raw/main/releases/2.00-latest/aux-loader.elf)
and add OpenWrt FIT image(openwrt-mvebu-cortexa72-mikrotik_rb5009-initramfs-fit-uImage.itb) to its end. Then boot the target device from it(via TFTP).
using the following RouterOS commands:

```
/system/routerboard/settings/set boot-device=ethernet
/system/reboot
```

Then write [ rbt-with-aux-for-mtd5.bin ](https://github.com/adron-s/aux-loader2/raw/main/releases/2.00-latest/rbt-with-aux-for-mtd5.bin)
to RouterBOOT mtd with the following command:

```
wget https://github.com/adron-s/aux-loader2/raw/main/releases/2.00-latest/rbt-with-aux-for-mtd5.bin \
  -O- | mtd write - RouterBOOT
```

This command updates only the main RouterBOOT. Even if something goes wrong, you will always have the backup RouterBOOT
(which can be easily activated by pressing and holding the reset button until power is supplied to the device).
Also, the modified RouterBOOT still supports all its previous functions, so that it can run both: OpenWrt and RouterOS
without any problems. Moreover, in order to return the original RouterBOOT, it is enough to press the update
bootloader button in the RouterOS.

As mentioned above - the auxiliary loader can launch a previously writed FIT images(or ELF images) from a NOR flash drive.
To do this, the boot-device must be selected as "f - boot Flash Configure Mode":

  RouterOS:

```
/system/routerboard/settings/set boot-device=flash-boot
```

  OpenWrt(before doing sysupgrade from initramfs):

```
echo cfg > /sys/firmware/mikrotik/soft_config/boot_device
echo 1 > /sys/firmware/mikrotik/soft_config/commit 
```

To reboot back to the RouterOS(dual-boot mode), you can use the following commands:

```
echo flash > /sys/firmware/mikrotik/soft_config/boot_device
echo 1 > /sys/firmware/mikrotik/soft_config/commit
reboot
```

[ Here is a demo video ](https://www.youtube.com/watch?v=MrPhoWplKxw) showing the process of modifying
RouterBOOT, OpenWrt firmware install and dual boot.

This version of auxiliary loader is only supports RB5009 and RouterBOOT 7.2-rc1, but it can be easily
extended to other platforms and versions. To do this, the cpu.c file must be supplemented with the
appropriate platform-dependent code. Also in the routerboot.h file, you must specify the new addresses
to call the RouterBOOT functions. The function addresses can be determined by examining the RouterBOOT
for the target platform in [ Ghidra ](https://ghidra-sre.org).
