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

#define mdelay ((uint_fn2 *)RBADDR(0x19a8))
#define printf ((printf_fn *)RBADDR(0x1dd4))
#define main_loop ((void_fn1 *)RBADDR(0xa218))
#define menu_handler ((void_fn1 *)RBADDR(0x6228))
#define reset_cpu ((void_fn1 *)RBADDR(0x4f90))
#define print_board_info ((void_fn1 *)RBADDR(0x3fb4))
#define spi_xfer ((spi_xfer_fn *)RBADDR(0xcc5c))
#define check_ELF_and_load_kernel ((void_fn2 *)RBADDR(0xd594))
#define memcpy ((memcpy_fn *)RBADDR(0x2f04))
#define memset ((memset_fn *)RBADDR(0x2630))
#define get_sctlr ((uint_fn1 *)RBADDR(0x1570))
#define set_sctlr ((void_fn2 *)RBADDR(0x15a0))
#define flush_dcache ((void_fn1 *)RBADDR(0xf088))
#define wait_for_dcache_flush ((void_fn1 *)RBADDR(0xf0a8))
#define invalidate_tlbi ((void_fn1 *)RBADDR(0xef78))
#define mvpp2_deinit ((void_fn3 *)RBADDR(0xc86c))

#endif /* __ROUTER_BOOT_H */
