#include <sys/api.h>
#include <stdio.h>

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

void x32_Fifo32Put(int data)
{
	int p[32];
	p[0] = mx32api_fifo32_put;
	p[1] = data;
	x32_ApiCall(p);
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

int x32_CreateWindow(void *buf, int w, int h, int inv, char *title)
{
	int p[32];
	p[0] = mx32api_createwindow;
	p[1] = buf;
	p[2] = w;
	p[3] = h;
	p[4] = inv;
	p[5] = title;
	x32_ApiCall(p);
	return p[15];
}

/* libc wrapper */
extern struct File_methods mth;

FILE *__open(char *path, char *mode)
{
	FILE *f = (FILE *)malloc(sizeof(FILE));
	
	int m = 0;
	if(strchr(mode,'r')) m |= FA_READ;
	if(strchr(mode,'w')) m |= FA_WRITE;
	
	f->obj = x32_Open(path,m);
	
	f->vmt = &mth;
	
	return f;
}

size_t __write(FILE* instance, const char *bp, size_t n);
size_t __read(FILE* instance, char *bp, size_t n);
int __close(FILE* instance);
int __seek(FILE* instance, size_t offset, int base);
long __tell(FILE* instance);
int __eof(FILE* instance);

struct File_methods mth = {
	.write = __write,
	.read = __read,
	.close = __close,
	.seek = __seek,
	.tell = __tell,
	.eof = __eof,
};

FILE const __stdin = {
	(void *)1, &mth
};

FILE const __stdout = {
	(void *)2, &mth
};

FILE const __stderr = {
	(void *)2, &mth
};

FILE* const stdin = &__stdin;
FILE* const stdout = &__stdout;
FILE* const stderr = &__stderr;

size_t __write(FILE* instance, const char *bp, size_t n)
{
	int i;
	switch((int)instance->obj) {
	case 1:
		for(i = 0; i < n; i++) x32_Fifo32Put(bp[i]);
		return i;
	case 2:
		for(i = 0; i < n; i++) x32_PutChar(bp[i]);
		return n;
	}
	
	return x32_Write((int)instance->obj,bp,n);
}

size_t __read(FILE* instance, char *bp, size_t n)
{
	int i;
	switch((int)instance->obj) {
	case 1:
		for(i = 0; i < n && x32_Fifo32Status(); i++) bp[i] = x32_Fifo32Get();
		return i;
	case 2:
		return 0;
	}
	
	return x32_Read((int)instance->obj,bp,n);
}

int __close(FILE* instance)
{
	switch((int)instance->obj) {
	case 1:
		return -1;
	case 2:
		return -1;
	}
	
	return x32_Close(instance->obj);
}

int __seek(FILE* instance, size_t offset, int base)
{
	switch((int)instance->obj) {
	case 1:
		return -1;
	case 2:
		return -1;
	}
	
	return x32_Lseek((int)instance->obj, offset, base);
}

long __tell(FILE* instance)
{
	switch((int)instance->obj) {
	case 1:
		return -1;
	case 2:
		return -1;
	}
	
	return x32_Tell((int)instance->obj);
}

int __eof(FILE *instance)
{
	int fd = (int)instance->obj;
	
	int cur = x32_Tell(fd);
	x32_Lseek(fd,0,SK_END);
	int end = x32_Tell(fd);
	
	x32_Lseek(fd,cur,SK_SET);
	
	return cur == end ? 1 : 0;
}

void *__malloc(size_t siz)
{
	return x32_Malloc(siz);
}

void __free(void *ptr)
{
	x32_Free(ptr);
}

void __assert_fail(const char *s, const char *f, unsigned int l)
{
	printf("ASSERT FAILED %s : %s:%d\n",s,f,l);
	for(;;);
}
