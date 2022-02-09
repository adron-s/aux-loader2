#!/bin/sh

RELEASE_LABEL=2.00-latest

cat ./bins/loader.elf > ./releases/${RELEASE_LABEL}/aux-loader.elf
cat ./bins/rbt-with-aux-for-mtd5.bin > ./releases/${RELEASE_LABEL}/rbt-with-aux-for-mtd5.bin
