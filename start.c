/*
 * Auxiliary OpenWRT kernel loader
 *
 * Copyright (C) 2019-2022 Serhii Serhieiev <adron@mstnt.com>
 */

int _main(unsigned int, unsigned int, unsigned int);

/* this function must go first */
int _start(unsigned int p0)
{
	register unsigned int lr, sp;
	asm volatile ("MOV %0, LR\n" : "=r" (lr));
	asm volatile ("MOV %0, SP\n" : "=r" (sp));
	return _main(p0, lr, sp);
}
