#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "memory.h"
#include "console.h"

#ifndef __WRAPPER_H__
#define __WRAPPER_H__

extern unsigned int timer_getticks(void);

extern struct CONSOLE *cons;

static void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__ ("outb %0, %1" : : "a" (val), "dN" (port));
}

static uint8_t inb(uint16_t port)
{
	uint8_t val;
	__asm__ __volatile__("inb %1, %0" : "=a" (val) : "dN" (port));
	return val;
}

static void outw(uint16_t port, uint16_t val)
{
	__asm__ __volatile__ ("outw %0, %1" : : "a" (val), "dN" (port));
}

static uint16_t inw(uint16_t port)
{
	uint16_t val;
	__asm__ __volatile__("inw %1, %0" : "=a" (val) : "dN" (port));
	return val;
}

static void outl(uint16_t port, uint32_t val)
{
	__asm__ __volatile__ ("outl %0, %1" : : "a" (val), "dN" (port));
}

static uint32_t inl(uint16_t port)
{
	uint32_t val;
	__asm__ __volatile__("inl %1, %0" : "=a" (val) : "dN" (port));
	return val;
}

static void *kmalloc(int size)
{
	return memman_alloc_4k(MEMMAN_ADDR,size);
}

static int printk(char *format, ...)
{
	int i;
	va_list ap;
	char s[4096];
	va_start(ap, format);
	i = vsprintf(s, format, ap);
	va_end(ap);
	cons_putstr0(cons,s);
	//fputs(s, stdout);
	return i;
}

static int __pit_count(void)
{
	int lo = inb(0x0041);
	int hi = inb(0x0041);
	
	return (hi << 8) | lo;
}

static void wait_loop_usec(int usec)
{
	unsigned int old = timer_getticks();
	unsigned int end = old + (usec / 1000);

	while (timer_getticks() <= end) __asm__ __volatile__("nop\n\t");
}
#endif
