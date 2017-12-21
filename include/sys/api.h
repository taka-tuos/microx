#ifndef __LIBMICROX_H__
#define __LIBMICROX_H__

#ifndef __KERNEL__

typedef enum {
	FR_OK = 0,				/* (0) Succeeded */
	FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,				/* (2) Assertion failed */
	FR_NOT_READY,			/* (3) The physical drive cannot work */
	FR_NO_FILE,				/* (4) Could not find the file */
	FR_NO_PATH,				/* (5) Could not find the path */
	FR_INVALID_NAME,		/* (6) The path name format is invalid */
	FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	FR_EXIST,				/* (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/* (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any problem */
	FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > FF_FS_LOCK */
	FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
};

/* File access mode and open method flags (3rd argument of f_open) */
#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30

/* File attribute bits for directory entry (FILINFO.fattrib) */
#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */

#define SK_SET	0x00
#define SK_CUR	0x01
#define SK_END	0x02

int x32_Open(const char *path, int mode);
int x32_Close(int fd);
int x32_Lseek(int fd, int off, int from);
int x32_Read(int fd, void *buf, int cnt);
int x32_Write(int fd, void *buf, int cnt);
int x32_Tell(int fd);

void x32_PutChar(int c);
void x32_KeyboardEnable();
int x32_Fifo32Status();
void x32_Fifo32Put(int data);
int x32_Fifo32Get();
int x32_GetChar();
void x32_Exit();
void x32_PutString(char *s);

void *x32_Malloc(int siz);
void x32_Free(void *ptr);

int x32_CreateWindow(void *buf, int w, int h, int inv, char *title);
void x32_CloseWindow(int win);
void x32_TextOut(int win, int x, int y, int c, char *s);
void x32_Sleep();
void x32_RefreshWindow(int win, int x0, int y0, int x1, int y1);

int x32_Errno();

void x32_ApiCall(int *p);

#endif

enum {
	mx32api_exit,
	mx32api_putchar,
	mx32api_fifo32_status,
	mx32api_fifo32_put,
	mx32api_fifo32_get,
	mx32api_fifo_typeenable,
	mx32api_fifo_typedisable,
	mx32api_open,
	mx32api_close,
	mx32api_read,
	mx32api_write,
	mx32api_lseek,
	mx32api_tell,
	mx32api_errno,
	mx32api_malloc,
	mx32api_free,
	mx32api_createwindow,
	mx32api_closewindow,
	mx32api_textout,
	mx32api_sleep,
	mx32api_refreshwindow,
};

enum {
	FIFOTYPE_KEYBOARD,
	FIFOTYPE_NUM
};

#endif
