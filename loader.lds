OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
SECTIONS {
	.text : {
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
	}
}
