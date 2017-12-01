#include "multiboot.h"
#include "ata.h"
#include "pci.h"
#include "memory.h"
#include "console.h"
#include "bootpack.h"
#include "ff.h"

#include <libmicrox.h>

struct CONSOLE *cons;

FATFS *ata;

void fork_sub(char *fname)
{
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	char *p, *q;
	struct TASK *task = task_now();
	UINT dmy;
	FIL *fd;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	fd = (FIL *) memman_alloc_4k(memman, sizeof(FIL));
	
	f_open(fd,fname,FA_READ);
	
	printk("fd = %d\n",fd);
	
	//while(1);
	
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
		
		printk("ldt = %08x\n\n",task->ldt);
		
		printk("segsiz = %08x\n",segsiz);
		printk("esp = %08x\n",esp);
		printk("datsiz = %08x\n",datsiz);
		printk("dathrb = %08x\n",dathrb);
		
		set_segmdesc(task->ldt + 0, appsiz + 1024 - 1, (int) p, AR_CODE32_ER + 0x60);
		set_segmdesc(task->ldt + 1, segsiz + 1024 - 1, (int) q, AR_DATA32_RW + 0x60);
		for (i = 0; i < datsiz; i++) {
			q[esp + i] = p[dathrb + i];
		}
		
		//printk("dathrb = %08x\n",dathrb);
		
		unsigned char *up = (unsigned char *)p;
		
		for (i = 0; i < appsiz; i+=16) {
			for (int j = 0; j < 16; j++) if(appsiz - i < j) printk("   "); else printk("%02X ",up[i+j]);
				for (int j = 0; j < 16; j++) if(appsiz - i < j) printk(" "); else printk("%c",up[i+j] < 0x20 ? '.' : up[i+j]);
			printk("\n");
		}
		
		start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
		memman_free_4k(memman, (int) q, segsiz);
	} else {
		printk(".eim file format error\n");
	}
	memman_free_4k(memman, (int) p, appsiz);
}

void _kernel_entry(UINT32 magic, MULTIBOOT_INFO *info)
{
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	struct TASK *task_a, *task;
	
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
	//memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	
	//memset(0xb8000,0x07,80*25*2);
	
	cons = (struct CONSOLE *) memman_alloc_4k(memman, sizeof(struct CONSOLE));
	
	//fd = (FIL *) memman_alloc_4k(memman, sizeof(FIL));
	
	char s[60];
	
	sprintf(s, "CS:%08x SS:%08x\n", read_cs(), read_ss());
	cons_putstr0(cons, s);
	
	char b[512];
	
	find_pci_device();
	//show_all_pci_device();
	init_ata_disk_driver();
	
	ata = &fs;
	
	printk("ATA DONE\n");
	
	int r = f_mount(&fs,"",1);
	
	printk("mount %d\n", r);
	
	struct TASK *forktask = task_alloc();
	int *fork_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	forktask->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	forktask->tss.eip = (int) &fork_sub;
	forktask->tss.es = 1 * 8;
	forktask->tss.cs = 2 * 8;
	forktask->tss.ss = 1 * 8;
	forktask->tss.ds = 1 * 8;
	forktask->tss.fs = 1 * 8;
	forktask->tss.gs = 1 * 8;
	*((char **) (forktask->tss.esp + 4)) = "app.eim";
	task_run(forktask, 1, 2);
	fifo32_init(&(forktask->fifo), 128, fork_fifo, forktask);
	
	while(1);
}

int *microx_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	struct TASK *task_cons;
	int ds_base = task->ds_base;
	int *reg = &eax + 1;
	
	int *p = (int *)(ds_base + eax);
	int *q = (int *)(ds_base + ecx);
	
	//printk("p = %08x\n",eax);
	//printk("q = %08x\n",ecx);
	
	//printk("p[0] = %d\n", p[0]);
	//printk("p[1] = %d\n", p[1]);
	
	if(p[0] == 0) printk("%c",p[1] & 0xff);
	
	return 0;
}
