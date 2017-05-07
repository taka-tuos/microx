#include "pci.h"

// PCI CONFIG_ADDRESS
#define PCI_CONFIG_ADDRESS 0x0cf8

// Config Data register range is 0x0cfc - 0x0cff.
#define CONFIG_DATA_1 0x0cfc
#define CONFIG_DATA_2 0x0cfd
#define CONFIG_DATA_3 0x0cfe
#define CONFIG_DATA_4 0x0cff

#define PCI_BUS_MAX 255
#define PCI_DEVICE_MAX 31
#define PCI_FUNCTION_MAX 7

// This structure represents PCI's CONFIG_ADDRESS register.
struct pci_configuration_register {
	uint32_t enable_bit;      // 31: enable bit.
	uint32_t reserved;        // 24-30: reserved.
	uint32_t bus_num;         // 16-23: bus number.
	uint32_t dev_num;         // 11-15: device number.
	uint32_t func_num;        // 8-10: function number.
	uint32_t reg_num;         // 2-7: regster number.
	uint32_t bit0;            // 0-1: always 0.
};

union pci_bios32 {
	struct {
		uint8_t sig[4];         // _32_.
		uint32_t entry;         // entry point.
		uint8_t rev;            // revision.
		uint8_t len;            // length.
		uint8_t checksum;       // checksum.
		uint8_t reserved[5];    // reserved.
	} fields;
	char data[16];
};

struct pci_device_list pci_device_head = {
	.next = &pci_device_head,
};

/////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////
static inline void finish_access_to_config_data(struct pci_configuration_register *reg);
static inline void write_pci_config_address(const struct pci_configuration_register *reg);

static void write_pci_data(struct pci_configuration_register *reg, uint32_t data);
static uint32_t read_pci_data(struct pci_configuration_register *reg);

static inline uint32_t read_pci_sub_system(struct pci_configuration_register *reg);
static inline uint32_t read_pci_reg00(struct pci_configuration_register *reg);
static inline uint32_t read_pci_class(struct pci_configuration_register *reg);
static inline uint32_t read_pci_header_type(struct pci_configuration_register *reg);
static uint32_t find_pci_data(uint8_t bus, uint8_t dev);

static bool store_pci_device_to_list(uint8_t bus, uint8_t devfn, 
				     uint32_t data, uint8_t func, 
				     uint32_t class, uint32_t header,
				     uint32_t subsystem);

static inline int is_multi_function(uint32_t data)
{
	return (data & 0x800000) ? 1 : 0;
}

static bool 
store_pci_device_to_list(uint8_t bus, uint8_t devfn, 
			 uint32_t data, uint8_t func, 
			 uint32_t class, uint32_t header,
			 uint32_t subsystem)
{
	struct pci_device_list *p;

	p = kmalloc(sizeof(*p));
	if (!p)
		return false;

	memset(p, 0, sizeof(*p));

	p->data.bus = bus;
	p->data.devfn = devfn;
	p->data.vender = data & 0xffff;
	p->data.devid = (data >> 16) & 0xffff;
	p->data.pg_if = (class >> 8) & 0xff;
	p->data.sub_class = (class >> 16) & 0xff;
	p->data.base_class = (class >> 24) & 0xff;
	p->data.func = func;
	p->data.header_type = ((header >> 16) & 0xff) & 0x7f;
	p->data.multi = (header & 0x800000) ? 1 : 0;
	p->data.sub_vender = subsystem & 0xffff;
	p->data.sub_devid = (subsystem >> 16) & 0xffff;


	p->next = pci_device_head.next;
	pci_device_head.next = p;

	return true;
}


/**
 * Set ENABLE bit to 0 and write data to CONFIG_ADDRESS.
 */
static inline void finish_access_to_config_data(struct pci_configuration_register *reg)
{
	reg->enable_bit = 0;
	write_pci_config_address(reg);
}

/**
 * Read CONFIG_DATA.
 * @param reg it should be set bus, device, function and so forth.
 * @return data from CONFIG_DATA.
 */
static uint32_t read_pci_data(struct pci_configuration_register *reg)
{
	uint32_t data;

	// Enable bit should be 1 before read PCI_DATA.
	reg->enable_bit = 1;

	// write data to CONFIG_ADDRESS.
	write_pci_config_address(reg);
	
	data = inl(CONFIG_DATA_1);

	finish_access_to_config_data(reg);

	return data;
}

/**
 * Write to CONFIG_DATA.
 * @param reg it should be set bus, device, function and so forth.
 * @param data should be write to CONFIG_DATA
 * @return data from CONFIG_DATA.
 */
static void write_pci_data(struct pci_configuration_register *reg, uint32_t data)
{
	// Enable bit should be 1 before read PCI_DATA.
	reg->enable_bit = 1;

	// write data to CONFIG_ADDRESS.
	write_pci_config_address(reg);
	
	outl(CONFIG_DATA_1, data);
	finish_access_to_config_data(reg);
}


/**
 * Write data to CONFIG_ADDRESS.
 * @param reg it should be set bus, device, function and so forth.
 */
static inline void write_pci_config_address(const struct pci_configuration_register *reg)
{
	uint32_t data = 0;

	data = (reg->enable_bit << 31) |
		(reg->reserved << 24) | 
		(reg->bus_num << 16) | 
		(reg->dev_num << 11) | 
		(reg->func_num << 8) |
		reg->reg_num;

	outl(PCI_CONFIG_ADDRESS, data);	
}

/**
 * Read pci class.
 * @param reg it should be set bus, device, function and so forth.
 * @return PCI class.
 */
static inline uint32_t read_pci_class(struct pci_configuration_register *reg)
{
	reg->reg_num = 0x8;

	return read_pci_data(reg);
}

/**
 * Read CONFIG_DATA by register 0x00.
 * @param reg it should be set bus, device, function and so forth.
 * @return vendor id and device id.
 */
static inline uint32_t read_pci_reg00(struct pci_configuration_register *reg)
{
	reg->reg_num = 0;

	return read_pci_data(reg);
}

/**
 * Read CONFIG_DATA by register 0x04.
 * @param reg it should be set bus, device, function and so forth.
 * @return status.
 */
static inline uint32_t read_pci_command_register(struct pci_configuration_register *reg)
{
	reg->reg_num = 0x4;

	return read_pci_data(reg);
}

/**
 * Read CONFIG_DATA by register 0x0c to check if it's PCI brigdge or not.
 * @param reg it should be set bus, device, function and so forth.
 * @return vendor id and device id.
 */
static inline uint32_t read_pci_header_type(struct pci_configuration_register *reg)
{
	reg->reg_num = 0xc;

	return read_pci_data(reg);
}

/**
 * Read CONFIG_DATA by register 0x2c to get its sub system data.
 * @param reg it should be set bus, device, function and so forth.
 * @return sub system.
 */
static inline uint32_t read_pci_sub_system(struct pci_configuration_register *reg)
{
	reg->reg_num = 0x2c;

	return read_pci_data(reg);
}

/**
 * Find PCI device by bus number and device number.
 * @param bus bus numer.
 * @param dev device number.
 * @return always 0.
 */
static uint32_t find_pci_data(uint8_t bus, uint8_t dev)
{
	uint32_t data;
	uint32_t status;
	uint32_t class;
	uint32_t header;
	uint32_t subsystem;

	int i;
	struct pci_configuration_register reg;
	bool b;

	// At first, check function number zero.
	memset(&reg, 0, sizeof(reg));
	reg.bus_num = bus;
	reg.dev_num = dev;

	// Check all function numbers.
	for (i = 0; i < PCI_FUNCTION_MAX; i++) {
		reg.func_num = i;		
		data = read_pci_reg00(&reg);
		if (data != 0xffffffff) {

			class = read_pci_class(&reg);
			header = read_pci_header_type(&reg);
			subsystem = read_pci_sub_system(&reg);
			status = read_pci_command_register(&reg);

			b = store_pci_device_to_list(bus, dev, data, i, class, header, subsystem);

			// if it's not a multi function, we need to search other function.
			if (i == 0 && !is_multi_function(header))
				return 0;
		}
	}
	
	return 0;
}

#ifdef USE_PCI_BIOS32
static bool find_pci_bios32(void);
/**
 * Find PCI BIOS.
 * @ret bool PCI BIOS is found or not.
 */
static bool find_pci_bios32(void)
{
	unsigned long addr = 0;
	union pci_bios32 *bios32;
	char sum;
	int len, i;

	for (addr = 0xe0000; addr < 0xffff0; addr += 16) {
		bios32 = (union pci_bios32 *) addr;

		if (bios32 != NULL && 
		    bios32->fields.sig[0] == '_' &&
		    bios32->fields.sig[1] == '3' &&
		    bios32->fields.sig[2] == '2' &&
		    bios32->fields.sig[3] == '_') {

			len = bios32->fields.len * 16;
			if (len) {
				for (i = 0, sum = 0; i < len; i++)
					sum += bios32->data[i];

				if (!sum) {
					printk("found pci bios32 at 0x%lx\n", addr);
					printk("PCI BIOS32 entry point is 0x%lx\n", bios32->fields.entry);
					printk("PCI BIOS32 revision is 0x%x\n", bios32->fields.rev);
					return true;
				}
			}
		}
				
	}
	printk("pci bios32 not found\n");
	return false;
 
}
#endif // USE_PCI_BIOS32

/////////////////////////////////////////////////
// public functions
/////////////////////////////////////////////////
/**
 * Find all PCI devices.
 */
void find_pci_device(void)
{
	int bus, dev;

	for (bus = 0; bus < PCI_BUS_MAX; bus++) {
		for (dev = 0; dev < PCI_DEVICE_MAX; dev++)
			find_pci_data(bus, dev);
	}

//	find_pci_bios32();
}

/**
 * Printing out all PCI devices which kernel found.
 */
void show_all_pci_device(void)
{
	struct pci_device_list *p;

	for (p = pci_device_head.next; p != &pci_device_head; p = p->next)
		printk("Found Device: Bus %d:Devfn %d:Vender 0x%x:Device 0x%x:func %d:header 0x%x:Class 0x%lx-0x%lx:Multi %d\n", 
		       p->data.bus, p->data.devfn, 
		       p->data.vender, p->data.devid, 
		       p->data.func, p->data.header_type,
		       p->data.base_class, p->data.sub_class,
		       p->data.multi);
}

/**
 * Check PCI device.
 * @param vender id.
 * @param device number.
 * @param function number.
 * @return if found it returns pci device information structure.
 */
struct pci_device *get_pci_device(uint16_t vender, uint16_t device, uint8_t function)
{
	struct pci_device_list *p;

	for (p = pci_device_head.next; p != &pci_device_head; p = p->next) {
		if (p->data.vender == vender &&
		    p->data.devid == device &&
		    p->data.func == function)
			return &p->data;
	}

	return NULL;
}

/**
 * Read data from PCI_DATA.
 * @param pci
 * @param reg_num which register you want to read.
 * @return data from PCI_DATA.
 */
uint32_t pci_data_read(struct pci_device *pci, uint8_t reg_num)
{
	struct pci_configuration_register reg;

	memset(&reg, 0, sizeof(reg));

	reg.reg_num = reg_num;
	reg.func_num = pci->func;
	reg.dev_num = pci->devfn;
	reg.bus_num = pci->bus;

	return read_pci_data(&reg);
}

/**
 * Write data to PCI_DATA.
 * @param pci
 * @param reg_num which register you want to write.
 * @param data.
 */
void pci_data_write(struct pci_device *pci, uint8_t reg_num, uint32_t data)
{
	struct pci_configuration_register reg;

	memset(&reg, 0, sizeof(reg));

	reg.reg_num = reg_num;
	reg.func_num = pci->func;
	reg.dev_num = pci->devfn;
	reg.bus_num = pci->bus;
	
	write_pci_data(&reg, data);
}
