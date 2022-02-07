#!/bin/sh

CUR_DIR=$(pwd)

. ./Build.sh

RBT_ENCDEC_DIR=/home/prog/openwrt/work/RB5009/dumps/fwf
TARGET="${CUR_DIR}/bins/rbt-MODI.dec"
SOURCE="./bins/rbt-7.2rc1-MTD.dec"

cd ${RBT_ENCDEC_DIR}
cat ${SOURCE} > ${TARGET}

. ./func.sh

#lets patch the RouterBOOT

#load_kernel_by_tag_0x03_way(): if tag_03 == 8
#(3 - boot Flash Configure Mode once, then NAND) => goto our_code(0xBA01)
binay_patch 0x62AC "20 40 97 d2  54 dd 32 94"
#check_ELF_and_load_kernel(): if not_an_elf_header => goto our_code(0xBA02)
binay_patch 0xD63C "40 40 97 d2  70 c0 32 94"

#pack it back to the mtdblock2-OWL.bin
./pack_to_fwf.sh ${TARGET} >/dev/null && ./pack_to_mtd.sh >/dev/null

RESULT=./bins/mtdblock2-OWL.bin

SEEK=${RES_BIN_OFFSET}
#printf 'SEEK: 0x%x\n\n' ${SEEK}
cat ${CUR_DIR}/loader.bin | dd of=${RESULT} bs=1 seek=${SEEK} conv=notrunc

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
