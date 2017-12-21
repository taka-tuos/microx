#include <sys/api.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "stb_image.h"

int pow(int x, int n)
{
	if(n == 2) return x*x;
	return pow(x,n-1); 
}

double ldexp(double x, int exp)
{
	return x * pow(2,exp);
}

void swaprgb(unsigned int *p, int sx, int sy)
{
	int x, y;
	
	for(y = 0; y < sy; y++) {
		for(x = 0; x < sx; x++) {
			int c = p[y * sx + x];
			int a = (c >> 24) & 0xff;
			int r = (c >> 16) & 0xff;
			int g = (c >>  8) & 0xff;
			int b = (c >>  0) & 0xff;
			
			p[y * sx + x] = (a << 24) | (b << 16) | (g << 8) | r;
		}
	}
}

//unsigned char fb[640*480/2];

//int buf[320*240];

int main(void)
{
	//DIR Dir;
	//FILINFO Finfo;
	
	//int res,p1,s1,s2;
	//res = f_opendir(&Dir,"/");
	
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
	
	/*FILE *fp;
	char s[256];
	fp = fopen("README.MD","rb");
	
	while(!feof(fp)) puts(fgets(s,256,fp));
	fclose(fp);
	
	printf("END\n");*/
	
	//void *buf = x32_Malloc(256*32*4);
	int buf[300*300];
	
	int win = x32_CreateWindow(buf, 300, 300, -1, "init.eim 日本語テスト");
	
	int w,h,b;
	
	unsigned int *p = (unsigned int *)stbi_load("pic.png", &w, &h, &b, 4);
	
	printf("%08x\n",p);
	
	swaprgb(p,w,h);
	
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			buf[(y+30) * 300 + (x+20)] = p[y * w + x];
		}
	}
	
	x32_RefreshWindow(win, 20, 30, 256+20, 256+30);
	
	while(1) {
		int ch = x32_GetChar();
		for(int y = 0; y < 32; y++) {
			for(int x = 0; x < 32; x++) {
				buf[(y+30) * 300 + (x+20)] = 0;
			}
		}
		
		char sz[2];
		
		sz[0] = ch;
		sz[1] = 0;
		
		x32_TextOut(win, 20, 30, 0xffffff, sz);
		
		x32_RefreshWindow(win, 20, 30, 32+20, 32+30);
	}
	
	//for(int i = 0;; i++) xprintf("scroll!! %d\n",i);
	
	return 0;
}
