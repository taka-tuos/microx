#include <sys/api.h>

void x32_ApiCall(int *p)
{
	asm volatile ("movl %0,%%eax\n\t" :: "r"(p));
	asm volatile ("int $0x40\n\t");
}

void x32_PutChar(int c)
{
	int p[32];
	p[0] = mx32api_putchar;
	p[1] = c;
	x32_ApiCall(p);
}

void x32_KeyboardEnable()
{
	int p[32];
	p[0] = mx32api_fifo_typeenable;
	p[1] = FIFOTYPE_KEYBOARD;
	x32_ApiCall(p);
}

int x32_Fifo32Status()
{
	int p[32];
	p[0] = mx32api_fifo32_status;
	x32_ApiCall(p);
	return p[1];
}

int x32_Fifo32Get()
{
	int p[32];
	p[0] = mx32api_fifo32_get;
	x32_ApiCall(p);
	return p[1];
}

int x32_GetChar()
{
	while(1) {
		while(x32_Fifo32Status() < 2);
		if(x32_Fifo32Get() == 0) {
			return x32_Fifo32Get();
		}
	}
}

void x32_Exit()
{
	int p[32];
	p[0] = mx32api_exit;
	x32_ApiCall(p);
}

void x32_PutString(char *s)
{
	unsigned char *sz = (unsigned char *)s;
	while(*sz) x32_PutChar(*sz++);
}

int x32_Open(const char *path, int mode)
{
	int p[32];
	p[0] = mx32api_open;
	p[1] = path;
	p[2] = mode;
	x32_ApiCall(p);
	return p[15];
}

int x32_Close(int fd)
{
	int p[32];
	p[0] = mx32api_close;
	p[1] = fd;
	x32_ApiCall(p);
	return p[15];
}

int x32_Lseek(int fd, int off, int from)
{
	int p[32];
	p[0] = mx32api_lseek;
	p[1] = fd;
	p[2] = off;
	p[3] = from;
	x32_ApiCall(p);
	return p[15];
}

int x32_Read(int fd, void *buf, int cnt)
{
	int p[32];
	p[0] = mx32api_read;
	p[1] = fd;
	p[2] = buf;
	p[3] = cnt;
	x32_ApiCall(p);
	return p[15];
}

int x32_Write(int fd, void *buf, int cnt)
{
	int p[32];
	p[0] = mx32api_write;
	p[1] = fd;
	p[2] = buf;
	p[3] = cnt;
	x32_ApiCall(p);
	return p[15];
}

int x32_Tell(int fd)
{
	int p[32];
	p[0] = mx32api_tell;
	p[1] = fd;
	x32_ApiCall(p);
	return p[15];
}

int x32_Errno()
{
	int p[32];
	p[0] = mx32api_errno;
	x32_ApiCall(p);
	return p[15];
}

void *x32_Malloc(int siz)
{
	int p[32];
	p[0] = mx32api_malloc;
	p[1] = siz;
	x32_ApiCall(p);
	return p[15];
}

void x32_Free(void *ptr)
{
	int p[32];
	p[0] = mx32api_free;
	p[1] = ptr;
	x32_ApiCall(p);
}
