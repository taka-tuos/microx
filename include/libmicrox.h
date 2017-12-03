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

#define SK_END	0x00
#define SK_CUR	0x01
#define SK_SET	0x02

int open(const char *path, int mode);
int close(int fd);
int lseek(int fd, int off, int from);
int read(int fd, void *buf, int cnt);
int write(int fd, void *buf, int cnt);
int tell(int fd);

void putc(int c);
void keyboard_enable();
int fifo32_status();
int fifo32_get();
int getc();
void exit();
void puts(char *s);

void mx32api_call(int *p);

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
};

enum {
	FIFOTYPE_KEYBOARD,
	FIFOTYPE_NUM
};

#endif
