#ifndef __LIBMICROX_H__
#define __LIBMICROX_H__

enum {
	mx32api_putchar,
	mx32api_getchar,
	mx32api_fifotypeenable,
	mx32api_fifotypedisable,
};

enum {
	FIFOTYPE_KEYBOARD
};

void mx32api_call(int *p);

#endif
