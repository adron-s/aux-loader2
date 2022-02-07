# aux-loader v2

This auxiliary bootloader implements the ability to run a OpenWRT Linux kernel(FIT image) from RouterBOOT.
The thing is that a RouterBOOT is able to load ONLY programs in ELF format.

The current version is only supports RB5009 and RouterBOOT 7.2-rc1, but it can be easily extended to other platforms and versions.

RouterBOOT is located at [ 0xb0000 ... 0xc0000 ]. In the end of this 64k block there is a small area of free space (~10Kb).
Exactly in this free space the aux-loader code is placed - in order to be in the same 64k block with the RouterBOOT.
Further, by means of binary patches, the calls of the aux-loader code are inserted into the decoded RouterBOOT.

Then it's all packed (using [Mikrotik RouterBOOT enc/dec](https://github.com/adron-s/mtik_routerboot_encdec.git)) and the output
is a ready-to-write MTD image with [ modified RouterBOOT + aux-loader ]

Then we only have to somehow write once the resulting file to the MTD partition of the target device.

The easiest way to achieve this is to use a [ specially prepared U-Boot](https://github.com/adron-s/aux-loader2/raw/main/releases/2.00-20220207/for-write-to-mtd.elf)
and boot the target device from it(via TFTP). The file [ rbt-with-aux-for-mtd5.bin ](https://github.com/adron-s/aux-loader2/raw/main/releases/2.00-20220207/rbt-with-aux-for-mtd5.bin)
must also be available for TFTP download(placed at the TFTP root). After starting the U-Boot, the download of file rbt-with-aux-for-mtd5.bin
will automatically begin and its writing in the corresponding mtd section. At the end of all these actions, the target device will
start blinking blue LEDs rhythmically. After that, the device can be turned off and further images in the FIT format can be loaded
to it via TFTP.

Also, this auxiliary loader can launch a previously writed FIT images(or ELF images) from a NOR flash drive.
To do this, the boot device must be selected as:

	3 - boot Flash Configure Mode once, then NAND.
	/system/routerboard/settings/set boot-device=flash-boot-once-then-nand

This rarely used mode was specifically chosen to pass control to the aux-loader code.
