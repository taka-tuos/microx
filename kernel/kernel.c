#include "multiboot.h"
#include "ata.h"
#include "pci.h"
#include "memory.h"
#include "console.h"
#include "bootpack.h"
#include "ff.h"
#include "sheet.h"

#include "nvbdflib.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#include "stb_image.h"

#define __KERNEL__
#include <sys/api.h>

#define DUMP(v) printk(#v " = %08x\n", v);

struct CONSOLE *cons;

BDF_FONT *bdf;
BDF_FONT *bdfj;

MULTIBOOT_INFO *mboot_info;

struct SHEET *sht_back;

FATFS *ata;

struct FIFO32 ***fifo_list;

struct SHTCTL *shtctl;

unsigned int mdata[4];

#define KEYCMD_LED		0xed

int api_errno = 0;

#define PS2_TAB                      9
#define PS2_ENTER                    13
#define PS2_BACKSPACE                127
#define PS2_ESC                      27
#define PS2_INSERT                   '^'
#define PS2_DELETE                   '~' 
#define PS2_HOME                     'H'
#define PS2_END                      'F'
#define PS2_PAGEUP                   '^'
#define PS2_PAGEDOWN                 '^'
#define PS2_UPARROW                  'A'
#define PS2_LEFTARROW                'D'
#define PS2_DOWNARROW                'B'
#define PS2_RIGHTARROW               'C'

#define BREAK     0x0001
#define MODIFIER  0x0002
#define SHIFT_L   0x0004
#define SHIFT_R   0x0008
#define CAPS      0x0010
#define ALT       0x0020
#define CTRL      0x0040
#define SCROLL    0x0080
#define NUM	      0x0100

const int keytable[2][128] = {
	{
		'\0', '\e', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' , '0' , '-' , '^' , '\b', '\t',
		'q' , 'w' , 'e' , 'r' , 't' , 'y' , 'u' , 'i' , 'o' , 'p' , '@' , '[' , '\n', '\0', 'a' , 's' ,
		'd' , 'f' , 'g' , 'h' , 'j' , 'k' , 'l' , ';' , ':' , '`' , '\0', ']' , 'z' , 'x' , 'c' , 'v' ,
		'b' , 'n' , 'm' , ',' , '.' , '/' , '\0', '*' , '\0', ' ' , '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '7' , '8' , '9' , '-' , '4' , '5' , '6' , '+' , '1' ,
		'2' , '3' , '0' , '.' , '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '_' , '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\\', '\0', '\0',
	},
	{
		'\0', '\e', '!' , '\"', '#' , '$' , '%' , '&' , '\'', '(' , ')' , '\0', '=' , '~' , '\b', '\t',
		'Q' , 'W' , 'E' , 'R' , 'T' , 'Y' , 'U' , 'I' , 'O' , 'P' , '`' , '{' , '\n', '\0', 'A' , 'S' ,
		'D' , 'F' , 'G' , 'H' , 'J' , 'K' , 'L' , '+' , '*' , '`' , '\0', '}' , 'Z' , 'X' , 'C' , 'V' ,
		'B' , 'N' , 'M' , '<' , '>' , '?' , '\0', '*' , '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '-' , '\0', '\0', '\0', '+' , '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '_' , '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\\', '\0', '\0',
	}
};

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 3); /* コンソールのカーソルOFF */
	}
	key_win->act = 0;
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 2); /* コンソールのカーソルON */
	}
	key_win->act = 1;
	return;
}

void fork_exit(void)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	io_cli();
	fifo32_put(fifo, task - taskctl->tasks0 + 1024);	/* 1024〜2023 */
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

void fork_task(char *path)
{
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	char *p, *q;
	struct TASK *task = task_now();
	UINT dmy;
	FIL *fd;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	FIL *fhandle[8];
	
	fd = (FIL *) memman_alloc_4k(memman, sizeof(FIL));
	
	for(i = 0; i < 8; i++) fhandle[i] = 0;
	
	int r = f_open(fd,path,FA_READ);
	
	if(r == FR_OK) {
		appsiz = f_size(fd);
		p = (char *) memman_alloc_4k(memman, appsiz);
		f_read(fd, p, appsiz, &dmy);
		
		if (appsiz >= 36 && strncmp(p + 4, "MicX", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			task->cs_base = (int) p;
			task->fhandle = fhandle;
			
			int malloc_ctl = *((int *)&p[0x20]);
			int malloc_adr = *((int *)&p[0x20])+32*1024;
			int malloc_siz = *((int *)&p[0x00])-malloc_adr;
			
			memman_init((struct MEMMAN *) &q[malloc_ctl]);
			malloc_siz &= 0xfffffff0;
			memman_free((struct MEMMAN *) &q[malloc_ctl], malloc_adr, malloc_siz);
			
			//DUMP(segsiz);
			//DUMP(esp);
			//DUMP(datsiz);
			//DUMP(dathrb);
			
			set_segmdesc(task->ldt + 0, appsiz + 1024 - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz + 1024 - 1, (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			
			task->isapp = 1;
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			task->isapp = 0;
			
			struct SHEET *sht;
	
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					sheet_free(sht);
				}
			}
			
			for (i = 0; i < 8; i++) {
				if(task->fhandle[i]) {
					f_close(task->fhandle[i]);
				}
			}
			
			memman_free_4k(memman, (int) q, segsiz);
		} else {
			printk("%s is not executable file\n", path);
		}
		memman_free_4k(memman, (int) p, appsiz);
	} else {
		printk("%s not found\n", path);
	}
	
	for(int t = 0; t < FIFOTYPE_NUM; t++)
		for(int l = 0; l < 1024; l++) if(fifo_list[t][l] == &task->fifo) fifo_list[t][l] = 0;
	
	printk("killed\n");
	
	fork_exit();
}

struct TASK *fork(char *path)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *forktask = task_alloc();
	int *fork_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	forktask->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	forktask->tss.eip = (int) &fork_task;
	forktask->tss.es = 1 * 8;
	forktask->tss.cs = 2 * 8;
	forktask->tss.ss = 1 * 8;
	forktask->tss.ds = 1 * 8;
	forktask->tss.fs = 1 * 8;
	forktask->tss.gs = 1 * 8;
	*((char **) (forktask->tss.esp + 4)) = path;
	task_run(forktask, 1, 2);
	fifo32_init(&(forktask->fifo), 128, fork_fifo, forktask);
}

void kill_fork(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	io_cli();
	task->flags = 0;
	if (taskctl->task_fpu == task) {
		taskctl->task_fpu = 0;
	}
	io_sti();
	task->flags = 0;
	return;
}

void _kernel_entry(UINT32 magic, MULTIBOOT_INFO *info)
{
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	struct TASK *task_a, *task;
	int key_shift = 0, key_leds = 0, keycmd_wait = -1;
	int i, j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
	int mx, my, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	struct SHEET *sht, *key_win;
	
	struct MOUSE_DEC mdec;
	
	struct FIFO32 *fifolist_buf[FIFOTYPE_NUM][1024];
	
	FATFS fs;
	
	mboot_info = info;
	
	MAX_X = info->framebuffer_width / 8;
	MAX_Y = info->framebuffer_height / 8;
	
	init_gdtidt(info);
	init_pic();
	io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xb8); /* PITとPIC1とキーボードを許可(10111000) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int memtotal = info->mem_upper;
	memman_init(memman);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	
	fifo_list = fifolist_buf;
	
	cons = (struct CONSOLE *) memman_alloc_4k(memman, sizeof(struct CONSOLE));
	
	cons_initalize(cons);
	
	graphic_init();
	
	//set_palette(0,16,table_rgb);
	
	find_pci_device();
	init_ata_disk_driver();
	
	ata = &fs;
	
	f_mount(&fs,"",1);
	
	printf("Loading ASCII BDF font...\n");
	bdf = bdfReadPath("6x12.bdf");
	printf("Loading JIS BDF font...\n");
	bdfj = bdfReadPath("k12.bdf");
	
	cons_initalize(cons);
	
	bdfSetDrawingFunction(bdfDot);
	
	shtctl = shtctl_init((unsigned int *)(info->framebuffer_addr[0]),info->framebuffer_width,info->framebuffer_height);
	
	//struct SHEET *sht = sheet_alloc(shtctl);
	
	//sheet_setbuf(sht,(unsigned int *)malloc(256*256*4),256,256,0xff000000);
	
	//dot_x = sht;
	
	/*unsigned int *fbp = (unsigned int *)sht->buf;
	
	for(int i = 0; i < 256; i++) {
		for(int j = 0; j < 256; j++) {
			//int col = 0x010101 * (128 - i*4);
			int col = (i << 8) | j;
			fbp[i*256+j] = col;
		}
	}*/
	
	sht_back = sheet_alloc(shtctl);
	
	sht_back->flag2 = 1;
	
	sheet_setbuf(sht_back,(unsigned int *)malloc(info->framebuffer_width*info->framebuffer_height*4),info->framebuffer_width,info->framebuffer_height,-1);
	
	for(int x = 0; x < info->framebuffer_width; x++) {
		for(int y = 0; y < info->framebuffer_height; y++) {
			float fx = ((float)x / (float)info->framebuffer_width) * 256.0f;
			float fy = ((float)y / (float)info->framebuffer_height) * 256.0f;
			int fc = (((int)fy) << 8) | ((int)fx);
			sht_back->buf[y * sht_back->bxsize + x] = fc;
		}
	}
	
	sheet_updown(sht_back,0);
	sheet_slide(sht_back,0,0);
	
	sheet_refresh(sht_back,0,0,info->framebuffer_width,info->framebuffer_height);
	
	struct SHEET *mouse = sheet_alloc(shtctl);
	
	{
		int w,h,bpp;
		stbi_uc *mgrp = stbi_load("cursor.png",&w,&h,&bpp,4);
		swaprgb((unsigned int*)mgrp, w, h);
		
		sheet_setbuf(mouse,(unsigned int *)mgrp,w,h,0xff007f7f);
	}
	
	sheet_updown(mouse,3);
	sheet_slide(mouse,0,0);
	
	sheet_refresh(mouse,0,0,128,128);
	
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@", "OQQQQQQQQQQQQQ$@", "OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@", "OQQQQ@@QQ@@QQQ$@", "OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@", "OQQQQQ@@@@QQQQ$@", "OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@", "OQQQQQQQQQQQQQ$@", "OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@", "@@@@@@@@@@@@@@@@"};
	
	/*for(i=0;i<2;i++){
		struct SHEET *sht_win = sheet_alloc(shtctl);
	
		sheet_setbuf(sht_win,(unsigned int *)malloc(256*256*4),256,256,-1);
		
		make_window(sht_win,"MicroX First Window");
		
		sheet_slide(sht_win,32+i*16,32+i*16);
		sheet_updown(sht_win, shtctl->top);
	}*/
	
	//bdfPrintString(bdf,4,5,"MicroX First Window");
	
	//printf("%08x\n",bdf->info.chars);
	
	/*int w,h,bpp;
	
	stbi_uc *b = stbi_load("pic.png",&w,&h,&bpp,4);
	
	swaprgb((unsigned int*)b, w, h);
	
	memcpy(sht->buf,b,256*256*4);*/
	
	//bdfSetDrawingAreaSize(256,256);
	//bdfSetDrawingFunction(dot);
	
	//bdfPrintString(bdf,0,0,"MicroX First BDF Message!!\nWindows 10");
	//char s[32];
	//sprintf(s,"%08x",b);
	//bdfPrintString(bdf,0,0,s);
	
	//io_out16(0x03ce,0x0005);
	//io_out16(0x03ce,0x0406);
	//io_out16(0x03c4,0x0402);
	//io_out16(0x03c4,0x0604);
	
	/*for(i = 0; i < 256; i++) {
		for(j = 0; j < 16; j++) {
			*((unsigned char *)((i * 32 + j) + 0xa0000)) = vga_font[i * 16 + j];
		}
	}*/
	
	//io_out16(0x03c4,0x0302);
	//io_out16(0x03c4,0x0204);
	//io_out16(0x03ce,0x1005);
	//io_out16(0x03ce,0x0e06);
	
	for(int t = 0; t < FIFOTYPE_NUM; t++)
		for(int l = 0; l < 1024; l++) fifo_list[t][l] = 0;
	
	fork("init.eim");
	fork("init.eim");
	
	int ps2_skip = 0;
	int ps2_state = 0;
	
	while(1) {
		if(fifo32_status(&fifo) != 0) {
			i = fifo32_get(&fifo);
			if (1024 <= i && i <= 2023) {
				kill_fork(taskctl->tasks0 + (i - 1024));
			}
			if (256 <= i && i <= 511) {
				int c = 0;
				char *esc = 0;
				int d = i - 256;
				if(d == 0xe0) {
					ps2_state |= MODIFIER;
				} else if (d >= 0x80) {
					d = d - 0x80;
					if(d == 0x2a) {
						ps2_state &= ~SHIFT_L;
					} else if(d == 0x36) {
						ps2_state &= ~SHIFT_R;
					} else if(d == 0x38) {
						ps2_state &= ~ALT;
					} else if(d == 0x1d) {
						ps2_state &= ~CTRL;
					}
					ps2_state &= ~MODIFIER;
				} else {
					if(d == 0x2a) {
						ps2_state |= SHIFT_L;
					} else if(d == 0x36) {
						ps2_state |= SHIFT_R;
					} else if(d == 0x38) {
						ps2_state |= ALT;
					} else if(d == 0x1d) {
						ps2_state |= CTRL;
					} else if(d == 0x3a && (ps2_state & (SHIFT_L |SHIFT_R))) {
						ps2_state ^= CAPS;
					} else if(d == 0x45) {
						ps2_state ^= NUM;
					} else if(d == 0x46) {
						ps2_state ^= SCROLL;
					}
					if((ps2_state & MODIFIER) || (d >= 0x47 && d <= 0x53 && ((ps2_state & NUM) == 0 || (ps2_state & (SHIFT_L |SHIFT_R))))) {
						esc = "\e[";
						switch (d) {
							case 0x52:
								esc = "\e[2";
								c = PS2_INSERT;
								break;
							case 0x47:
								c = PS2_HOME;
								break;
							case 0x49:
								esc = "\e[5";
								c = PS2_PAGEUP;
								break;
							case 0x53:
								esc = "\e[3";
								c = PS2_DELETE;
								break;
							case 0x4f:
								c = PS2_END;
								break;
							case 0x51:
								esc = "\e[6";
								c = PS2_PAGEDOWN;
								break;
							case 0x48:
								c = PS2_UPARROW;
								break;
							case 0x4b:
								c = PS2_LEFTARROW;
								break;
							case 0x50:
								c = PS2_DOWNARROW;
								break;
							case 0x4d:
								c = PS2_RIGHTARROW;
								break;
							case 0x35:
								c = '/';
								esc = 0;
								break;
							case 0x1c:
								c = PS2_ENTER;
								break;
							default:
								break;
						}
					} else {
						int addr = (ps2_state & (SHIFT_L | SHIFT_R)) ? 1 : 0;
						c = keytable[addr][d];
						if(ps2_state & CAPS)
							if(c >= 'a' && c <= 'z') c -= 0x20;
						if((d >= 0x3b && d <= 0x44) || d == 0x57 || d == 0x58) {
							int fx = d >= 0x57 ? d - 0x57 + 10 : d - 0x3b;
							esc = fx > 8 ? "\e2" : "\e1";
							c = "123457890134"[fx];
						}
					}
					if(ps2_state & CTRL) {
						if(c >= 'a' && c <= 'z') c = c - 'a' + 1;
					}
					ps2_state &= ~MODIFIER;
				}
				
				struct FIFO32 *fifo = &key_win->task->fifo;
				char *p = esc;
				if(!fifo->task->flags) continue;
				while(*p && p) {
					fifo32_put(fifo, FIFOTYPE_KEYBOARD);
					fifo32_put(fifo, *p++);
				}
				if(c) {
					fifo32_put(fifo, FIFOTYPE_KEYBOARD);
					fifo32_put(fifo, c);
				}
				
				/*struct FIFO32 **fp = fifo_list[FIFOTYPE_KEYBOARD];
				for(i = 0; i < 1024; i++) {
					if(fp[i]) {
						struct FIFO32 *fifo = fp[i];
						char *p = esc;
						if(!fifo->task->flags) continue;
						while(*p && p) {
							fifo32_put(fifo, FIFOTYPE_KEYBOARD);
							fifo32_put(fifo, *p++);
						}
						if(c) {
							fifo32_put(fifo, FIFOTYPE_KEYBOARD);
							fifo32_put(fifo, c);
						}
					}
				}*/
				
				// CNS 421
				key_leds = 0;
				key_leds |= (ps2_state & CAPS) * 4;
				key_leds |= (ps2_state & NUM) * 2;
				key_leds |= (ps2_state & SCROLL) * 1;
				fifo32_put(&keycmd, KEYCMD_LED);
				fifo32_put(&keycmd, key_leds);
			} else if (512 <= i && i <= 767) { /* マウスデータ */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* マウスカーソルの移動 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > info->framebuffer_width - 1) {
						mx = info->framebuffer_width - 1;
					}
					if (my > info->framebuffer_height - 1) {
						my = info->framebuffer_height - 1;
					}
					
					mdata[0] = mx;
					mdata[1] = my;
					new_mx = mx;
					new_my = my;
					
					//sheet_slide(mouse, new_mx, new_my);
//#if 0
					if ((mdec.btn & 0x01) != 0) {
						/* 左ボタンを押している */
						mdata[2] = 1;
						if (mmx < 0) {
							/* 通常モードの場合 */
							/* 上の下じきから順番にマウスが指している下じきを探す */
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv && sht->flag2 == 0) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											if(key_win->act == 1) keywin_off(key_win);
											key_win = sht;
											if(key_win->act == 0) keywin_on(key_win);
										}
										if ((3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) && sht->flag2 == 0) {
											mmx = mx;	/* ウィンドウ移動モードへ */
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
												/* 「×」ボタンクリック */
											if ((sht->flags & 0x10) != 0) {		/* アプリが作ったウィンドウか？ */
												task = sht->task;
												io_cli();	/* 強制終了処理中にタスクが変わると困るから */
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
												task_run(task, -1, 0);
											} else {	/* コンソールなど */
												task = sht->task;
												sheet_updown(sht, -1); /* とりあえず非表示にしておく */
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							/* ウィンドウ移動モードの場合 */
							mdata[2] = 0;
							x = mx - mmx;	/* マウスの移動量を計算 */
							y = my - mmy;
							new_wx = mmx2 + x;
							new_wy = new_wy + y;
							mmy = my;	/* 移動後の座標に更新 */
						}
					} else if (mdec.btn & 0x02) {
						/* 右ボタン */
						mdata[3] = 1;
					} else {
						/* 左ボタンを押していない */
						mdata[2] = 0;
						mdata[3] = 0;
						mmx = -1;	/* 通常モードへ */
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);	/* 一度確定させる */
							new_wx = 0x7fffffff;
						}
					}
//#endif
				}
			}
		} else {
			if (new_mx >= 0) {
				//io_sti();
				sheet_slide(mouse, new_mx, new_my);
				//printf("MOUSE\n");
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				//io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
			}
		}
	}
}

void *app_malloc(int siz)
{
	struct TASK *task = task_now();
	char *p = (char *)task->cs_base;
	int malloc_ctl = *((int *)&p[0x20]);
	int malloc_adr = *((int *)&p[0x20])+32*1024;
	int malloc_siz = *((int *)&p[0x00])-malloc_adr;
	struct MEMMAN *memman = (struct MEMMAN *)(malloc_ctl + task->ds_base);
	
	char *q = (char *)memman_alloc(memman, siz+4);
	
	*((int *)q) = siz+4;
	
	return q + 4;
}

void *app_free(void *ptr)
{
	struct TASK *task = task_now();
	char *p = (char *)task->cs_base;
	int malloc_ctl = *((int *)&p[0x20]);
	int malloc_adr = *((int *)&p[0x20])+32*1024;
	int malloc_siz = *((int *)&p[0x00])-malloc_adr;
	struct MEMMAN *memman = (struct MEMMAN *)(malloc_ctl + task->ds_base);
	
	char *q = ((char *)ptr) - 4;
	
	memman_free(memman, q, *((int *)q));
}

int *microx_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	struct TASK *task_cons;
	int ds_base = task->ds_base;
	int *reg = &eax + 1;
	
	struct FIFO32 *fifo = &task->fifo;
	
	int *p = (int *)(ds_base + eax);
	
	int id = p[0];
	
	//printf("API 0x%02x", id);
	
	if(id == mx32api_putchar) {
		putchar(p[1] & 0xff);
	} else if(id == mx32api_exit) {
		//printk("EXIT");
		return &(task->tss.esp0);
	} else if(id == mx32api_fifo32_status) {
		p[1] = fifo32_status(fifo);
	} else if(id == mx32api_fifo32_put) {
		p[2] = fifo32_put(fifo,p[1]);
	} else if(id == mx32api_fifo32_get) {
		p[1] = fifo32_get(fifo);
	} else if(id == mx32api_fifo_typeenable) {
		int type = p[1];
		struct FIFO32 **fp = fifo_list[type];
		for(int i = 0; i < 1024; i++) {
			if(fp[i] == 0) {
				fp[i] = fifo;
				break;
			}
		}
	} else if(id == mx32api_fifo_typedisable) {
		int type = p[1];
		struct FIFO32 **fp = fifo_list[type];
		for(int i = 0; i < 1024; i++) {
			if(fp[i] == fifo) {
				fp[i] = 0;
				break;
			}
		}
	} else if(id == mx32api_open) {
		int i;
		for(i = 0; i < 8; i++) {
			if(task->fhandle[i] == 0) break;
		}
		if(i != 8) {
			FIL *fp = app_malloc(sizeof(FIL));
			int r = f_open(fp,p[1]+ds_base,p[2]);
			p[15] = r != FR_OK ? -1 : fp;
			if(r != FR_OK) app_free(fp);
			api_errno = r;
			task->fhandle[i] = fp;
		} else {
			p[15] = -1;
			api_errno = -1;
		}
	} else if(id == mx32api_close) {
		FIL *fp = (FIL *)p[1];
		int r = f_close(fp);
		int i;
		
		for(i = 0; i < 8; i++) {
			if(task->fhandle[i] == fp) task->fhandle[i] = 0;
		}
		
		api_errno = r;
		p[15] = r;
		app_free(fp);
	} else if(id == mx32api_read) {
		FIL *fp = (FIL *)p[1];
		UINT br;
		int r = f_read(fp,p[2]+ds_base,p[3],&br);
		api_errno = r;
		p[15] = br;
	} else if(id == mx32api_write) {
		FIL *fp = (FIL *)p[1];
		UINT bw;
		int r = f_write(fp,p[2]+ds_base,p[3],&bw);
		api_errno = r;
		p[15] = bw;
	} else if(id == mx32api_lseek) {
		FIL *fp = (FIL *)p[1];
		int ofs;
		ofs = p[2];
		if(p[3] == 0x01) ofs = (int)f_tell(fp) + ofs;
		if(p[3] == 0x02) ofs = (int)f_size(fp) - ofs;
		int r = f_lseek(fp, ofs);
		api_errno = r;
		p[15] = r;
	} else if(id == mx32api_tell) {
		FIL *fp = (FIL *)p[1];
		api_errno = 0;
		p[15] = f_tell(fp);
	} else if(id == mx32api_errno) {
		p[15] = api_errno;
	} else if(id == mx32api_malloc) {
		p[15] = app_malloc(p[1]);
	} else if(id == mx32api_free) {
		p[15] = app_free(p[1]);
	} else if(id == mx32api_createwindow) {
		struct SHEET *sht = sheet_alloc(shtctl);
		sheet_setbuf(sht, (unsigned int *)(((unsigned int *)p)[1] + ds_base), p[2], p[3], p[4]);
		
		//printf("%08x %d %d %d %s\n",p[1],p[2],p[3],p[4],p[5] + ds_base);
		
		make_window(sht, p[5] + ds_base);
		
		sheet_slide(sht,32,32);
		sheet_updown(sht, shtctl->top);
		
		sht->task = task;
		sht->flags |= 0x10;
		sht->flag2 = 0;
		
		p[15] = sht;
	} else if(id == mx32api_closewindow) {
		struct SHEET *sht = p[1];
		sheet_free(sht);
	} else if(id == mx32api_textout) {
		struct SHEET *sht = p[1];
		textout(sht,p[2],p[3],p[4],p[5] + ds_base);
	} else if(id == mx32api_sleep) {
		task_sleep(task);
	} else if(id == mx32api_refreshwindow) {
		struct SHEET *sht = p[1];
		sheet_refresh(sht,p[2],p[3],p[4],p[5]);
	}
	
	
	return 0;
}

extern struct File_methods mth;

FILE *__open(char *path, char *mode)
{
	FILE *f = (FILE *)malloc(sizeof(FILE));
	
	BYTE m = 0;
	if(strchr(mode,'r')) m |= FA_READ;
	if(strchr(mode,'w')) m |= FA_WRITE;
	
	//printk("%02x\n",m);
	
	f->obj = (FIL *)malloc(sizeof(FIL));
	
	f->vmt = &mth;
	
	f_open((FIL *)f->obj,path,m);
	
	return f;
}

int *inthandler0c(int *esp) {
	struct TASK *task = task_now();
	printf("\nINT 0C :\n Stack Exception.\n");
	printf("EIP = %08X\n", esp[11]);
	if(!task->isapp) {
		printf("*FATAL* IN OS!!\n");
		while(1);
	}
	return &(task->tss.esp0); /* 異常終了させる */
}

int *inthandler0d(int *esp) {
	struct TASK *task = task_now();
	printf("\nINT 0D :\n General Protected Exception.\n");
	printf("EIP = %08X\n", esp[11]);
	if(!task->isapp) {
		printf("*FATAL* IN OS!!\n");
		while(1);
	}
	return &(task->tss.esp0); /* 異常終了させる */
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
	struct TASK *task = task_now();
	int i;
	switch((int)instance->obj) {
	case 1:
		for(i = 0; i < n; i++) fifo32_put(&(task->fifo),bp[i]);
		return i;
	case 2:
		cons_putstr1(cons, bp, n);
		return n;
	}
	
	UINT bw;
	f_write((FIL *)instance->obj, bp, n, &bw);
	
	return bw;
}

size_t __read(FILE* instance, char *bp, size_t n)
{
	struct TASK *task = task_now();
	int i;
	switch((int)instance->obj) {
	case 1:
		for(i = 0; i < n && fifo32_status(&(task->fifo)); i++) bp[i] = fifo32_get(&(task->fifo));
		return i;
	case 2:
		return 0;
	}
	
	UINT br;
	f_read((FIL *)instance->obj, bp, n, &br);
	
	return br;
}

int __close(FILE* instance)
{
	switch((int)instance->obj) {
	case 1:
		return -1;
	case 2:
		return -1;
	}
	
	return f_close(instance->obj);
}

int __seek(FILE* instance, size_t offset, int base)
{
	switch((int)instance->obj) {
	case 1:
		return -1;
	case 2:
		return -1;
	}
	
	int fofs = offset;
	
	switch(base) {
	case SEEK_CUR:
		fofs = f_tell((FIL *)instance->obj) + offset;
		break;
	case SEEK_END:
		fofs = f_size((FIL *)instance->obj) - offset;
		break;
	}
	
	//printk("SEEK %d:%d (%d)\n", base, offset, fofs);
	
	return f_lseek((FIL *)instance->obj, fofs);
}

long __tell(FILE* instance)
{
	switch((int)instance->obj) {
	case 1:
		return -1;
	case 2:
		return -1;
	}
	
	return f_tell((FIL *)instance->obj);
}

int __eof(FILE *instance)
{
	return f_eof((FIL *)instance->obj);
}

void *__malloc(size_t siz)
{
	return kmalloc(siz);
}

void __free(void *ptr)
{
	kfree(ptr);
}

void __assert_fail(const char *s, const char *f, unsigned int l)
{
	printf("ASSERT FAILED %s : %s:%d\n",s,f,l);
	for(;;);
}

int pow(int x, int n)
{
	if(n == 2) return x*x;
	return pow(x,n-1); 
}

double ldexp(double x, int exp)
{
	return x * pow(2,exp);
}
