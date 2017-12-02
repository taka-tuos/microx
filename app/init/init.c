#include <libmicrox.h>
#include <stdio.h>

void putc(int c)
{
	int p[32];
	p[0] = mx32api_putchar;
	p[1] = c;
	mx32api_call(p);
}

void puts(char *s)
{
	while(*s) putc(*s++);
}

void HariMain(void)
{
	int i = 0;
	while(1) {
		char s[256];
		sprintf(s,"hello,world %d\n", i);
		puts(s);
		i++;
	}
}
