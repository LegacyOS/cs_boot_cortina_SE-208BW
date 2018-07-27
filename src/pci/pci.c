/****************************************************************************
* Copyright  JessLink Corp 2005.  All rights reserved.
*--------------------------------------------------------------------------
* Name			: pci.c
* Description	: PCI functions for sl2312 host PCI bridge
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	10/28/2005	Vic Tseng	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include "pci.h"

//debu
//#define PCI_DEBUG	1

#ifdef	PCI_DEBUG
#define	PCI_PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PCI_PRINTF(fmt,args...)
#endif

//#define IO_ADDRESS(x)      (((x&0xff000000)>>4)|(x & 0x000fffff)|0xF0000000)
//==> IO_ADDRESS(SL2312_PCI_IO_BASE) = 0xF3000000

// sl2312 PCI bridge access routines

#define PCI_IOSIZE_REG	(*(volatile unsigned long *) (SL2312_PCI_IO_BASE))
#define PCI_PROT_REG	(*(volatile unsigned long *) (SL2312_PCI_IO_BASE + 0x04))
#define PCI_CTRL_REG	(*(volatile unsigned long *) (SL2312_PCI_IO_BASE + 0x08))
#define PCI_SOFTRST_REG	(*(volatile unsigned long *) (SL2312_PCI_IO_BASE + 0x10))
#define PCI_CONFIG_REG	(*(volatile unsigned long *) (SL2312_PCI_IO_BASE + 0x28))
#define PCI_DATA_REG	(*(volatile unsigned long *) (SL2312_PCI_IO_BASE + 0x2C))


extern void hal_delay_us(UINT32 us);

static void sl2312_PCI_soft_reset(void)
{
/*
  if( REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) |=  0x0000015A )
     printf("\n      Reset O.K. !! ");
  else
     printf("\n      Reset Fail !! ");
*/
     REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) |=  0x000001DA;		// 111006-astel: 0x0000015A
     hal_delay_us(100000);
}

static void sl2312_read_miscellaneous_reg(void)
{
	unsigned long mis_reg;

    mis_reg = REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL);
    PCI_PRINTF(">>>>>> Miscellaneous Register = 0x%08x \n", mis_reg);
	hal_delay_us(100);
}

static void sl2312_PCI_clk_33MHz(void)
{
/* mark 
   if( REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~0x00000040 )
    printf("\n      Set 33MHz O.K. !!");
   else
    printf("\n      Set 33Mhz Fail !!");
*/
    REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~0x00040000;	// 111006-astel: 0x00000040
    hal_delay_us(10000);
}

static void sl2312_enable_drive_PCI_clk(void)
{
    //REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |=  0x00000020;
/* mark  
    if ( REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |=  0x0000020 )
     printf("\n      Enable Clock O.K. !! ");
    else
     printf("\n      Enable Clock Fail !! ");
*/
    REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |=  0x00020000;	// // 111006-astel: 0x00000020
    hal_delay_us(10000);
}

int sl2312_read_config_byte(struct pci_dev *dev, int where, u8 *val)
{
	unsigned long addr, data;
	unsigned long flags;

	addr = 0x80000000 | (PCI_SLOT(dev->devfn) << 11) | (PCI_FUNC(dev->devfn) << 8) | (where & ~0x03);
	PCI_CONFIG_REG = addr;
	data = PCI_DATA_REG;
	*val = (u8) (data >> ((where & 0x03) * 8));

	return PCIBIOS_SUCCESSFUL;
}

int sl2312_read_config_word(struct pci_dev *dev, int where, u16 *val)
{
	unsigned long addr, data;
	unsigned long flags;

	addr = 0x80000000 | (PCI_SLOT(dev->devfn) << 11) | (PCI_FUNC(dev->devfn) << 8) | (where & ~0x03);
	PCI_CONFIG_REG = addr;
	data = PCI_DATA_REG;
	*val = (u16) (data >> ((where & 0x02) * 8));

	return PCIBIOS_SUCCESSFUL;
}

int sl2312_read_config_dword(struct pci_dev *dev, int where, u32 *val)
{
	unsigned long addr;
	unsigned long flags;

	addr = 0x80000000 | (PCI_SLOT(dev->devfn) << 11) | (PCI_FUNC(dev->devfn) << 8) | (where & ~0x03);
        PCI_CONFIG_REG = addr;
	*val = PCI_DATA_REG;
	PCI_PRINTF("addr = %08x / val = %08x || ", addr, *val);
/*
	if ((where >= 0x10) && (where <= 0x24)) {
		if ((*val & 0xfff00000) == SL2312_PCI_IO_BASE) {
			*val &= 0x000fffff;
			*val |= IO_ADDRESS(SL2312_PCI_IO_BASE);
		}
	}
*/
	return PCIBIOS_SUCCESSFUL;
}

int sl2312_write_config_word(struct pci_dev *dev, int where, u16 val)
{
	unsigned long addr, data;
	unsigned long flags;

	addr = 0x80000000 | (PCI_SLOT(dev->devfn) << 11) | (PCI_FUNC(dev->devfn) << 8) | (where & ~0x03);
	PCI_CONFIG_REG = addr;
	data = PCI_DATA_REG;
	data &= ~(0xffff << ((where & 0x02) * 8));
	data |= (val << ((where & 0x02) * 8));
	PCI_DATA_REG = data;

	return PCIBIOS_SUCCESSFUL;
}

int sl2312_write_config_dword(struct pci_dev *dev, int where, u32 val)
{
	unsigned long addr;
	unsigned long flags;
/*
	if ((where >= 0x10) && (where <= 0x24)) {
		if ((val & 0xfff00000) == IO_ADDRESS(SL2312_PCI_IO_BASE)) {
			val &= 0x000fffff;
			val |= SL2312_PCI_IO_BASE;
		}
	}
*/
	addr = 0x80000000 | (PCI_SLOT(dev->devfn) << 11) | (PCI_FUNC(dev->devfn) << 8) | (where & ~0x03);
	PCI_CONFIG_REG = addr;
	PCI_DATA_REG = val;

	return PCIBIOS_SUCCESSFUL;
}

//debug on 04/17/2006 for VIA VT6212
#define PID_VIDN 0x00351033	//NEC uPD720101
#define PID_VIDV 0x30381106	//VIA VT6212
static uPD72010_exist = 0;
static VT6212_exist = 0;
static u32 OHCI_Host_Contorller_BAR[3];

int ast1000Detect = 0;
int ast1200Detect = 0;
int mdin380Detect = 0;
struct pci_dev astPciDev;
struct pci_dev mdinPciDev;

void scan_pci_bus(void)
{
	struct pci_dev SPCI;
	u32 dev, fun, vendorID, hdr_type=0, temp;

	hal_delay_us(100000);  
	sl2312_PCI_soft_reset();

	hal_delay_us(100000);  
	sl2312_PCI_clk_33MHz();

	hal_delay_us(1000000);  
	sl2312_enable_drive_PCI_clk();

	hal_delay_us(100000);  
/*
	if (REG32(SL2312_GLOBAL_BASE + GLOBAL_IO_PIN_DRIVING) |=  0x05000000 )
            printf("\n      GPIO Driving Capability Control Set O.K. !! ");
        else
            printf("\n      GPIO Driving Capability Control Set Fail !! ");
*/

	REG32(SL2312_GLOBAL_BASE + GLOBAL_IO_PIN_DRIVING) |=  0x05000000 ;
	hal_delay_us(10000);

/*	if (REG32(SL2312_GLOBAL_BASE + GLOBAL_IO_PIN_SLEW_RATE) |=  0x00000080)
            printf("\n      GPIO Slew Rate Control Register Set O.K. !! ");
        else
            printf("\n      GPIO Slew Rate Control Register Set Fail !! ");
*/
	REG32(SL2312_GLOBAL_BASE + GLOBAL_IO_PIN_SLEW_RATE) |=  0x00000080 ;
	hal_delay_us(10000);

	//sl2312_read_miscellaneous_reg();
	SPCI.devfn = 0;

/* mark  
	if ( sl2312_read_config_word(&SPCI, PCI_COMMAND, &temp) )
             printf("\n      Read Config Word O.K. !! ");
        else 
             printf("\n      Read Config Word Fail !! ");
    temp |= 0x00000017;
	if (sl2312_write_config_word(&SPCI, PCI_COMMAND, temp) )
             printf("\n      Write Config value 0x00000017 by word O.K. !!");
        else
             printf("\n      Write Config value 0x00000017 by word Fail !!");

	//sl2312_read_config_word(&SPCI, PCI_COMMAND, &temp);
	//PCI_PRINTF("==>PCI UBS0:PCI_COMMAND = 0x%08x \n", temp);

	//sl2312_read_config_dword(&SPCI, SL2312_PCI_CTRL1, &temp);
	//PCI_PRINTF("==>PCI_Bus0:SL2312_PCI_CTRL1 = 0x%08x \n", temp);
	temp = 0x000A0A00;
	if (sl2312_write_config_dword(&SPCI, SL2312_PCI_CTRL1, temp))
             printf("\n      Write Config value 0x000A0A00 by dword O.K. !!");
        else
             printf("\n      Write Config value 0x000A0A00 by dword Fail !!");

//	sl2312_read_config_dword(&SPCI, SL2312_PCI_CTRL2, &temp);
//	temp |= 0x03C00000;
//	sl2312_write_config_dword(&SPCI, SL2312_PCI_CTRL2, temp);

	temp = (SL2312_PCI_DMA_MEM1_BASE & 0xfff00000) | (SL2312_PCI_DMA_MEM1_SIZE << 16);
        printf("\n      The DMA Memory size is locate %x",temp);
        if (sl2312_write_config_dword(&SPCI, SL2312_PCI_MEM1_BASE_SIZE, temp))
             printf("\n      Write Config on Memory base by dword O.K. !!");
        else
             printf("\n      Write Config on Memory base by dword Fail !!");
        if (sl2312_read_config_dword(&SPCI, SL2312_PCI_MEM1_BASE_SIZE, &temp))
             printf("\n      Read Config on Memory base by dword O.K. !!");
        else
             printf("\n      Read Config on Memory base by dword Fail !!");
   
    PCI_PRINTF("==>PCI Bus0:SL2312_PCI_MEM1_BASE_SIZE = 0x%08x \n", temp);
*/
        //printf("\n      Scan 32 device on Bus 0 \n");

	printf("PCI Device Information:\n");

	for(dev = 0; dev < 32; dev++)
	{
		fun = 0;
		SPCI.devfn = PCI_DEVFN(dev, fun);

		if((sl2312_read_config_dword(&SPCI, PCI_VENDOR_ID, &vendorID)))
		{
			printf("      Device Num %d ",dev);
			printf("SPCI.devfn = %03x ==> Read configuration error!!!\n", SPCI.devfn);
		}
		else
		{
			PCI_PRINTF("SPCI.devfn = %08x ==> Read configuration vendorID = %08x \n", SPCI.devfn, vendorID);

			if (vendorID != 0xffffffff)
			{
				printf("    Device Num %2d -> VendorID is %08x ", dev, vendorID);

				if (vendorID == 0x4321159b)
					printf("(PCI Bridge)");
				else if (vendorID == 0x6ff49229)
				  {
					printf("(AST1000)");
					astPciDev = SPCI;
					ast1000Detect = 1;
				  }
				else if (vendorID == 0x6ff09339)
				  {
					printf("(AST1200)");
					astPciDev = SPCI;
					ast1200Detect = 1;
				  }
				else if (vendorID == 0x030017a1)
				  {
					mdinPciDev = SPCI;
					mdin380Detect = 1;
					printf("(MDIN380)");
				  }

				printf("\n");
			}

#ifdef VIA_USB
			if(PID_VIDV == vendorID)
			{
				for(fun = 0; fun < 3; fun++)
				{
					SPCI.devfn = PCI_DEVFN(dev, fun);
					temp = 0;
					// Memory access enable
					sl2312_read_config_word(&SPCI, PCI_COMMAND, &temp);
    				temp |= 0x00000006;
					sl2312_write_config_word(&SPCI, PCI_COMMAND, temp);
					sl2312_read_config_word(&SPCI, PCI_COMMAND, &temp);
					// assign memory base address
					OHCI_Host_Contorller_BAR[fun] = (u32)(SL2312_PCI_MEM_BASE + ((fun + 1) * 0x00100000));
					sl2312_write_config_dword(&SPCI, PCI_BASE_ADDRESS_0, OHCI_Host_Contorller_BAR[fun]);
					//sl2312_read_config_dword(&SPCI, PCI_BASE_ADDRESS_0, &OHCI_Host_Contorller_BAR[fun]);
					PCI_PRINTF("	==> Host controller %d BAR = %08x PCI_COMMAND = 0x%08x \n", fun, OHCI_Host_Contorller_BAR[fun], temp);
					printf("...");
				}

				//debug
				//uPD72010_exist = 1;
				//printf(" ***Find NEC uPD720101***\n");
				VT6212_exist = 1;
				printf(" Found VIA VT6212 ");
			}
#endif

#ifdef NEC_USB
			//Add 
			if(PID_VIDN == vendorID)
			{
				for(fun = 0; fun < 3; fun++)
				{
					SPCI.devfn = PCI_DEVFN(dev, fun);
					temp = 0;
					// Memory access enable
					sl2312_read_config_word(&SPCI, PCI_COMMAND, &temp);
    				temp |= 0x00000006;
					sl2312_write_config_word(&SPCI, PCI_COMMAND, temp);
					sl2312_read_config_word(&SPCI, PCI_COMMAND, &temp);
					// assign memory base address
					OHCI_Host_Contorller_BAR[fun] = (u32)(SL2312_PCI_MEM_BASE + ((fun + 1) * 0x00100000));
					sl2312_write_config_dword(&SPCI, PCI_BASE_ADDRESS_0, OHCI_Host_Contorller_BAR[fun]);
					//sl2312_read_config_dword(&SPCI, PCI_BASE_ADDRESS_0, &OHCI_Host_Contorller_BAR[fun]);
					PCI_PRINTF("	==> Host controller %d BAR = %08x PCI_COMMAND = 0x%08x \n", fun, OHCI_Host_Contorller_BAR[fun], temp);
					printf("...");
				}

				//debug
				uPD72010_exist = 1;
				printf(" Found NEC uPD720101 ");			
			}

#endif

			//if(sl2312_read_config_byte(&SPCI, PCI_HEADER_TYPE, &hdr_type))
			//	continue;

			//PCI_PRINTF("====== PCI_HEADER_TYPE = %04x (PCI_CONFIG_REG = %08x / PCI_DATA_REG = %08x)====== \n", hdr_type, &PCI_CONFIG_REG, &PCI_DATA_REG);
			/*if(hdr_type & 0x80)	//multi-function device
			{
				for(fun = 0; fun < 8; fun++)
				{
					SPCI.devfn = PCI_DEVFN(dev, fun);
					if((sl2312_read_config_dword(&SPCI, PCI_VENDOR_ID, &vendorID)))
					{
						PCI_PRINTF("==> SPCI.devfn = %08x ==> Read configuration error!!!\n", SPCI.devfn);
					}
					//else
					//{
						//PCI_PRINTF("==> SPCI.devfn = %08x ==> Read configuration vendorID = %08 \n", SPCI.devfn, vendorID);
					//}
				}
			}*/
		}
	}
}

// input host number
// return OHCI memory base address(0x)
#ifdef NEC_USB
u32 is_uPD72010_existed(int Host_Num)
{
	
	//debug
	//printf("%s: Host_Num=%d\r\n", __func__, Host_Num);
	
	if (uPD72010_exist)
	{
		//debug
		//printf("%s: Host_Num=%d, port=0x%x\r\n", __func__, Host_Num, OHCI_Host_Contorller_BAR[Host_Num]);
		return OHCI_Host_Contorller_BAR[Host_Num];
	}
	else
	{
		return 0;
	}
}
#endif
	
	//Added 

#ifdef VIA_USB
u32 is_VT6212_existed(int Host_Num)
{
	if (VT6212_exist)
	{
		//debug
		//printf("%s: Host_Num=%d, port=0x%x\r\n", __func__, Host_Num, OHCI_Host_Contorller_BAR[Host_Num]);
		return OHCI_Host_Contorller_BAR[Host_Num];
	}
	else
	{
		return 0;
	}
}
#endif

