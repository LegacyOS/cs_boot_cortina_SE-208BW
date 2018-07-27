/****************************************************************************
* Copyright  JessLink Corp 2005.  All rights reserved.
*--------------------------------------------------------------------------
* Name			: pci.h
* Description	: PCI define for sl2312 host PCI bridge
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	10/28/2005	Vic Tseng	Create
*
****************************************************************************/
#define PCI_VENDOR_ID		0x00	/* 16 bits */
#define PCI_DEVICE_ID		0x02	/* 16 bits */
#define PCI_COMMAND		0x04	/* 16 bits */
#define  PCI_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define PCI_HEADER_TYPE		0x0e	/* 8 bits */
#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */

#define SL2312_PCI_PMC				0x40
#define SL2312_PCI_PMCSR			0x44
#define SL2312_PCI_CTRL1			0x48
#define SL2312_PCI_CTRL2			0x4c
#define SL2312_PCI_MEM1_BASE_SIZE	0x50
#define SL2312_PCI_MEM2_BASE_SIZE	0x54
#define SL2312_PCI_MEM3_BASE_SIZE	0x58

#define SL2312_PCI_DMA_MEM1_BASE		0x00000000
#define SL2312_PCI_DMA_MEM2_BASE		0x00000000
#define SL2312_PCI_DMA_MEM3_BASE		0x00000000
#define SL2312_PCI_DMA_MEM1_SIZE		7
#define SL2312_PCI_DMA_MEM2_SIZE		6
#define SL2312_PCI_DMA_MEM3_SIZE		6

#define PCI_DEVFN(slot,func)	((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)			(((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)			((devfn) & 0x07)

/*
 * Error values that may be returned by PCI functions.
 */
#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED	0x81
#define PCIBIOS_BAD_VENDOR_ID		0x83
#define PCIBIOS_DEVICE_NOT_FOUND	0x86
#define PCIBIOS_BAD_REGISTER_NUMBER	0x87
#define PCIBIOS_SET_FAILED		0x88
#define PCIBIOS_BUFFER_TOO_SMALL	0x89

void scan_pci_bus(void);
#ifdef NEC_USB
extern u32 is_uPD72010_existed(int);
#endif
#ifdef VIA_USB
extern u32 is_VT61212_existed(int);
#endif

struct pci_dev {
	unsigned int	devfn;		/* encoded device & function index */
	unsigned short	vendor;
	unsigned short	device;
	unsigned short	subsystem_vendor;
	unsigned short	subsystem_device;
};

#define PCI_VENDOR_ID_NEC			0x1033
#define PCI_DEVICE_ID_NEC_OHCI			0x0035

#define	GLOBAL_IO_PIN_DRIVING			GLOBAL_DRIVE_CTRL
#define	GLOBAL_IO_PIN_SLEW_RATE			GLOBAL_SLEW_RATE_CTRL
