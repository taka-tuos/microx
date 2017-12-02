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

void puts(char *s)
{
	unsigned char *sz = (unsigned char *)s;
	while(*sz) putc(*sz++);
}

FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode)
{
	int p[32];
	p[0] = mx32api_open;
	p[1] = fp;
	p[2] = path;
	p[3] = mode;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_close(FIL* fp)
{
	int p[32];
	p[0] = mx32api_close;
	p[1] = fp;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br)
{
	int p[32];
	p[0] = mx32api_read;
	p[1] = fp;
	p[2] = buff;
	p[3] = btr;
	p[4] = br;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw)
{
	int p[32];
	p[0] = mx32api_write;
	p[1] = fp;
	p[2] = buff;
	p[3] = btw;
	p[4] = bw;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_lseek(FIL* fp, FSIZE_t ofs)
{
	int p[32];
	p[0] = mx32api_lseek;
	p[1] = fp;
	p[2] = ofs;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_truncate(FIL* fp)
{
	int p[32];
	p[0] = mx32api_truncate;
	p[1] = fp;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_sync(FIL* fp)
{
	int p[32];
	p[0] = mx32api_sync;
	p[1] = fp;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_opendir(DIR* dp, const TCHAR* path)
{
	int p[32];
	p[0] = mx32api_opendir;
	p[1] = dp;
	p[2] = path;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_closedir(DIR* dp)
{
	int p[32];
	p[0] = mx32api_closedir;
	p[1] = dp;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_readdir(DIR* dp, FILINFO* fno)
{
	int p[32];
	p[0] = mx32api_readdir;
	p[1] = dp;
	p[2] = fno;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_mkdir(const TCHAR* path)
{
	int p[32];
	p[0] = mx32api_mkdir;
	p[1] = path;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_unlink(const TCHAR* path)
{
	int p[32];
	p[0] = mx32api_unlink;
	p[1] = path;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new)
{
	int p[32];
	p[0] = mx32api_rename;
	p[1] = path_old;
	p[2] = path_new;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_stat(const TCHAR* path, FILINFO* fno)
{
	int p[32];
	p[0] = mx32api_stat;
	p[1] = path;
	p[2] = fno;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_chdir(const TCHAR* path)
{
	int p[32];
	p[0] = mx32api_chdir;
	p[1] = path;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_chdrive(const TCHAR* path)
{
	int p[32];
	p[0] = mx32api_chdrive;
	p[1] = path;
	mx32api_call(p);
	return (FRESULT)p[15];
}

FRESULT f_getcwd(TCHAR* buff, UINT len)
{
	int p[32];
	p[0] = mx32api_getcwd;
	p[1] = buff;
	p[2] = len;
	mx32api_call(p);
	return (FRESULT)p[15];
}

int f_putc(TCHAR c, FIL* fp)
{
	int p[32];
	p[0] = mx32api_fputc;
	p[1] = c;
	p[2] = fp;
	mx32api_call(p);
	return (int)p[15];
}

int f_puts(const TCHAR* str, FIL* cp)
{
	int p[32];
	p[0] = mx32api_fputs;
	p[1] = str;
	p[2] = cp;
	mx32api_call(p);
	return (int)p[15];
}

int f_printf(FIL* fp, const TCHAR* str, ...)
{
	va_list ap;
	char s[4096];
	va_start(ap, str);
	int i = vsprintf(s, str, ap);
	va_end(ap);
	
	int p[32];
	p[0] = mx32api_fputs;
	p[1] = s;
	p[2] = fp;
	mx32api_call(p);
	return i;
}

TCHAR* f_gets(TCHAR* buff, int len, FIL* fp)
{
	int p[32];
	p[0] = mx32api_fgets;
	p[1] = buff;
	p[2] = len;
	p[3] = fp;
	mx32api_call(p);
	return (TCHAR*)p[15];
}
