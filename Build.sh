#!/bin/sh

OPENWRT_DIR=/home/prog/openwrt/2021-openwrt/last-openwrt/openwrt

#CPU Marvell Armada-7040(RB5009)
CPU_TYPE=MVEBU
SPI_NOR_BASE=0x1c00000

TEXT_BASE=0x1cbd800 #if we start from NOR flash and locate at it offset: 0xbd800

RES_BIN_OFFSET=$((TEXT_BASE-SPI_NOR_BASE))

export STAGING_DIR=${OPENWRT_DIR}/staging_dir
export STAGING_DIR_HOST=${STAGING_DIR}/host
export CROSS_COMPILE=aarch64-openwrt-linux-
export TOOLPATH=${STAGING_DIR}/toolchain-aarch64_cortex-a72_gcc-11.2.0_musl

export PATH=${TOOLPATH}/bin:${PATH}

CC=${CROSS_COMPILE}gcc
LD=${CROSS_COMPILE}ld
OBJDUMP="${CROSS_COMPILE}objdump"
OBJCOPY="${CROSS_COMPILE}objcopy"

CFLAGS="-Wall -fno-stack-protector -fno-builtin -O2"
CFLAGS2="-I ../include"
CFLAGS="${CFLAGS} ${CFLAGS2}"
#ASFLAGS="${CFLAGS} -D__ASSEMBLY__"

cd ./objs
$CC $CFLAGS ../start.c -c
$CC $CFLAGS ../main.c -c
$CC $CFLAGS ../cpu.c -c
$CC $CFLAGS ../fdt.c -c
$CC $CFLAGS ../lzma.c -c
$CC $CFLAGS ../LzmaDecode.c -c

#main.o must be go first !
$LD -Ttext ${TEXT_BASE} -T ../loader.lds -Map loader.map -e _elf_start \
	start.o main.o cpu.o fdt.o lzma.o LzmaDecode.o -o ../loader.elf
cd ../
$OBJCOPY -j .text -j .rodata -O binary loader.elf loader.bin
#$OBJCOPY -j .data -O binary loader.elf data.bin
#$OBJCOPY -j .text -O binary loader.elf loader.bin

#$OBJDUMP -D loader.elf
#$OBJDUMP -x ./loader.elf

#exit 0

#check for size fit
true && {
	MAX_SIZE=10240
	SIZE=$(du -b ./loader.bin | sed 's/\t.*//')
	[ ${SIZE} -gt ${MAX_SIZE} ] && {
		echo "\nWARNING: !!! res.bin size is out of RANGE! ${SIZE} > ${MAX_SIZE} !!!\n"
	}
}
