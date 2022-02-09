#!/bin/sh
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation.
#

CUR_DIR=$(pwd)

. ./Build.sh

RBT_ENCDEC_DIR=/home/prog/openwrt/work/RB5009/dumps/fwf
TARGET="${CUR_DIR}/bins/rbt-MODI.dec"
SOURCE="./bins/rbt-7.2rc1-MTD.dec"

cd ${RBT_ENCDEC_DIR}
cat ${SOURCE} > ${TARGET}

. ./func.sh

#lets patch the RouterBOOT

#check_ELF_and_load_kernel(): if not_an_elf_header => goto our_code(0xBA02)
binay_patch 0xD63C "40 40 97 d2  70 c0 32 94"
#change default boot-device to 0x07(Flash Configure Mode)
binay_patch 0x10924 "07 00 00 00"
#change the behavior of two boot-device menu items: 7(Flash Configure Mode)
#and 8(Flash Configure Mode once)
#load_kernel_by_tag_0x03_way(): case 8 patch(Flash Configure Mode once)
binay_patch 0xF12B "04" #switchD_010062a8 offsets: extend case 8 block size(+4 bytes)
binay_patch 0x62AC "e1 00 80 52" #change once next value: 0x01->0x07
binay_patch 0x62B8 "96 21 00 94" #jump to do_tftp_kernel_load()
#load_kernel_by_tag_0x03_way(): case 7 patch(Flash Configure Mode)
binay_patch 0x62BC "20 40 97 d2  50 dd 32 94" #jump to aux_loader->_start(0xBA01)

#pack it back to the mtdblock2-OWL.bin
./pack_to_fwf.sh ${TARGET} >/dev/null && ./pack_to_mtd.sh >/dev/null

RESULT=./bins/mtdblock2-OWL.bin

SEEK=${RES_BIN_OFFSET}
#printf 'SEEK: 0x%x\n\n' ${SEEK}
cat ${CUR_DIR}/bins/loader.bin | dd of=${RESULT} bs=1 seek=${SEEK} conv=notrunc

SOURCE=${RESULT}
RESULT=./bins/rbt-with-aux-for-mtd5.bin
cd ${CUR_DIR}
dd if=${RBT_ENCDEC_DIR}/${SOURCE} of=${RESULT} bs=1 skip=$((ROUTERBOOT_OFFSET)) count=$((ROUTERBOOT_MTD_SIZE))

echo ""
echo "The result file - for write to RouterBOOT(mtd5): ${RESULT}"

[ "${1}" = "nc" ] && {
	echo "nc 172.20.1.77 1111 | mtd write - RouterBOOT" | \
		xclip -selection clipboard

	echo ""
	echo "Ready for NC. port: 1111"
	cat ${RESULT} | nc -l -p 1111 -q 1
}
