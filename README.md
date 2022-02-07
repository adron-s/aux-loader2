# aux-loader v2

This auxiliary bootloader was created to implement the ability to run a OpenWRT Linux kernel(from FIT images) for RouterBOOT.
The thing is that a RouterBOOT is able to load ONLY programs in ELF format.

The current implementation only supports RB5009 and RouterBOOT 7.2-rc1, but can be easily extended to other platforms and versions.

RouterBOOT is located at [ 0xb000 ... 0xc000 ]. In the end of this block there is a small area of free space (~10Kb).
Exactly in this space the aux-loader code is placed - in order to be in the same 64k block with the RouterBOOT.
Further, by means of binary patches, calls of the aux-loader code are inserted into the decoded RouterBOOT.

Then it's all packed (using [Mikrotik RouterBOOT enc/dec](https://github.com/adron-s/mtik_routerboot_encdec.git)) and the output
is a ready-to-write MTD image with [ modified RouterBOOT + aux-loader ]

Then we only have to somehow write once the resulting file to the MTD partition of the target device.

The easiest way to achieve this is to use a [ specially prepared U-Boot](./releases/2.00-20220207/for-write-to-mtd.elf) and boot
the target device from it(via TFTP). The file rbt-with-aux-for-mtd5.bin must also be available for TFTP download(placed at the TFTP root).
After starting the U-Boot, the download of file rbt-with-aux-for-mtd5.bin will automatically begin and its writing in the corresponding mtd
section. At the end of all these actions, the target device will start blinking blue LEDs rhythmically. After that, the device can be
turned off and further images in the FIT format can be loaded to it via TFTP. Also, this auxiliary loader can launch a previously
recorded FIT image(or ELF image) from a NOR flash drive.
