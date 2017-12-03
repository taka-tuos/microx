#include <libmicrox.h>
#include <stdio.h>

#include "xprintf.h"

//unsigned char fb[640*480/2];

void HariMain(void)
{
	//DIR Dir;
	//FILINFO Finfo;
	
	//int res,p1,s1,s2;
	//res = f_opendir(&Dir,"/");
	
	xdev_out(putc);
	
	//fbblt(fb);
	
	/*int i = 0;
	while(1) {
		fbwcol(i&0xf);
		for(int y = 0; y < 480; y++) {
			for(int x = 0; x < 640/8; x++) {
				fbwrite(y*(640/8)+x,0xff);
			}
		}
		i++;
	}*/
	
	/*if (res) {
		puts("opendir failed");
		exit();
	}*/
	//p1 = s1 = s2 = 0;
	/*for(;;) {
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0]) break;
		if (Finfo.fattrib & AM_DIR) {
			s2++;
		} else {
			s1++; p1 += Finfo.fsize;
		}
		xprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n", 
				(Finfo.fattrib & AM_DIR) ? 'D' : '-',
				(Finfo.fattrib & AM_RDO) ? 'R' : '-',
				(Finfo.fattrib & AM_HID) ? 'H' : '-',
				(Finfo.fattrib & AM_SYS) ? 'S' : '-',
				(Finfo.fattrib & AM_ARC) ? 'A' : '-',
				(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
				(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
				Finfo.fsize, &(Finfo.fname[0]));
	}
	xprintf("%4u File(s),%10lu bytes\n%4u Dir(s)\n", s1, p1, s2);*/
	
	//f_closedir(&Dir);
	
	int fd;
	char s[256];
	fd = open("README.MD",FA_READ);
	
	while(read(fd,s,1)) putc(s[0]);
	close(fd);
	
	xprintf("END\n");
	
	for(int i = 0;; i++) xprintf("scroll!! %d\n",i);
	
	exit();
}
