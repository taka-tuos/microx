#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "memory.h"
#include "console.h"
#include "ff.h"

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
	char *p = (char *)memman_alloc_4k(MEMMAN_ADDR,size+4);
	*((int *)p) = size+4;
	return p + 4;
}

static void kfree(void *ptr)
{
	char *p = (char *)ptr;
	p = p - 4;
	memman_free_4k(MEMMAN_ADDR,p,*((int *)p));
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
/*
static FILE *fopen(const char *path, const char *mode)
{
	BYTE modeb = *mode == 'r' ? FA_READ : FA_WRITE;
	FILE *fp = kmalloc(sizeof(FILE));
	
	f_open(fp,path,modeb);
	
	return fp;
}

static void fclose(FILE *fp)
{
	f_close(fp);
	
	kfree(fp);
}

static void fseek(FILE *fp, int ofs, int org)
{
	int fofs = 0;
	switch(org) {
	case SEEK_CUR:
		fofs = f_tell(fp) + ofs;
		break;
	case SEEK_END:
		fofs = f_size(fp) - ofs;
		break;
	}
	
	f_lseek(fp, fofs);
}

static int ftell(FILE *fp)
{
	return f_tell(fp);
}

static int fread(void *p, int siz1, int siz2, FILE *fp)
{
	UINT br;
	
	f_read(fp, p, siz1*siz2,&br);
	
	return br;
}*/

#endif
