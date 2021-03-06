#include "ata.h"
#include "pci.h"

#define DATA_REGISTER             0x01f0
#define ERROR_REGISTER            0x01f1
#define FEATURES_REGISTER         0x01f1
#define SECTOR_COUNT_REGISTER     0x01f2
#define SECTOR_NUMBER_REGISTER    0x01f3
#define CYLINDER_LOW_REGISTER     0x01f4
#define CYLINDER_HIGH_REGISTER    0x01f5
#define DEVICE_HEAD_REGISTER      0x01f6
#define STATUS_REGISTER           0x01f7
#define COMMAND_REGISTER          0x01f7
#define ALTERNATE_STATUS_REGISTER 0x03f6
#define DEVICE_CONTROL_REGISTER   0x03f6

#define PIO_SECTOR_WRITE_CMD 0x34
#define PIO_SECTOR_READ_CMD  0x24

// Max logical sector number.
static uint32_t max_logical_sector_num[] = { 0, 0 };

static int ata_disk_inited[2] = { 0, 0 };

// driver operations.
static int open_ATA_disk(void);
static int close_ATA_disk(void);
static int read_sector(int device, uint32_t sector, 
		       sector_t *buf,	size_t buf_size);
static int write_sector(int device, uint32_t sector, 
			sector_t *buf,	size_t buf_size);
			
extern void io_cli(void);
extern void io_sti(void);

// For find IDE interface.
static struct pci_device_info ata_info[] = {
	{ 0x8086, 0x7010, 1 }, // PIIX3 IDE Interface (Triton II)
};

struct block_driver_operations ata_dev = {
	.name = "ATA disk",
	.open = &open_ATA_disk,
	.close = &close_ATA_disk,
	.read = &read_sector,
	.write = &write_sector,
};

static struct pci_device *this_device;

/////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////
// for PCI.
static bool find_ata_device(void);
static void set_bus_master_bit(void);
static void get_base_address(void);

// for ATA device.

static bool wait_until_BSY_and_DRQ_are_zero(uint16_t port);
static bool wait_until_BSY_is_zero(uint16_t port);
static void set_device_number(int device);
static bool do_device_selection_protocol(int device);
static void set_cylinder_register(uint8_t high, uint8_t low);
static uint8_t get_cylinder_regster(uint16_t port);
static void set_sector_count_register(uint8_t data);
static uint8_t get_DRDY(void);
static void write_command(uint8_t com);
static bool is_error(uint8_t data);
static bool is_drq_active(uint8_t data);
static void print_error_register(int device);
static bool is_device_fault(void);
static bool do_identify_device(int device, sector_t *buf);
static void do_soft_reset(int device);
static bool initialize_common(int device);
static bool initialize_ata(void);
static bool sector_rw_common(uint8_t cmd, int device, uint32_t sector, uint32_t count);
static void finish_sector_rw(void);

typedef union block_data_ {
	sector_t sector[256];
	char data[512];
} block_data_t;

//#define USE_SECTOR_RW_TEST
#ifdef USE_SECTOR_RW_TEST
static void sector_rw_test(void)
{
	block_data_t block;
	bool ret;

	printk("%s\n", __FUNCTION__);

	ret = read_sector(0, 0x1b8, block.sector, 256);
	if (ret)
		printk("read error\n");

	printk("Data at 0x1b8 is %s", block.data);

//	buf[0] = 0x6261;
//	buf[1] = 0x6463;
//	buf[2] = 0xa065;
	
//	write_sector(0, 222, buf, sizeof(buf) / sizeof(buf[0]));

	memset(block.sector, 0x0, sizeof(block.sector));

	read_sector(0, 0x1ba, block.sector, 256);
	printk("Data at 0x1ba is %s", block.data);

}
#else
static void sector_rw_test(void) {}
#endif // USE_SECTOR_RW_TEST

/**
 * Open ATA disk.
 * not supported yet.
 */
static int open_ATA_disk(void)
{
	bool ret;

	ret = initialize_ata();
	if (!ret)
		return -1;

	sector_rw_test();

	return (ata_disk_inited[1] << 1) | ata_disk_inited[0];
}

/**
 * Close ATA disk.
 * not supported yet.
 */
static int close_ATA_disk(void)
{
	printk("%s\n", __FUNCTION__);

	return 0;
}

/**
 * Writing one sector.
 * @param device number.
 * @param sector number.
 * @param data to write.
 * @param data size. it should be 256.
 * @return negative if fail to write data.
 */
int write_sector(int device, uint32_t sector, 
		  sector_t *buf, size_t buf_size)
{
	bool ret;
	size_t i;
	
	io_cli();

	ret = sector_rw_common(PIO_SECTOR_WRITE_CMD, device, sector, buf_size >> 8);
	if (!ret)
		return -1;

	finish_sector_rw();

	for (i = 0; i < buf_size; i++) 
		outw(DATA_REGISTER, buf[i]);
		
	io_sti();

	return 0;
}

/**
 * Reading one sector.
 * @param device number.
 * @param sector number.
 * @param data to store..
 * @param data size. it should be 256.
 * @return  0 if process is success. if something wrong it returns negative value
 */
int read_sector(int device, uint32_t sector, 
		 sector_t *buf, size_t buf_size)
{
	bool ret;
	size_t i;
	
	io_cli();

	//printk("DEVICE %d LBA 0x%08x READ\n", device, sector);
	
	/*if (buf_size != 256) {
		printk("buf_size isn't 256\n");
		while (1);
	}*/
		
	ret = sector_rw_common(PIO_SECTOR_READ_CMD, device, sector, buf_size >> 8);
	if (!ret)
		return -1;

	for (i = 0; i < buf_size; i++)
		buf[i] = inw(DATA_REGISTER);
		
	/*unsigned char *ubuf = (unsigned char *)buf;
	for (i = 0; i < 128; i+=16) {
		for (int j = 0; j < 16; j++) printk("%02X ",ubuf[i+j]);
		for (int j = 0; j < 16; j++) printk("%c",ubuf[i+j] < 0x20 ? '.' : ubuf[i+j]);
		printk("\n");
	}*/

	finish_sector_rw();
	
	io_sti();

	return 0;
}


/**
 * Find ATA device from pci device list.
 * @return bool
 */
static bool find_ata_device(void)
{
	struct pci_device_list *p;

	for (p = pci_device_head.next; p != &pci_device_head; p = p->next)
	{
		if(p->data.base_class == 0x01 && (p->data.sub_class == 0x05 || p->data.sub_class == 0x01)) {
			this_device = &p->data;
			printk("Found ATA device %x:%x:%d\n", p->data.vender, p->data.devid, p->data.func);
			return true;
		}
	}
	
	/*int i;
	int size = sizeof(ata_info) / sizeof(ata_info[0]);

	for (i = 0; i < size; i++) {
		this_device = get_pci_device(ata_info[i].vender, ata_info[i].devid, ata_info[i].func);
		if (this_device) {
			printk("Found ATA device %x:%x:%d\n", ata_info[i].vender, ata_info[i].devid, ata_info[i].func);
			return true;
		}
	}
	return false;*/
	return false;
}

/**
 * Set bus master bit in command register in CONFIG_DATA.
 */
static void set_bus_master_bit(void)
{
	uint32_t data;

	data = pci_data_read(this_device, 0x4) | 0x04;
	pci_data_write(this_device, 0x04, data);
}

/**
 *Get base address.
 */
static void get_base_address(void)
{
	int i;

	for (i = 0x10; i <= 0x24; i += 0x04) {
		this_device->base_address = pci_data_read(this_device, i);
		if (this_device->base_address) {
			printk("ATA Base Address is 0x%lx(0x%x)\n", this_device->base_address, i);
			break;
		}
	}

}

/**
 * Wait sometime until BSY and DRQ bits are zero.
 * @param port which can be STATUS REGISTER or ALTERNATE STATUS REGISTER.
 * @return true if both bits are zero in few usec.
 */
static bool wait_until_BSY_and_DRQ_are_zero(uint16_t port)
{
	uint8_t data = 0xff;
	uint8_t bsy, drq;
	int i = 0;

	do {
		data = inb(port);
		bsy = (data >> 7) & 0x01;
		drq = (data >> 3) & 0x01;
		i++;
		if(!(!bsy && !drq)) wait_loop_usec(5);
	} while (!(!bsy && !drq) && i < 1000);

	return (bsy == 0 && drq == 0) ? true : false;
}

/**
 * Wait sometime until BSY  bit is zero.
 * @param port which can be STATUS REGISTER or ALTERNATE STATUS REGISTER.
 * @return true if BSY bit is zero in few usec.
 */
static bool wait_until_BSY_is_zero(uint16_t port)
{
	uint8_t data = 0xff;
	uint8_t bsy;
	int i = 0;
	
	i = 0;

	do {
		data = inb(port);
		bsy = (data >> 7) & 0x01;
		i++;
		if(bsy) wait_loop_usec(5);
	} while (bsy && i < 1000);

	return (bsy == 0) ? true : false;

}

/**
 * Set device number to Device/Head register.
 * @param device must be 0 or 1.
 */
static void set_device_number(int device)
{
	uint8_t data = 0;

	// Device number is zero.
	data = inb((uint16_t) DEVICE_HEAD_REGISTER);

	data = 0xa0 | (device << 4);

	outb((uint16_t) DEVICE_HEAD_REGISTER, data);

	// five usec should be enough time to wait.
	wait_loop_usec(5);
}

/**
 * Device selection protocol.
 * @param device number
 * @return true if success.
 */
static bool do_device_selection_protocol(int device)
{
	bool ret = false;

	ret = wait_until_BSY_and_DRQ_are_zero(ALTERNATE_STATUS_REGISTER);
	if (ret) {
		set_device_number(device);
		wait_loop_usec(2);
		ret = wait_until_BSY_and_DRQ_are_zero(ALTERNATE_STATUS_REGISTER);
	}

	return ret;
}

/**
 * Set data to Cylinder High and Low  registers.
 * @param high data to Cylinder High register.
 * @param log data to Cylinder Low register.
 */
static void set_cylinder_register(uint8_t high, uint8_t low)
{
	outb(CYLINDER_LOW_REGISTER, low);
	outb(CYLINDER_HIGH_REGISTER, high);
}

/**
 * Read cylinder register.
 * @param port is Cylinder high or low.
 * @return data
 */
static uint8_t get_cylinder_regster(uint16_t port)
{
	return inb(port);
}

/**
 * Set data to Sector Count register.
 * @param data
 */
static void set_sector_count_register(uint8_t data)
{
	outb(SECTOR_COUNT_REGISTER, data);
}

/**
 * Returns DRDY bit which in Status register.
 * @return 1 or 0.
 */
static uint8_t get_DRDY(void)
{
	uint8_t data = inb(STATUS_REGISTER);

	return (data >> 6) & 0x01;
}

/**
 * Execute ATA command.
 * @param com is command number.
 */
static void write_command(uint8_t com)
{
	outb(COMMAND_REGISTER, com);
}

/**
 * Check if error bit is active .
 * @return false means "no error".
 */
static bool is_error(uint8_t data)
{
	return (data & 0x01) == 0 ? false : true;
}

/**
 * Check if DRQ bit is active.
 * @return true is this bit is active.
 */
static bool is_drq_active(uint8_t data)
{
	return (((data >> 3) & 0x01) == 1) ? true : false;
}

/**
 * Printing error register data.
 * @param device number.
 */
static void print_error_register(int device)
{
	if (do_device_selection_protocol(device)) {
		uint8_t err = inb(ERROR_REGISTER);
		printk("error is 0x%x\n", err);
	}
}

/**
 * Check if Device Fault bit is active.
 * @return true means some error occured.
 */
static bool is_device_fault(void)
{
	uint8_t data;

	data = inb(STATUS_REGISTER);
	
	return (data >> 5 & 0x01) == 1 ? true : false;
}

static bool wait_until_device_is_ready(int device)
{
	int i;
	bool b = false;

	for (i = 0; i < 1000; i++) {
		b = do_device_selection_protocol(device);
		if (b)
			break;
		wait_loop_usec(5);
	}

	return b;
}

/**
 * Sector Read/Write common function for PIO data R/W.
 * @param cmd is read or write command.
 * @param device number.
 * @param sector number.
 * @return true if success.
 */
static bool sector_rw_common(uint8_t cmd, int device, uint32_t sector, uint32_t count)
{
	bool b = false;
	uint8_t status;
	int loop = 0;

	// sector number need to be checked.
	if (sector > max_logical_sector_num[device]) {
		printk("Invalid Sector number 0x%lx\n", sector);
		return false;
	}

	b = wait_until_device_is_ready(device);
	if (!b) {
		printk("Device wasn't ready.\n");
		return false;
	}

	// nIEN bit should be enable and other bits are disable.
	outb(DEVICE_CONTROL_REGISTER, 0x02);

	// Features register should be 0.
	outb(FEATURES_REGISTER, 0x00);

	// Set Logical Sector.
	/*outb(DEVICE_HEAD_REGISTER, ((sector >> 24) & 0x0f) | 0xe0 | (device << 4));
	outb(SECTOR_NUMBER_REGISTER, sector & 0xff);
	outb(CYLINDER_LOW_REGISTER, (sector >> 8) & 0xff);
	outb(CYLINDER_HIGH_REGISTER, (sector >> 16) & 0xff);
	outb(SECTOR_COUNT_REGISTER, count);*/
	
	outb(DEVICE_HEAD_REGISTER, 0x40 | (device << 4));
	outb(SECTOR_COUNT_REGISTER, count >> 8);
	outb(SECTOR_NUMBER_REGISTER, (sector >> 24) & 0xff);
	outb(CYLINDER_LOW_REGISTER, 0);
	outb(CYLINDER_HIGH_REGISTER, 0);
	outb(SECTOR_COUNT_REGISTER, count & 0xff);
	outb(SECTOR_NUMBER_REGISTER, sector & 0xff);
	outb(CYLINDER_LOW_REGISTER, (sector >> 8) & 0xff);
	outb(CYLINDER_HIGH_REGISTER, (sector >> 16) & 0xff);

#if 0
	printk("device:0x%x secnum:0x%x low:0x%x high:0x%x head:0x%x\n",
	       device,
	       sector & 0xff,
	       (sector >> 8) & 0xff,
	       (sector >> 16) & 0xff,
	       (((sector >> 24) & 0x1f) | 0x40));
#endif

	// Execute command.
	outb(COMMAND_REGISTER, cmd);

	wait_loop_usec(4);

	if (!wait_until_BSY_is_zero(STATUS_REGISTER)) {
		printk("wait failed\n");
		return false;
	}

	inb(ALTERNATE_STATUS_REGISTER);

read_status_register_again:
	wait_loop_usec(5);
	status = inb(STATUS_REGISTER);

	if (is_error(status)) {
		printk("error occured:0x%x\n", status);
		print_error_register(device);
		return false;
	}
	
	if (!is_drq_active(status)) {
		if (loop > 32) {
			printk("DRQ didn't be active\n");
			return false;
		}
		loop++;
		goto read_status_register_again;
	}

	return true;
}

/**
 * Read ALTERNATE_STATUS_REGISTER and STATUS_REGISTER when finish read/write sector.
 */
static void finish_sector_rw(void)
{
	inb(ALTERNATE_STATUS_REGISTER);
	inb(STATUS_REGISTER);
}

/**
 * Execute Identify Device command.
 * @param device number.
 * @param buf is to store data.
 * @param false is failed Identify Device command.
 */
static bool do_identify_device(int device, sector_t *buf)
{
	bool ret = false;
	uint8_t data;
	int i;

	do_device_selection_protocol(device);
	ret = get_DRDY();
	if (ret) {

		write_command(0xec);

//	read_bsy: // unused.
		wait_loop_usec(5);

		if (!wait_until_BSY_is_zero(STATUS_REGISTER)) {
			printk("wait failed\n");
			return false;
		}

		data = inb(ALTERNATE_STATUS_REGISTER);

	read_status_register:
		data = inb(STATUS_REGISTER);

		if (is_error(data)) {
			printk("error occured:0x%x\n", data);
			print_error_register(device);
			return false;
		}

		if (!is_drq_active(data))
			goto read_status_register;

		if (is_device_fault()) {
			printk("some error occured\n");
			return false;
		}

		for (i = 0; i < 256; i++)
			buf[i] = inw(DATA_REGISTER);
	} 

	return true;

}

/**
 * Do sotf reset.
 * @param device should be 0(master) or 1(slave).
 */
static void do_soft_reset(int device)
{
	// Initialize master/slave.
	set_device_number(device);

	// Do DEVICE RESET command. 
	write_command(0x08);

	// Wait sometime after DEVICE RESET.
	wait_loop_usec(5);	
}

// It represents device type.
enum {
	DEV_TYPE_UNKNOWN = -1,
	DEV_TYPE_ATA = 0,
	DEV_TYPE_ATAPI,
};

/**
 * Check this device is ATA or ATAPI.
 * @param high data from Cylinder high register.
 * @param low data from Cylinder low register.
 * @return device type.
 */
static int get_device_type(uint8_t high, uint8_t low)
{
	if (high == 0x0 && low == 0x0)
		return DEV_TYPE_ATA;
	else if (high == 0xeb && low == 0x14)
		return DEV_TYPE_ATAPI;
	else
		return DEV_TYPE_UNKNOWN;
}

/**
 * Main initialize routine.
 * @param device is master or slave.
 * @return true or false.
 */
static bool initialize_common(int device)
{
	uint8_t high, low;
	sector_t buf[256];
	int dev = 0;
	bool ret = false;

	memset(buf, 0x0, sizeof(buf));

	high = low = 0;

	do_soft_reset(device);

	// Read Cylinder register high and low before use.
	low = get_cylinder_regster(CYLINDER_LOW_REGISTER);
	high = get_cylinder_regster(CYLINDER_HIGH_REGISTER);

	// Is this device ATA?
	dev = get_device_type(high, low);
	switch (dev) {
	case DEV_TYPE_ATA: // Only supports ATA device yet.
		break;
	case DEV_TYPE_ATAPI:
		printk("ATAPI device is not supported yet.\n");
		return false;
	default:
		printk("Unknown device\n");
		return false;
	}

	ret = do_identify_device(device, buf);
	if (!ret) {
		printk("identify device failed\n");
		return false;
	}

	max_logical_sector_num[device] = ((uint32_t) buf[61] << 16) | buf[60];
	
	printk("ATA device sector num : %d\n", max_logical_sector_num[device]);
	
	return true;
}

/**
 * Initialize ATA device when use it.
 */
static bool initialize_ata(void)
{
	bool ret1 = false, ret2 = false;

	ret1 = initialize_common(0);
	if (!ret1) {
		printk("Don't setup master\n");
	}
	ret2 = initialize_common(1);
	if (!ret2) {
		printk("Don't setup slave\n");
	}
	
	ata_disk_inited[0] = ret1 ? 1 : 0;
	ata_disk_inited[1] = ret2 ? 1 : 0;

	return ret1 | (ret2 << 1);
}

/////////////////////////////////////////////////
// public functions
/////////////////////////////////////////////////
bool init_ata_disk_driver(void)
{
	if (!find_ata_device())
		return false;
	
	set_bus_master_bit();

	get_base_address();

	return true;
}
