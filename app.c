#include <libmicrox.h>

void putc(int c)
{
	int p[32];
	p[0] = 0;
	p[1] = c;
	mx32api_call(p);
}

void puts(char *s)
{
	while(*s) putc(*s++);
}

void HariMain(void)
{
	puts("hello,world");
	while(1);
}
