/*
 * Auxiliary OpenWRT kernel loader
 *
 * Copyright (C) 2019-2022 Serhii Serhieiev <adron@mstnt.com>
 */

#include <io.h>
#include <types.h>
#include <elf.h>
#include <uimage/fdt.h>
#include <routerboot.h>

//#define DEBUG

#ifdef DEBUG
#define debug printf
#else
#define debug(...)
#endif /* DEBUG */

#define VERSION "2.00"

#define WATCHDOG_PERIOD 60 /* sec */

#define NOR_BOOT_ARG0 0xba01
#define NET_BOOT_ARG0 0xba02

#define SPINOR_OP_READ		0x03	/* Read data bytes (low frequency) */
#define SPINOR_OP_RDSR		0x05	/* Read status register */
#define SPI_REG_CTRL 0xf0510600

#define FIT_IH_MAGIC	0xd00dfeed	/* FIT Magic Number */
#define FIT_KERNEL_NODE_NAME "kernel-1"
#define FIT_DTB_NODE_NAME "fdt-1"

enum image_types {
	UNK_IMAGE,
	ELF_IMAGE,
	FIT_IMAGE
};

int _start(unsigned int);
static void spi_cs_ctrl(int);
unsigned long int ntohl(unsigned long int);
void lzma_setup_workspace(void *);
int lzma_gogogo(void *, void *, u32, u32 *);
int fdt_check_header(void *, u32);
char *fdt_get_prop(void *, char *, char *, u32 *);
int handle_fit_header(void *, u32);
void mvpp2_cleanup(void);
void cleanup_before_linux(void);
void enable_caches(void);
unsigned int mmu_is_enabled(void);
void watchdog_setup(int);
void watchdog_keepalive(void);

void (*kernel_entry)(void *fdt_addr, void *res0,
	void *res1, void *res2);
void *kernel_p0;
void *dtb_mem;

#ifdef DEBUG
static inline void dump_mem(unsigned char *data, unsigned int size) {
	int a;
	for (a = 0; a < size; a++) {
		if (a % 4 == 0)
			printf("| ");
		if (a % 16 == 0)
			printf("\n0x%08x: ", &data[a]);
		printf("%02x ", data[a]);
	}
	printf("\n");
}
#endif

static int elf_check_header(unsigned char *buf)
{
	Elf64_Ehdr *elf_hdr = (void *)buf;
	if (buf[0] != 0x7F || buf[1] != 0x45 || buf[2] != 0x4C || buf[3] != 0x46)
		return -1;
	return elf_hdr->e_shoff + (elf_hdr->e_shnum) * elf_hdr->e_shentsize;
}

static void read_spi_nor(void *buf, int len)
{
	unsigned char data[4];
	data[0] = SPINOR_OP_READ;
	//offset: +1Mb
	data[1] = 0x10;
	data[2] = 0x00;
	data[3] = 0x00;
	debug("spi_xfer: reading %d bytes to [ 0x%x .. 0x%x ]\n",
		len, buf, buf + len);
	spi_cs_ctrl(1);
	spi_xfer(4 << 3, data, 0);
	spi_xfer(len << 3, 0, buf);
	spi_cs_ctrl(0);
	debug("spi_xfer: end\n");
}

static inline int image_header_max_size(void)
{
	if (sizeof(struct fdt_header) > sizeof(Elf64_Ehdr))
		return sizeof(struct fdt_header);
	else
		return sizeof(Elf64_Ehdr);
}

static int check_image_header(void *buf, int *_len)
{
	int len, image_type;
	image_type = FIT_IMAGE;
	len = fdt_check_header(buf, 0);
	if (len < 0) {
		image_type = ELF_IMAGE;
		len = elf_check_header(buf);
	}
	if (len < 0) {
		image_type = UNK_IMAGE;
		*_len = 0;
	} else {
		*_len = len;
	}
	return image_type;
}

int _main(u32 arg0, u32 lr_reg, u32 sp_reg)
{
	int image_len = 0, image_type;
	unsigned char *data_buf = ELF_image_ptr;
	int mmu_status = mmu_is_enabled();

	if (arg0 == NET_BOOT_ARG0)
		printf("not ELF\n");

	printf("\n");
	printf("OpenWRT AUX loader v%s\n",  VERSION);
	printf("Copyright 2019-2022, Serhii Serhieiev <adron@mstnt.com>\n\n");

	debug("Call point: 0x%x, SP: 0x%x, Start address: 0x%p\n", lr_reg, sp_reg, (void *)_start);
	debug("arg0: 0x%x\n\n", arg0);

	debug("Watchdog is set to: %d sec\n", WATCHDOG_PERIOD);
	watchdog_setup(WATCHDOG_PERIOD);

	if (arg0 < NOR_BOOT_ARG0 || arg0 > NET_BOOT_ARG0) {
		printf("Incorrect arg0 value: 0x%x !\n", arg0);
		mdelay(10000);
		reset_cpu();
	}

	debug("MMU status: 0x%x\n", mmu_status);
	if (!mmu_status)
		enable_caches(); /* Enable I and D caches (for LZMA ops speed up) */

	if (arg0 == NOR_BOOT_ARG0) { //NOR boot
		watchdog_keepalive();
		/* at first we process the header - to find out the total
			 length and image type */
		read_spi_nor(data_buf, image_header_max_size());
		watchdog_keepalive();
	}
	image_type = check_image_header(data_buf, &image_len);
	if (image_type == UNK_IMAGE) {
		printf("Unknown image header: 0x%08x\n", * ((u32 *)data_buf));
		reset_cpu();
	}
	if (arg0 == NOR_BOOT_ARG0 && image_len > 0) { //NOR boot
		printf("Loading %d bytes from SPI NOR...", image_len);
		read_spi_nor(data_buf, image_len);
		printf("Done\n\n");
	}
	debug("Image type: %d, length: %d\n\n", image_type, image_len);

	if (image_type == FIT_IMAGE) {
		void *_kernel_data_start = (void *)data_buf;
		u32 *_magic = (void*)_kernel_data_start, magic;
		int ret = -100;
		dtb_mem = DTB_mem_ptr;
		magic = ntohl(*_magic);
		printf("FIT image:\n");
		if (magic == FIT_IH_MAGIC) {
			printf("  Magic:        0x%x, FIT uImage\n", magic);
			ret = handle_fit_header(_kernel_data_start, image_len);
		} else
			printf("  Magic:        0x%x, UNKNOWN !!!\n", magic);
		if (ret) {
			printf("handle_fit_header() is finished. ret: -%d\n", -1 * ret);
		} else {
			//dump_mem(data_buf);
			printf("Doing cleanup before start the Linux kernel\n");
			mvpp2_cleanup();
			cleanup_before_linux();
			watchdog_keepalive();
			printf("Starting kernel at 0x%p, dtb(arg0): 0x%p\n", kernel_entry, kernel_p0);
			__asm__ __volatile__("": : :"memory");
			kernel_entry(kernel_p0, NULL, NULL, NULL);
			reset_cpu();
		}
	} else if (image_type == ELF_IMAGE) {
		//dump_mem(data_buf, 0x100);
		check_ELF_and_load_kernel(0);
	}

	reset_cpu();
	while (1) { };
}

int _elf_start(void)
{
	return _start(0xba01);
}

static void spi_cs_ctrl(int activate)
{
	unsigned int ctrl_reg_val = readl(SPI_REG_CTRL);
	if (activate)
		ctrl_reg_val |= 1;
	else
		ctrl_reg_val &= 0xfffffffe;
	writel(ctrl_reg_val, SPI_REG_CTRL);
}

unsigned long int ntohl(unsigned long int d)
{
	unsigned long int res = 0;
	int a;
	for(a = 0; a < 3; a++){
		res |= d & 0xFF;
		res <<= 8; d >>= 8;
	}
	res |= d & 0xFF;
	return res;
}

/* FIT - Flattened Image Tree
	 ! Not to be confused with an FDT - Flattened Device Tree !
	 ! MAGIC of FIT == ~MAGIC of FDT !
*/
int handle_fit_header(void *_kernel_data_start, u32 kern_image_len)
{
	void *data = (void*)_kernel_data_start;
	void *kernel_load = NULL;
	void *kernel_ep = NULL;
	char *kernel_name = NULL;
	char *kernel_compr = NULL;
	void *dtb_data = NULL; //device tree blob
	void *dtb_dst = DTB_mem_ptr;
	u32 dtb_body_len = 0;
	u32 kernel_body_len = 0;
	u32 kernel_uncompr_size = 0;
	void *src = NULL;
	void *dst = NULL;
	char *tmp_c;
	int ret;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");
	/* Do FDT header base checks */
	ret = fdt_check_header(data, kern_image_len);
	if (ret < 0) {
		printf("Error ! FDT header is corrupted! ret: %d\n", ret);
		return -99;
	}
	if (!(kernel_name = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "description", NULL)))
		return -98;
	if (!(tmp_c = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "load", NULL)))
		return -97;
	kernel_load = (void*)ntohl(*(u32*)tmp_c);
	dst = kernel_load;
	if (!(tmp_c = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "entry", NULL)))
		return -96;
	kernel_ep = (void*)ntohl(*(u32*)tmp_c);
	if (!(src = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "data", &kernel_body_len)))
		return -95;
	if (!(kernel_compr = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "compression", NULL)))
		return -94;
	if (!(dtb_data = fdt_get_prop(data, FIT_DTB_NODE_NAME, "data", &dtb_body_len)))
		return -93;

	printf("  Kernel Size:  %u\n", kernel_body_len);
	printf("  Description:  %s\n", kernel_name);
	printf("  Load Address: 0x%08x\n", kernel_load);
	printf("  Entry Point:  0x%08x\n", kernel_ep);
	printf("  Compression:  %s\n", kernel_compr);

	//check kernel@1->data size for adequate value
	if (kernel_body_len >= kern_image_len) {
		printf("\n");
		printf("Error ! Kernel sizes mismath detected !\n");
		printf("  details: %d >= %d !\n", kernel_body_len, kern_image_len);
		return -99;
	}
	printf("\n");
	watchdog_keepalive();
	printf("Extracting LZMA kernel...");
	lzma_setup_workspace(LZMA_workspace_ptr);
	ret = lzma_gogogo(dst, src, kernel_body_len, &kernel_uncompr_size);
	if (ret)
		return ret;

	watchdog_keepalive();
	/* prepare device tree blob data */
	memcpy(dtb_dst, dtb_data, dtb_body_len);

	kernel_entry = kernel_ep;
	kernel_p0 = (void *)dtb_dst;
	return 0;
}
