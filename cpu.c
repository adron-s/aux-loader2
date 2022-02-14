/*
 * Auxiliary OpenWRT kernel loader
 *
 * Copyright (C) 2019-2022 Serhii Serhieiev <adron@mstnt.com>
 *
 * Some structures and code has been taken from the U-Boot project.
 *	(C) Copyright 2008 Semihalf
 *	(C) Copyright 2000-2005
 *	Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <types.h>
#include <io.h>
#include <routerboot.h>

#define CR_M		(1 << 0)	/* MMU enable			*/
#define CR_A		(1 << 1)	/* Alignment abort enable	*/
#define CR_C		(1 << 2)	/* Dcache enable		*/
#define CR_SA		(1 << 3)	/* Stack Alignment Check Enable	*/
#define CR_I		(1 << 12)	/* Icache enable		*/
#define CR_WXN		(1 << 19)	/* Write Permision Imply XN	*/
#define CR_EE		(1 << 25)	/* Exception (Big) Endian	*/

/*
 * Generic timer implementation of get_tbclk()
 */
unsigned long get_tbclk(void)
{
	unsigned long cntfrq;
	asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
	return cntfrq;
}

static unsigned int current_el(void)
{
	unsigned long el;

	asm volatile("mrs %0, CurrentEL" : "=r" (el) : : "cc");
	return 3 & (el >> 2);
}

static unsigned int get_sctlr(void)
{
	unsigned int el;
	unsigned long val;

	el = current_el();
	if (el == 1)
		asm volatile("mrs %0, sctlr_el1" : "=r" (val) : : "cc");
	else if (el == 2)
		asm volatile("mrs %0, sctlr_el2" : "=r" (val) : : "cc");
	else
		asm volatile("mrs %0, sctlr_el3" : "=r" (val) : : "cc");

	return val;
}

static void set_sctlr(unsigned long val)
{
	unsigned int el;

	el = current_el();
	if (el == 1)
		asm volatile("msr sctlr_el1, %0" : : "r" (val) : "cc");
	else if (el == 2)
		asm volatile("msr sctlr_el2, %0" : : "r" (val) : "cc");
	else
		asm volatile("msr sctlr_el3, %0" : : "r" (val) : "cc");

	asm volatile("isb");
}

unsigned int mmu_is_enabled(void)
{
	return get_sctlr() & CR_M;
}

void enable_caches(int what)
{
	uint32_t sctlr;
	sctlr = get_sctlr();
	if (what == 2) {
		/* since RouterBoot has already configure all the caches, but now its
			 are turned off - we just turn them on here, and nothing more */
		set_sctlr(sctlr | (CR_C|CR_M|CR_I));
	} else if (what == 1) {
		/* enable only ICache and without RouterBOOT help!
			 the DСache code is very complex and large.
			 so I won't bring it here. */
		set_sctlr(sctlr | CR_I);
	}
}

void icache_disable(void)
{
	set_sctlr(get_sctlr() & ~CR_I);
}

/*
	If we do not do this cleaning, then the Linux kernel will swear
	at the dirty mvpp2 bufs:
		Pool does not have so many bufs pool(0) bufs(2064)
		WARNING: CPU: 0 PID: 472 at mvpp2_bm_bufs_free+0x1bc/0x1f4.
	In RouberBOOT->check_ELF_and_load_kernel() this is done
		just before cleanup_before_linux()	*/
void mvpp2_cleanup(void)
{
	u64 *ptr = mvpp2_priv_ptr;
	if (*ptr) {
		//printf("ptr: 0x%p: 0x%llx\n", ptr, *ptr);
		ptr = (void *)(*ptr + 0x70);
		int is_enabled = (*ptr & 0x1) != 0;
		//printf("ptr: 0x%p: 0x%llx. mvpp2_is_enabled = %d\n", ptr, *ptr, is_enabled);
		if (is_enabled) {
			ptr = mvpp2_mem0_ptr;
			if (*ptr) {
				//printf("ptr: 0x%p: 0x%llx\n", ptr, *ptr);
				ptr = (void *)(*ptr + 0x10);
				if (*ptr) {
					void *p0 = (void *)*ptr;
					//printf("ptr: 0x%p: 0x%llx\n", ptr, *ptr);
					ptr = (void *)(*ptr + 0x48);
					if (*ptr) {
						void *p1 = (void *)*ptr;
						//printf("ptr: 0x%p: 0x%llx\n", ptr, *ptr);
						//printf("mvpp2_deinit(0x%p, 0x%p)\n", p0, p1);
						mvpp2_deinit(p0, p1);
					}
				}
			}
		}
	}
}

void cleanup_before_linux(void)
{
	uint32_t sctlr;
	sctlr = get_sctlr();
  flush_dcache();
  wait_for_dcache_flush();
  invalidate_tlbi();
	set_sctlr(sctlr & ~(CR_C|CR_M|CR_I));
}

#define SBSA_GWDT_WRR		0x000
#define SBSA_GWDT_WCS		0x000
#define SBSA_GWDT_WOR		0x008
#define SBSA_GWDT_WCS_EN	0x1
#define SBSA_GWDT_W_IIDR	0xfcc
#define SBSA_GWDT_VERSION_MASK  0xF
#define SBSA_GWDT_VERSION_SHIFT 16

void watchdog_setup(int period)
{
	u32 reg_val, clk = get_tbclk();
	reg_val = clk / 2 * period;
	writel(reg_val, SBSA_GWDT_CF_BASE + SBSA_GWDT_WOR);
	writel(SBSA_GWDT_WCS_EN, SBSA_GWDT_CF_BASE + SBSA_GWDT_WCS);
}

void watchdog_keepalive(void)
{
	writel(0, SBSA_GWDT_RF_BASE + SBSA_GWDT_WRR);
}

void native_reset_cpu(void)
{
	watchdog_setup(1);
	while (1);
}
