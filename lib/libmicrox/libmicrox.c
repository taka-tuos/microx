#include <libmicrox.h>
#include <stdio.h>

void mx32api_call(int *p)
{
	asm volatile ("movl %0,%%eax\n\t" :: "r"(p));
	asm volatile ("int $0x40\n\t");
}

void putc(int c)
{
	int p[32];
	p[0] = mx32api_putchar;
	p[1] = c;
	mx32api_call(p);
}

void keyboard_enable()
{
	int p[32];
	p[0] = mx32api_fifo_typeenable;
	p[1] = FIFOTYPE_KEYBOARD;
	mx32api_call(p);
}

int fifo32_status()
{
	int p[32];
	p[0] = mx32api_fifo32_status;
	mx32api_call(p);
	return p[1];
}

int fifo32_get()
{
	int p[32];
	p[0] = mx32api_fifo32_get;
	mx32api_call(p);
	return p[1];
}

int getc()
{
	while(1) {
		while(fifo32_status() < 2);
		if(fifo32_get() == 0) {
			return fifo32_get();
		}
	}
}

void exit()
{
	int p[32];
	p[0] = mx32api_exit;
	mx32api_call(p);
}

int printf(char *str, ...)
{
	va_list ap;
	char s[4096];
	va_start(ap, str);
	int i = vsprintf(s, str, ap);
	va_end(ap);
	
	puts(s);
}

void puts(char *s)
{
	unsigned char *sz = (unsigned char *)s;
	while(*sz) putc(*sz++);
}

int open(const char *path, int mode)
{
	int p[32];
	p[0] = mx32api_open;
	p[1] = path;
	p[2] = mode;
	mx32api_call(p);
	return p[15];
}

int close(int fd)
{
	int p[32];
	p[0] = mx32api_close;
	p[1] = fd;
	mx32api_call(p);
	return p[15];
}

int lseek(int fd, int off, int from)
{
	int p[32];
	p[0] = mx32api_lseek;
	p[1] = fd;
	p[2] = off;
	p[3] = from;
	mx32api_call(p);
	return p[15];
}

int read(int fd, void *buf, int cnt)
{
	int p[32];
	p[0] = mx32api_read;
	p[1] = fd;
	p[2] = buf;
	p[3] = cnt;
	mx32api_call(p);
	return p[15];
}

int write(int fd, void *buf, int cnt)
{
	int p[32];
	p[0] = mx32api_write;
	p[1] = fd;
	p[2] = buf;
	p[3] = cnt;
	mx32api_call(p);
	return p[15];
}

int tell(int fd)
{
	int p[32];
	p[0] = mx32api_tell;
	p[1] = fd;
	mx32api_call(p);
	return p[15];
}

int errno()
{
	int p[32];
	p[0] = mx32api_errno;
	mx32api_call(p);
	return p[15];
}
