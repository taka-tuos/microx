#include "multiboot.h"
#include "ata.h"
#include "pci.h"
#include "memory.h"
#include "console.h"
#include "bootpack.h"
#include "ff.h"

#define __KERNEL__
#include <libmicrox.h>

#define DUMP(v) printk(#v " = %08x\n", v);

struct CONSOLE *cons;

FATFS *ata;

struct FIFO32 ***fifo_list;

#define KEYCMD_LED		0xed

int api_errno = 0;

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
	
	fd = (FIL *) memman_alloc_4k(memman, sizeof(FIL));
	
	int r = f_open(fd,path,FA_READ);
	
	if(r == FR_OK) {
		appsiz = f_size(fd);
		p = (char *) memman_alloc_4k(memman, appsiz);
		f_read(fd, p, appsiz, &dmy);
		
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			task->cs_base = (int) p;
			
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
			
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
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
	
	//printk("killed\n");
	
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

const unsigned char table_rgb[16 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xaa,
	0x00, 0xaa, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0x00, 0xaa,
	0xaa, 0xaa, 0x00,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0x55, 0x55, 0xff,
	0x55, 0xff, 0x55,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0x55,
	0xff, 0xff, 0xff,
};

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


void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	io_cli();
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_sti();
	return;
}

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

void _kernel_entry(UINT32 magic, MULTIBOOT_INFO *info)
{
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	struct TASK *task_a, *task;
	int key_shift = 0, key_leds = 0, keycmd_wait = -1;
	
	struct FIFO32 *fifolist_buf[FIFOTYPE_NUM][1024];
	
	FATFS fs;
	
	init_gdtidt(info);
	init_pic();
	io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可(11111000) */
	io_out8(PIC1_IMR, 0xff); /* 割り込み禁止(11111111) */
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
	
	set_palette(0,16,table_rgb);
	
	io_out16(0x03c4,0x0100);
	
	io_out8(0x03c2,0xe3);
	io_out8(0x03c3,0x01);
	
	int i;
	int x3c4_data[] = { 0x01, 0x0f, 0x00, 0x06 };
	int x3d4_dataA[] = { 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e };
	int x3d4_dataB[] = { 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int x3d4_dataC[] = { 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff };
	int x3ce_data[] = { 0x00, 0x0f, 0x00, 0x00, 0x00, 0x03, 0x05, 0x00, 0xff };
	int x3c0_dataA[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	int x3c0_dataB[] = { 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	int x3c0_dataC[] = { 0x01, 0x00, 0x0f, 0x00, 0x00 };
	
	for(i = 0x01; i <= 0x04; i++) io_out16(0x03c4,i | (x3c4_data[i-1] << 8));
	
	io_out16(0x03c4,0x0300);
	
	io_out16(0x03d4,0x2011);
	
	for(i = 0x00; i <= 0x07; i++) io_out16(0x03d4,i | (x3d4_dataA[i] << 8));
	for(i = 0x08; i <= 0x0f; i++) io_out16(0x03d4,i | (x3d4_dataB[i-0x08] << 8));
	for(i = 0x10; i <= 0x18; i++) io_out16(0x03d4,i | (x3d4_dataC[i-0x10] << 8));
	
	for(i = 0x00; i <= 0x08; i++) io_out16(0x03ce,i | (x3ce_data[i] << 8));
	
	for(i = 0x00; i <= 0x07; i++) {
		int dmy = io_in8(0x3da);
		io_out8(0x3c0, i|0x20);
		io_out8(0x3c0, x3c0_dataA[i]);
	}
	
	for(i = 0x08; i <= 0x0f; i++) {
		int dmy = io_in8(0x3da);
		io_out8(0x3c0, i|0x20);
		io_out8(0x3c0, x3c0_dataB[i-0x08]);
	}
	
	for(i = 0x10; i <= 0x14; i++) {
		int dmy = io_in8(0x3da);
		io_out8(0x3c0, i|0x20);
		io_out8(0x3c0, x3c0_dataC[i-0x10]);
	}
	
	io_out8(0x3c6, 0xff);
	io_out16(0x3ce, 0x0305);
	
	memset(0xa0000,0,640*480/8);
	
	for(int t = 0; t < FIFOTYPE_NUM; t++)
		for(int l = 0; l < 1024; l++) fifo_list[t][l] = 0;
	
	find_pci_device();
	init_ata_disk_driver();
	
	ata = &fs;
	
	f_mount(&fs,"",1);
	
	fork("init.eim");
	
	int ps2_skip = 0;
	int ps2_state = 0;
	
	while(1)
	{
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
				
				struct FIFO32 **fp = fifo_list[FIFOTYPE_KEYBOARD];
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
				}
				
				// CNS 421
				key_leds = 0;
				key_leds |= (ps2_state & CAPS) * 4;
				key_leds |= (ps2_state & NUM) * 2;
				key_leds |= (ps2_state & SCROLL) * 1;
				fifo32_put(&keycmd, KEYCMD_LED);
				fifo32_put(&keycmd, key_leds);
			}
		} else {
			task_sleep(task_a);
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
	
	if(id == mx32api_putchar) {
		printk("%c",p[1] & 0xff);
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
		FIL *fp = app_malloc(sizeof(FIL));
		int r = f_open(fp,p[1]+ds_base,p[2]);
		p[15] = r != FR_OK ? -1 : fp;
		if(r != FR_OK) app_free(fp);
		api_errno = r;
	} else if(id == mx32api_close) {
		FIL *fp = (FIL *)p[1];
		int r = f_close(fp);
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
	}
	
	return 0;
}
