#define IDE_IMPLEMENT
#include "ide.h"
#include "wrapper.h"

unsigned char ide_read(unsigned char channel, unsigned char reg)
{
	unsigned char result;
	if		(reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	if		(reg < 0x08) result = inb(channels[channel].base  + reg - 0x00);
	else if	(reg < 0x0C) result = inb(channels[channel].base  + reg - 0x06);
	else if	(reg < 0x0E) result = inb(channels[channel].ctrl  + reg - 0x0A);
	else if	(reg < 0x16) result = inb(channels[channel].bmide + reg - 0x0E);
	if		(reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	return result;
}

void ide_write(unsigned char channel, unsigned char reg, unsigned char data)
{
	if		(reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	if		(reg < 0x08) outb(channels[channel].base  + reg - 0x00, data);
	else if	(reg < 0x0C) outb(channels[channel].base  + reg - 0x06, data);
	else if	(reg < 0x0E) outb(channels[channel].ctrl  + reg - 0x0A, data);
	else if	(reg < 0x16) outb(channels[channel].bmide + reg - 0x0E, data);
	if		(reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads)
{
	if		(reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
	if		(reg < 0x08) insl(channels[channel].base  + reg - 0x00, buffer, quads);
	else if	(reg < 0x0C) insl(channels[channel].base  + reg - 0x06, buffer, quads);
	else if	(reg < 0x0E) insl(channels[channel].ctrl  + reg - 0x0A, buffer, quads);
	else if	(reg < 0x16) insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
	if		(reg > 0x07 && reg < 0x0C) ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_polling(unsigned char channel, unsigned int advanced_check)
{

	// (I) Delay 400 nanosecond for BSY to be set:
	// -------------------------------------------------
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

	// (II) Wait for BSY to be cleared:
	// -------------------------------------------------
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait for BSY to be zero.

	if (advanced_check) {

		unsigned char state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

		// (III) Check For Errors:
		// -------------------------------------------------
		if (state & ATA_SR_ERR) return 2; // Error.

		// (IV) Check If Device fault:
		// -------------------------------------------------
		if (state & ATA_SR_DF ) return 1; // Device Fault.

		// (V) Check DRQ:
		// -------------------------------------------------
		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
		if (!(state & ATA_SR_DRQ)) return 3; // DRQ should be set

	}

	return 0; // No Error.

}

unsigned char ide_print_error(unsigned int drive, unsigned char err)
{

	if (err == 0) return err;

	printk(" IDE:");
	if (err == 1) {printk("- Device Fault\n     "); err = 19;}
	else if (err == 2) {
		unsigned char st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
		if (st & ATA_ER_AMNF)	{printk("- No Address Mark Found\n");		err = 7;}
		if (st & ATA_ER_TK0NF)	{printk("- No Media or Media Error\n");		err = 3;}
		if (st & ATA_ER_ABRT)	{printk("- Command Aborted\n");				err = 20;}
		if (st & ATA_ER_MCR)	{printk("- No Media or Media Error\n");		err = 3;}
		if (st & ATA_ER_IDNF)	{printk("- ID mark not Found\n");			err = 21;}
		if (st & ATA_ER_MC)		{printk("- No Media or Media Error\n");		err = 3;}
		if (st & ATA_ER_UNC)	{printk("- Uncorrectable Data Error\n");	err = 22;}
		if (st & ATA_ER_BBK)	{printk("- Bad Sectors\n     ");			err = 13;}
	} else  if (err == 3)		{printk("- Reads Nothing\n     ");			err = 23;}
	else  if (err == 4)			{printk("- Write Protected\n     ");		err = 8;}
	printk("- [%s %s] %s\n",
	(const char *[]){"Primary","Secondary"}[ide_devices[drive].channel],
	(const char *[]){"Master", "Slave"}[ide_devices[drive].drive],
	ide_devices[drive].model);

	return err;
}

int ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3, unsigned int BAR4)
{
	int i, j, k, count = 0;

	// 1- Detect I/O Ports which interface IDE Controller:
	channels[ATA_PRIMARY  ].base  = (BAR0 &= 0xFFFFFFFC) + 0x1F0*(!BAR0);
	channels[ATA_PRIMARY  ].ctrl  = (BAR1 &= 0xFFFFFFFC) + 0x3F6*(!BAR1);
	channels[ATA_SECONDARY].base  = (BAR2 &= 0xFFFFFFFC) + 0x170*(!BAR2);
	channels[ATA_SECONDARY].ctrl  = (BAR3 &= 0xFFFFFFFC) + 0x376*(!BAR3);
	channels[ATA_PRIMARY  ].bmide = (BAR4 &= 0xFFFFFFFC) + 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = (BAR4 &= 0xFFFFFFFC) + 8; // Bus Master IDE
	
	/*channels[ATA_PRIMARY  ].base  = 0x1F0;
	channels[ATA_PRIMARY  ].ctrl  = 0x3F6;
	channels[ATA_SECONDARY].base  = 0x170;
	channels[ATA_SECONDARY].ctrl  = 0x376;
	channels[ATA_PRIMARY  ].bmide = 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = 8; // Bus Master IDE*/

	// 2- Disable IRQs:
	printk("Disable IRQ\n");
	ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
	ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);
	// 3- Detect ATA-ATAPI Devices:
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			unsigned char err = 0, type = IDE_ATA, status;
			ide_devices[count].reserved   = 0; // Assuming that no drive here.

			// (I) Select Drive:
			ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
			ide_write(i, ATA_REG_SECCOUNT0, 0);
			ide_write(i, ATA_REG_LBA0, 0);
			ide_write(i, ATA_REG_LBA1, 0);
			ide_write(i, ATA_REG_LBA2, 0);
			//sleep(1); // Wait 1ms for drive select to work.

			// (II) Send ATA Identify Command:
			ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			//sleep(1); // This function should be implemented in your OS. which waits for 1 ms. it is based on System Timer Device Driver.

			// (III) Polling:
			status = ide_read(i, ATA_REG_STATUS);
			//printk("%d/%d : %s\n",i,j,status ? "found" : "no device");
			if (!(status)) continue; // If Status = 0, No Device.
			
			//printk("Waiting...");
			while(1) {
				char *strs[] = { "ERR " , "IDX " , "CORR " , "DRQ " , "DSC " , "DF " , "DRDY " , "BSY " };
				//printk("\nstatus : ");
				//for(int b=0;b<8;b++) if(status & (1 << b)) printk(strs[b]);
				if ( (status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
				if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
				status = ide_read(i, ATA_REG_STATUS);
				sleep(1000);
			}
			//printk("done\n");
			
			// (IV) Probe for ATAPI Devices:
			if (err) {
				unsigned char cl = ide_read(i,ATA_REG_LBA1);
				unsigned char ch = ide_read(i,ATA_REG_LBA2);

				if		(cl == 0x14 && ch ==0xEB) type = IDE_ATAPI;
				else if	(cl == 0x69 && ch ==0x96) type = IDE_ATAPI;
				else continue; // Unknown Type (And always not be a device).

				ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				//sleep(1);
			}

			// (V) Read Identification Space of the Device:
			ide_read_buffer(i, ATA_REG_DATA, (unsigned int) ide_buf, 128);

			// (VI) Read Device Parameters:
			ide_devices[count].reserved		= 1;
			ide_devices[count].type			= type;
			ide_devices[count].channel		= i;
			ide_devices[count].drive		= j;
			ide_devices[count].sign			= ((unsigned short *) (ide_buf + ATA_IDENT_DEVICETYPE   ))[0];
			ide_devices[count].capabilities	= ((unsigned short *) (ide_buf + ATA_IDENT_CAPABILITIES ))[0];
			ide_devices[count].commandsets	= ((unsigned int   *) (ide_buf + ATA_IDENT_COMMANDSETS  ))[0];

			// (VII) Get Size:
			if (ide_devices[count].commandsets & (1<<26)){
				// Device uses 48-Bit Addressing:
				ide_devices[count].size   = ((unsigned int   *) (ide_buf + ATA_IDENT_MAX_LBA_EXT   ))[0];
				// Note that Quafios is 32-Bit Operating System, So last 2 Words are ignored.
			} else {
				// Device uses CHS or 28-bit Addressing:
				ide_devices[count].size   = ((unsigned int   *) (ide_buf + ATA_IDENT_MAX_LBA   ))[0];
			}

			// (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
			for(k = ATA_IDENT_MODEL; k < (ATA_IDENT_MODEL+40); k+=2) {
				ide_devices[count].model[k - ATA_IDENT_MODEL] = ide_buf[k+1];
				ide_devices[count].model[(k+1) - ATA_IDENT_MODEL] = ide_buf[k];
			}
			ide_devices[count].model[40] = 0; // Terminate String.

			count++;
		}
	}

	int ret = 0;
	
	// 4- Print Summary:
	for (i = 0; i < 4; i++) {
		if (ide_devices[i].reserved == 1) {
			printk(" Found %s Drive %dGB - %s\n",
			(const char *[]){"ATA", "ATAPI"}[ide_devices[i].type],         /* Type */
			ide_devices[i].size/1024/1024/2,               /* Size */
			ide_devices[i].model);
			if(ide_devices[i].type == 0) ret |= 1 << i;
		}
	}
	
	return ret;
}

/* ATA/ATAPI Read/Write Modes:
* ++++++++++++++++++++++++++++++++
*  Addressing Modes:
*  ================
*   - LBA28 Mode.         (+)
*   - LBA48 Mode.         (+)
*   - CHS.            (+)
*  Reading Modes:
*  ================
*   - PIO Modes (0 : 6)       (+) // Slower than DMA, but not a problem.
*   - Single Word DMA Modes (0, 1, 2).
*   - Double Word DMA Modes (0, 1, 2).
*   - Ultra DMA Modes (0 : 6).
*  Polling Modes:
*  ================
*   - IRQs
*   - Polling Status         (+) // Suitable for Singletasking   
*/

unsigned char ide_ata_access(unsigned char direction, unsigned char drive, unsigned int lba, unsigned char numsects, unsigned short selector, unsigned int edi)
{
	unsigned char lba_mode /* 0: CHS, 1:LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
	unsigned char lba_io[6];
	unsigned int  channel		= ide_devices[drive].channel; // Read the Channel.
	unsigned int  slavebit		 = ide_devices[drive].drive; // Read the Drive [Master/Slave]
	unsigned int  bus		  = channels[channel].base; // The Bus Base, like [0x1F0] which is also data port.
	unsigned int  words		 = 256; // Approximatly all ATA-Drives has sector-size of 512-byte.
	unsigned short cyl, i; unsigned char head, sect, err;
	
	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);
	
		// (I) Select one from LBA28, LBA48 or CHS;
	if (lba >= 0x10000000) { // Sure Drive should support LBA in this case, or you are giving a wrong LBA.
		// LBA48:
		lba_mode	 = 2;
		lba_io[0] = (lba & 0x000000FF)>> 0;
		lba_io[1] = (lba & 0x0000FF00)>> 8;
		lba_io[2] = (lba & 0x00FF0000)>>16;
		lba_io[3] = (lba & 0xFF000000)>>24;
		lba_io[4] = 0; // We said that we lba is integer, so 32-bit are enough to access 2TB.
		lba_io[5] = 0; // We said that we lba is integer, so 32-bit are enough to access 2TB.
		head		 = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} else if (ide_devices[drive].capabilities & 0x200)  { // Drive supports LBA?
		// LBA28:
		lba_mode	 = 1;
		lba_io[0] = (lba & 0x00000FF)>> 0;
		lba_io[1] = (lba & 0x000FF00)>> 8;
		lba_io[2] = (lba & 0x0FF0000)>>16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0; // These Registers are not used here.
		lba_io[5] = 0; // These Registers are not used here.
		head		 = (lba & 0xF000000)>>24;
	} else {
		// CHS:
		lba_mode	 = 0;
		sect		 = (lba % 63) + 1;
		cyl		 = (lba + 1	 - sect)/(16*63);
		lba_io[0] = sect;
		lba_io[1] = (cyl>>0) & 0xFF;
		lba_io[2] = (cyl>>8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head		 = (lba + 1	 - sect)%(16*63)/(63); // Head number is written to HDDEVSEL lower 4-bits.
	}
	
	// (II) See if Drive Supports DMA or not;
	dma = 0; // Supports or doesn't, we don't support !!!
	
	// (III) Wait if the drive is busy;
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait if Busy.
	
	// (IV) Select Drive from the controller;
	if (lba_mode == 0) ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit<<4) | head);	 // Select Drive CHS.
	else			 ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit<<4) | head);	 // Select Drive LBA.
	
	// (V) Write Parameters;
	if (lba_mode == 2) {
		ide_write(channel, ATA_REG_SECCOUNT1,	 0);
		ide_write(channel, ATA_REG_LBA3,	  lba_io[3]);
		ide_write(channel, ATA_REG_LBA4,	  lba_io[4]);
		ide_write(channel, ATA_REG_LBA5,	  lba_io[5]);
	}
	ide_write(channel, ATA_REG_SECCOUNT0,	 numsects);
	ide_write(channel, ATA_REG_LBA0,	  lba_io[0]);
	ide_write(channel, ATA_REG_LBA1,	  lba_io[1]);
	ide_write(channel, ATA_REG_LBA2,	  lba_io[2]);
	
	// (VI) Select the command and send it;
	// Routine that is followed:
	// If ( DMA & LBA48)	  DO_DMA_EXT;
	// If ( DMA & LBA28)	  DO_DMA_LBA;
	// If ( DMA & LBA28)	  DO_DMA_CHS;
	// If (!DMA & LBA48)	  DO_PIO_EXT;
	// If (!DMA & LBA28)	  DO_PIO_LBA;
	// If (!DMA & !LBA#)	  DO_PIO_CHS;
	if (lba_mode == 0 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;	
	if (lba_mode == 2 && dma == 0 && direction == 0) cmd = ATA_CMD_READ_PIO_EXT;	 
	if (lba_mode == 0 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 0) cmd = ATA_CMD_READ_DMA_EXT;
	if (lba_mode == 0 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 2 && dma == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
	if (lba_mode == 0 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
	ide_write(channel, ATA_REG_COMMAND, cmd);					  // Send the Command.
	
	if (dma)
		if (direction == 0);
		// DMA Read.
		else; // DMA Write.
	else
	if (direction == 0) {
			// PIO Read.
			for (i = 0; i < numsects; i++) {
				if (err = ide_polling(channel, 1)) return err; // Polling, then set error and exit if there is.
				insw(bus,edi,words);
				/*asm("pushw %es");
				asm("mov %%ax, %%es"::"a"(selector));
				asm("rep insw"::"c"(words), "d"(bus), "D"(edi)); // Receive Data.
				asm("popw %es");*/
				edi += (words*2);
			}
		} else {
			// PIO Write.
			for (i = 0; i < numsects; i++) {
				ide_polling(channel, 0); // Polling.
				outsw(bus,edi,words);
				/*asm("pushw %ds");
				asm("mov %%ax, %%ds"::"a"(selector));
				asm("rep outsw"::"c"(words), "d"(bus), "S"(edi)); // Send Data
				asm("popw %ds");*/
				edi += (words*2);
			}
			ide_write(channel, ATA_REG_COMMAND, (char []) {	  ATA_CMD_CACHE_FLUSH,
								ATA_CMD_CACHE_FLUSH,
								ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
			ide_polling(channel, 0); // Polling.
		}

	return 0; // Easy, ... Isn't it?
}

void ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, unsigned short es, unsigned int edi)
{

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0) package[0] = 0x1;		  // Drive Not Found!

	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
		package[0] = 0x2;							  // Seeking to invalid position.

	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else {
		unsigned char err;
		if (ide_devices[drive].type == IDE_ATA)
			err = ide_ata_access(ATA_READ, drive, lba, numsects, es, edi);
		package[0] = ide_print_error(drive, err);
	}
}

void ide_write_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, unsigned short es, unsigned int edi)
{

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0) package[0] = 0x1;		  // Drive Not Found!
	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
		package[0] = 0x2;							  // Seeking to invalid position.
	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else {
		unsigned char err;
		if (ide_devices[drive].type == IDE_ATA)
			err = ide_ata_access(ATA_WRITE, drive, lba, numsects, es, edi);
		else if (ide_devices[drive].type == IDE_ATAPI)
			err = 4; // Write-Protected.
		package[0] = ide_print_error(drive, err);
	}
}
