#ifndef __ROUTER_BOOT_H
#define __ROUTER_BOOT_H

typedef int printf_fn(const char *, ...);
typedef unsigned int uint_fn1(void);
typedef unsigned int uint_fn2(unsigned int);
typedef void void_fn1(void);
typedef void void_fn2(unsigned int);
typedef void void_fn3(void*, void*);
typedef void spi_xfer_fn(unsigned int, void*, void*);
typedef void memcpy_fn(void*, void*, unsigned long);
typedef void *memset_fn(void *s, int c, unsigned long);

#define ROUTERBOOT_TEXT_BASE 0x1000000
#define RBADDR(addr) ((void *)(ROUTERBOOT_TEXT_BASE + addr))

#define SOC_REGS_PHY_BASE	0xf0000000
#define MVEBU_REGISTER(x)	((void *)(SOC_REGS_PHY_BASE + x))

#define SBSA_GWDT_CF_BASE MVEBU_REGISTER(0x610000)
#define SBSA_GWDT_RF_BASE MVEBU_REGISTER(0x600000)

#define mvpp2_mem0_ptr RBADDR(0x121d0)
#define mvpp2_priv_ptr RBADDR(0x12330)

#define ELF_image_ptr (void *)(0x02000000)

#define LZMA_workspace_ptr (void *)(0x03000000)
#define DTB_mem_ptr (void *)0x00700000

#define rbt_version_must_be_string "7.2rc1"
#define rbt_version_must_be_offset RBADDR(0x1097c)

#define declare_rbt_func(name, type, addr) \
	static __attribute__ ((unused)) type *name = RBADDR(addr)

declare_rbt_func(printf, printf_fn, 0x1dd4);
declare_rbt_func(reset_cpu, void_fn1, 0x4f90);
declare_rbt_func(spi_xfer, spi_xfer_fn, 0xcc5c);
declare_rbt_func(check_ELF_and_load_kernel, void_fn2, 0xd594);
declare_rbt_func(flush_dcache, void_fn1, 0xf088);
declare_rbt_func(wait_for_dcache_flush, void_fn1, 0xf0a8);
declare_rbt_func(invalidate_tlbi, void_fn1, 0xef78);
declare_rbt_func(mvpp2_deinit, void_fn3, 0xc86c);
declare_rbt_func(mdelay, uint_fn2, 0x19a8);
declare_rbt_func(memcpy, memcpy_fn, 0x2f04);
declare_rbt_func(main_loop, void_fn1, 0xa218);
declare_rbt_func(menu_handler, void_fn1, 0x6228);
declare_rbt_func(print_board_info, void_fn1, 0x3fb4);
declare_rbt_func(memset, memset_fn, 0x2630);
//declare_rbt_func(get_sctlr, uint_fn1, 0x1570);
//declare_rbt_func(set_sctlr, void_fn2, 0x15a0);

#endif /* __ROUTER_BOOT_H */
