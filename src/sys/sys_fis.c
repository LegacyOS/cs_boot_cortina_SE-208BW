/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_fis.c
* Description	: 
*		Handle FLASH Image System (FIS)
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	08/01/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include <sys_fis.h>

//static unsigned char *fis_datap;
unsigned char *fis_datap;

static void fis_create_default(void);
//static unsigned long fis_desc_chksum(FIS_T *img);
unsigned long fis_desc_chksum(FIS_T *img);
static FIS_T *fis_get_free_image(void);
//static void fis_write_flash(void);
void fis_write_flash(void);

#ifdef BOARD_SUPPORT_TWO_CPU
extern char     _exception_handlers[];
extern char     _exception_handlers_end[];
#endif

#ifdef BOARD_NAND_BOOT	
extern UINT32 info_flash[12];
#endif

/*----------------------------------------------------------------------
* sys_fis_init
*	looking for BOOT-1, BOOT-2, Kernel & RAM-Disk file image.
*	If Kernel & RAM-DISK files are found, return 0
*	If not found, create FIS with default Boot-1 and Boot-2 
*	return -1 if error
*----------------------------------------------------------------------*/
int sys_fis_init(void) 
{
	int i;
	FIS_T *img;
	char *datap;
	UINT32 ipaddr, netmask, gateway;
	
	fis_datap = (unsigned char *)sys_malloc(FIS_TOTAL_SIZE);
	if (!fis_datap)
	{
		printf("sys_fis_init: No free memory!\n");
		return -1;
	}
	
	// get image files from FLASH
	hal_flash_enable();
	memcpy(fis_datap, BOARD_FLASH_FIS_ADDR, FIS_TOTAL_SIZE);
	hal_flash_disable();
	
	// check FIS existed or not
	// create one if not found
	// check kernel
	img = fis_find_image(BOARD_FIS_BOOT_NAME);
	if (!img)
	{
		printf("FIS is not found! Create a default FIS!\n");
		fis_create_default();
		return -1;
	}
	
	// get IP address
	datap = img->data;
	ipaddr = str2ip(datap);
	datap += strlen(datap) + 1;
	netmask = str2ip(datap);
	datap += strlen(datap) + 1;
	gateway = str2ip(datap);
	sys_set_ip_addr(ipaddr);
	sys_set_ip_netmask(netmask);
	sys_set_ip_gateway(gateway);
	
	// check kernel
	img = fis_find_image(BOARD_FIS_KERNEL_NAME);
	if (!img || !img->file.mem_base || !img->file.entry_point)
	{
		printf("Could not load the Kernel!\n");
		return -1;
	}
	
	// check ram disk
#if (BOARD_RAM_DISK_SIZE != 0)
	img = fis_find_image(BOARD_FIS_RAM_DISK_NAME);
	if (!img || !img->file.mem_base)
	{
		printf("Could not load the RAM Disk!\n");
		return -1;
	}
#endif
		
	return 0;
}

/*----------------------------------------------------------------------
* fis_load_images
* load image to dram
*----------------------------------------------------------------------*/
void fis_load_images(void)
{
	FIS_T *img;
	int i, j;
    unsigned long old_ints;
    unsigned long *dest, *srce, size;
    void (*apps_routine)(void);
    unsigned int    value;
#ifdef BOARD_SUPPORT_TWO_CPU
	unsigned char *p ;
	unsigned char *tgt0 ;
	unsigned int cpu2_base=BOARD_DRAM_CPU2_ADDR;
#endif  
	
    HAL_DISABLE_INTERRUPTS(old_ints);
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_DCACHE_INVALIDATE_ALL();
    HAL_ICACHE_ENABLE();
    HAL_DCACHE_ENABLE();
    
	apps_routine = NULL;
	img = (FIS_T *)fis_datap;
	for (i=0; i<FIS_MAX_ENTRY; i++, img++)
	{
		if (img->file.name[0] != 0xff && img->file.name[0] != 0x00 &&
			img->desc_cksum == fis_desc_chksum(img) &&
			img->file.flash_base && img->file.mem_base)
		{
			if ((strncasecmp(img->file.name, BOARD_FIS_BOOT_NAME) == 0)
				|| (strncasecmp(img->file.name, BOARD_FIS_DIRECTORY_NAME) == 0)
				|| (img->file.entry_point == 0))
			{
				continue;
			}
			if (!apps_routine && img->file.entry_point)
				apps_routine = (void *)img->file.entry_point;
				
    		srce=(unsigned long *)img->file.flash_base;
    		dest=(unsigned long *)img->file.mem_base;
    		//size = ((img->file.size + 3) & ~3)/ 4;

#ifdef BOARD_SUPPORT_TWO_CPU
		//get fis kern2
		if (strncasecmp(img->file.name, BOARD_FIS_CPU2_NAME) == 0)
		{
			cpu2_base = img->file.entry_point - CPU2_BOOT_OFFSET;
		}
#endif			
			printf("Load %s image from 0x%x to 0x%x size %d\n",
					img->file.name, img->file.flash_base, img->file.mem_base, img->file.data_length);//img->file.size);
			
			hal_flash_enable();
			value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;	
			if((value&0x1000)==0x1000)
			{
				//size = img->file.size;
				size = img->file.data_length;
				sys_memcpy(dest, srce, size );
			}
			else
			{
				size = ((img->file.size + 3) & ~3)/ 4;
    			for (j = 0; j < size; j++)
    				*dest++=*srce++;
			}
			hal_flash_disable();
		}
		
	}

#ifdef BOARD_SUPPORT_TWO_CPU

#if (CPU2_BOOT_OFFSET != 0)	
	// install vector
	//memset(dest,0xE1A00000,0x2000000);	// set 32MB as nop for safe
	size = (unsigned int)(&_exception_handlers_end[0] - &_exception_handlers[0]);
	p = (unsigned char *)(&_exception_handlers[0]);
//	tgt0 = (unsigned char *)BOARD_DRAM_CPU2_ADDR;
	tgt0 = (unsigned char *)cpu2_base;
	while(size--) {
		*tgt0 = *p;
		tgt0++; p++;
	}
#endif	
	
	HAL_ICACHE_INVALIDATE_ALL();
	HAL_DCACHE_INVALIDATE_ALL();
    
	REG32(0x40000030) |= 0x00010040;	// Enable LPC
    
	printf("Start the Core 2 !\n");
//	REG32(0x66000040) = CPU2_RAM_BASE + CPU2_RAM_SIZE;
	if(cpu2_base == 0xE000000) //224-32
		REG32(0x66000040) = cpu2_base + 32;
	else if(cpu2_base == 0x6000000)  ////96-32
		REG32(0x66000040) = cpu2_base + 32;
	else  //64-64
		REG32(0x66000040) = cpu2_base + 64;

		
	//REG32(GLOBAL_CTRL_CPU1_REG) &= ~CPU1_RESET_BIT_MASK;
	 
	//REG32(0x50000008) = 0x16;	// PCI
	printf("Start the Core 1 !\n");

#endif
	
	REG32(0x50000008) = 0x16;       // PCI

	if (apps_routine)
	{
		hal_interrupt_mask_all();
		HAL_DISABLE_INTERRUPTS(old_ints);
    	//HAL_ICACHE_INVALIDATE_ALL();
    	//HAL_DCACHE_INVALIDATE_ALL();
    	//HAL_ICACHE_DISABLE();
    	//HAL_DCACHE_DISABLE();
    
		sys_run_apps(apps_routine);
	}
	
	HAL_RESTORE_INTERRUPTS(old_ints);
	HAL_ICACHE_INVALIDATE_ALL();
	HAL_DCACHE_INVALIDATE_ALL();
	HAL_ICACHE_ENABLE();
}


/*----------------------------------------------------------------------
* sys_fis_list
*----------------------------------------------------------------------*/
void fis_ui_list(int type)
{
	FIS_T *img;
	int i;
	
	if (!fis_datap)
	{
		printf("No FIS!\n");
		return;
	}
	printf("Name              FLASH addr           Mem addr    Datalen     Entry point\n");
	img = (FIS_T *)fis_datap;
	for (i=0; i<FIS_MAX_ENTRY; i++, img++)
	{
		if (img->file.name[0] != 0xff && img->file.name[0] != 0x00 &&
			img->desc_cksum == fis_desc_chksum(img))
		{
			printf("%-16s  0x%08lX-%08lX  0x%08lX  0x%08lX  0x%08lX\n",
					img->file.name,
					img->file.flash_base,
					(img->file.size) ? img->file.flash_base+img->file.size-1 : img->file.flash_base,
					img->file.mem_base,
					img->file.data_length,
					img->file.entry_point);
		}
	}
}

/*----------------------------------------------------------------------
* fis_desc_chksum
*	do file descriptor check sum
*----------------------------------------------------------------------*/
unsigned long fis_desc_chksum(FIS_T *img)
{
	unsigned char *cp;
	unsigned long sum = 0;
	int i;
	
	cp  = (unsigned char *)img;
	for (i=0; i<(FIS_ENTRY_SIZE - 8); i++, cp++) // not include chksum field
		sum += *cp & 0xff;

	
	return sum;
}
	


/*----------------------------------------------------------------------
* fis_find_image
*----------------------------------------------------------------------*/
FIS_T *fis_find_image(char *filename)
{
	FIS_T *img;
	int i;
	
	img = (FIS_T *)fis_datap;
	
	for (i=0; i<FIS_MAX_ENTRY; i++, img++)
	{
		if (img->file.name[0] == 0xff)
			break;
			
		if ((strncasecmp(filename, img->file.name) == 0) && 
			(img->desc_cksum == fis_desc_chksum(img)))
		{
			return img;
		}
	}
	return NULL;
}

/*----------------------------------------------------------------------
* fis_get_free_image
*----------------------------------------------------------------------*/
static FIS_T *fis_get_free_image(void)
{
	FIS_T *img;
	int i;
	
	img = (FIS_T *)fis_datap;
	
	for (i=0; i<FIS_MAX_ENTRY; i++, img++)
	{
		if (img->file.name[0] == 0xff || img->file.name[0] == 0
			|| img->desc_cksum != fis_desc_chksum(img))
			return img;
	}
	return NULL;
}


/*----------------------------------------------------------------------
* fis_create_default
*----------------------------------------------------------------------*/
static void fis_create_default(void)
{
	FIS_T	*img;
	UINT32 ipaddr, netmask, gateway;
	char *datap;
	
	img = (FIS_T *)fis_datap;
	
	memset((char *)fis_datap, -1, FIS_TOTAL_SIZE);
	
	// boot-1
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_BOOT_NAME);
	img->file.flash_base	= BOARD_FLASH_BOOT_ADDR;
	img->file.mem_base		= 0;//BOARD_FLASH_BOOT_ADDR;
	img->file.size			= BOARD_FLASH_BOOT_SIZE;
	img->file.entry_point	= 0; //BOARD_FLASH_BOOT_ADDR;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_FLASH_BOOTIMG_SIZE;
#else	
	img->file.data_length	= BOARD_FLASH_BOOT_SIZE;
#endif
	
	ipaddr = sys_get_ip_addr();
	netmask = sys_get_ip_netmask();
	gateway = sys_get_ip_gateway();
	datap = img->data;
	sprintf(datap, "%d.%d.%d.%d",
			IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr));
	datap += strlen(datap) + 1;
	sprintf(datap, "%d.%d.%d.%d",
			IP1(netmask), IP2(netmask), IP3(netmask), IP4(netmask));
	datap += strlen(datap) + 1;
	sprintf(datap, "%d.%d.%d.%d",
			IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway));
	
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	
	// FIS directory
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_DIRECTORY_NAME);
	img->file.flash_base	= BOARD_FLASH_FIS_ADDR;
	img->file.mem_base		= 0;//BOARD_FLASH_FIS_ADDR;
	img->file.size			= BOARD_FLASH_FIS_SIZE;
	img->file.entry_point	= 0;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_FLASH_FISIMG_SIZE;
#else		
	img->file.data_length	= BOARD_FLASH_FIS_SIZE;//FIS_TOTAL_SIZE;
#endif	
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	
	// kernel
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_KERNEL_NAME);
	img->file.flash_base	= BOARD_FLASH_KERNEL_ADDR;
	img->file.mem_base		= BOARD_DRAM_KERNEL_ADDR;
	img->file.size			= BOARD_KERNEL_SIZE;
	img->file.entry_point	= BOARD_DRAM_KERNEL_ADDR;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_KERNELIMG_SIZE;
#else		
	img->file.data_length	= BOARD_KERNEL_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;

	// Configuration
	#ifndef BOARD_SUPPORT_TWO_CPU
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_CONFIG_NAME);
	img->file.flash_base	= BOARD_FLASH_CONFIG_ADDR;
	img->file.mem_base		= 0;
	img->file.size			= BOARD_FLASH_CONFIG_SIZE;
	img->file.entry_point	= 0;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_FLASH_CONFIGIMG_SIZE;
#else			
	img->file.data_length	= BOARD_FLASH_CONFIG_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	#endif
	
#ifndef NETBSD	
	// Ram Disk
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_RAM_DISK_NAME);
	img->file.flash_base	= BOARD_FLASH_RAM_DISK_ADDR;
	img->file.mem_base		= BOARD_DRAM_RAM_DISK_ADDR;
	img->file.size			= BOARD_RAM_DISK_SIZE;
	img->file.entry_point	= BOARD_DRAM_RAM_DISK_ADDR; //0;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_RAM_DISKIMG_SIZE;
#else			
	img->file.data_length	= BOARD_RAM_DISK_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;

		
	// Application
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_APPS_NAME);
	img->file.flash_base	= BOARD_FLASH_APPS_ADDR;
	img->file.mem_base		= 0;
	img->file.size			= BOARD_APPS_SIZE;
	img->file.entry_point	= 0;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_APPSIMG_SIZE;
#else			
	img->file.data_length	= BOARD_APPS_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;


#ifdef BOARD_SUPPORT_TWO_CPU
	// CPU2 code
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_CPU2_NAME);
	img->file.flash_base	= BOARD_FLASH_CPU2_ADDR;
	img->file.mem_base		= BOARD_DRAM_KERNEL2_ADDR;
	img->file.size			= BOARD_CPU2_SIZE;
	img->file.entry_point	= BOARD_DRAM_KERNEL2_ADDR;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_CPU2IMG_SIZE;
#else			
	img->file.data_length	= BOARD_CPU2_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	
	// CPU2 Ramdisk code
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_RAM_DISK2_NAME);
	img->file.flash_base	= BOARD_FLASH_CPU2_RD_ADDR;
	img->file.mem_base		= BOARD_DRAM_RAM_DISK2_ADDR;
	img->file.size			= BOARD_CPU2_RD_SIZE;
	img->file.entry_point	= BOARD_DRAM_RAM_DISK2_ADDR;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_CPU2_RDIMG_SIZE;
#else		
	img->file.data_length	= BOARD_CPU2_RD_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;

#ifdef BOARD_NAND_BOOT		
	// Configure
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, "Configure");
	img->file.flash_base	= BOARD_FLASH_CONFIG_ADDR;
	img->file.mem_base		= 0;
	img->file.size			= BOARD_FLASH_CONFIG_SIZE;
	img->file.entry_point	= 0;
	img->file.data_length	= BOARD_FLASH_CONFIGIMG_SIZE;
	img->desc_cksum			= fis_desc_chksum(img);
	img++;

	// eCos
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, "eCos");
	img->file.flash_base	= BOARD_ECOS_ADDR;
	img->file.mem_base		= 0;
	img->file.size			= BOARD_FLASH_CONFIG_SIZE;
	img->file.entry_point	= 0;
	img->file.data_length	= BOARD_ECOSIMG_SIZE;
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	
	
#endif
	
#endif

#endif //netbsd	
	// VCTL
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, BOARD_FIS_VCTL_NAME);
	img->file.flash_base	= BOARD_FLASH_VCTL_ADDR;
	img->file.mem_base		= 0;
	img->file.size			= BOARD_FLASH_VCTL_SIZE;
	img->file.entry_point	= 0;
#ifdef BOARD_NAND_BOOT		
	img->file.data_length	= BOARD_FLASH_VCTLIMG_SIZE;
#else		
	img->file.data_length	= BOARD_FLASH_VCTL_SIZE;
#endif
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	
#ifdef BOARD_NAND_BOOT		
	// Reserved
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, "Reserved");
	img->file.flash_base	= BOARD_RESV_ADDR;
	img->file.mem_base		= 0;
	img->file.size			= BOARD_RESV_SIZE;
	img->file.entry_point	= 0;
	img->file.data_length	= BOARD_RESVIMG_SIZE;
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
	
	
#endif	
/*
#ifdef BOARD_NAND_BOOT		
	// FS
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	strcpy(img->file.name, "FS");
	img->file.flash_base	= 0x32000000;
	img->file.mem_base		= 0;
	img->file.size			= 0x1000000;
	img->file.entry_point	= 0;
	img->file.data_length	= 0x1000000;
	img->desc_cksum			= fis_desc_chksum(img);
	img++;
#endif
*/
	fis_write_flash();
}
/*----------------------------------------------------------------------
* fis_set_ip_addr
*----------------------------------------------------------------------*/
void fis_write_flash(void)
{
#if BOARD_SUPPORT_FIS
    unsigned long	err_addr;
	int				stat,fis_size;
	
	printf("\nStart Update FIS data......\n");

#ifdef BOARD_NAND_BOOT		
	FIS_T *img;
	int i;
	img = (FIS_T *)fis_datap;
	
	fis_size = FIS_TOTAL_SIZE;
	
	for (i=0; i<FIS_MAX_ENTRY; i++, img++)
	{
		if (img->file.name[0] != 0xff && img->file.name[0] != 0x00 &&
			img->desc_cksum == fis_desc_chksum(img) &&
			(img->file.flash_base == BOARD_FLASH_FIS_ADDR))
		{
			break;
		}
	}
	printf("\nProgram flash (0x%x): Size=%u ", BOARD_FLASH_FIS_ADDR, fis_size);
	if ((stat = flash_nand_program((void *)BOARD_FLASH_FIS_ADDR, (void *)fis_datap, fis_size,  (unsigned long *)&err_addr, img->file.size)) != 0)
	{
		printf(" FAILED at 0x%x: ", err_addr);
		printf((char *)flash_errmsg(stat));
		printf("\n"); 
		return;
	}
	else
	{
		printf(" OK!\n"); 
		return;
	}
	
#else
	fis_size = BOARD_FLASH_FIS_SIZE;

	printf("Erase flash (0x%x): Size=%u ", BOARD_FLASH_FIS_ADDR, fis_size); 
 	if ((stat = flash_erase((void *)BOARD_FLASH_FIS_ADDR, 
    							    fis_size,
    							    (unsigned long *)&err_addr)) != 0)
	{
		printf(" FAILED at 0x%x: ", err_addr);
		printf((char *)flash_errmsg(stat));
		printf("\n"); 
		return;
	}
	printf(" OK!\n"); 
	printf("Program flash (0x%x): Size=%u ", BOARD_FLASH_FIS_ADDR, fis_size);//FIS_TOTAL_SIZE); 
	if ((stat = flash_program((void *)BOARD_FLASH_FIS_ADDR, 
    						  (void *)fis_datap, 
    						  fis_size,//FIS_TOTAL_SIZE, 
	    					  (unsigned long *)&err_addr)) != 0)
	{
		printf(" FAILED at 0x%x: ", err_addr);
		printf((char *)flash_errmsg(stat));
		printf("\n"); 
		return;
	}
	else
		printf(" OK!\n"); 
#endif	
#endif
}

/*----------------------------------------------------------------------
* fis_set_ip_addr
*----------------------------------------------------------------------*/
void fis_set_ip_addr(UINT32 ipaddr)
{
	FIS_T	*img;
	UINT32  netmask, gateway;
	char	*datap;

	img = fis_find_image(BOARD_FIS_BOOT_NAME);
	if (!img)
	{
		printf("FIS is not found! Create a default FIS!\n");
		sys_set_ip_addr(ipaddr);
		fis_create_default();
		return;
	}
	
	netmask = sys_get_ip_netmask();
	gateway = sys_get_ip_gateway();
	datap = img->data;
	sprintf(datap, "%d.%d.%d.%d",
			IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr));
	datap += strlen(datap) + 1;
	sprintf(datap, "%d.%d.%d.%d",
			IP1(netmask), IP2(netmask), IP3(netmask), IP4(netmask));
	datap += strlen(datap) + 1;
	sprintf(datap, "%d.%d.%d.%d",
			IP1(gateway), IP2(gateway), IP3(gateway), IP4(gateway));
	
	img->desc_cksum = fis_desc_chksum(img);
	fis_write_flash();
}	

/*----------------------------------------------------------------------
* fis_ui_delete_image
*----------------------------------------------------------------------*/
void fis_ui_delete_image(int type)
{
	FIS_T *img;
	char name[FIS_NAME_SIZE];
	int rc;
	
	name[0] = 0x00;
	fis_ui_list(0);
	printf("Input image name: ");
	rc = ui_gets(name, sizeof(name));
	if (rc)
		img = fis_find_image(name);
	if (rc == 0 || !img)
	{
		printf("Image is not found!\n");
		return;
	}
	
	if ((strncasecmp(name, BOARD_FIS_BOOT_NAME) == 0)
		|| (strncasecmp(name, BOARD_FIS_DIRECTORY_NAME) == 0))
	{
		printf("Cannot delete %s or %s image!\n", BOARD_FIS_BOOT_NAME, BOARD_FIS_DIRECTORY_NAME);
		return;
	}
		
	memset((char *)img, 0, FIS_ENTRY_SIZE);
	fis_write_flash();
}

/*----------------------------------------------------------------------
* sys_fis_create_image
*----------------------------------------------------------------------*/
void fis_ui_create_image(int type)
{
	FIS_T	*img;
	char	name[FIS_NAME_SIZE];
	char	buf[20];
	int		rc;
	int		max_size, total_size;
	unsigned long flash_addr, ram_addr, entry_point;

	printf("Input Image Name: ");
	name[0] = 0x00;
	rc = ui_gets(name, sizeof(name));
	if (rc == 0)
		return;
	//img = fis_find_image(name);
	//if (img)
	//{
	//	printf("Image is existed!(%s, %x)\n", name, img);
	//	return;
	//}
	//img = fis_get_free_image();
	//if (!img)
	//{
	//	printf("No free image space!\n");
	//	return;
	//}
	
	img = fis_find_image(name);
	if (!img)
	{
		img = fis_get_free_image();
		if (!img)
		{
			printf("No free image space!\n");
			return;
		}
	}
	
	
	printf("Input Flash Address: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	flash_addr = str2hex(buf);
#ifdef BOARD_NAND_BOOT	
	if (flash_addr < BOARD_FLASH_BASE_ADDR || 
		flash_addr > (BOARD_FLASH_BASE_ADDR +(info_flash[1]<<20)))
#else
	if (flash_addr < BOARD_FLASH_BASE_ADDR || 
		flash_addr > (BOARD_FLASH_BASE_ADDR + BOARD_FLASH_SIZE))
#endif		
	{
		printf("Address is NOT in FLASH range!\n");
		return;
	}
	
	printf("Input Memory Address: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	ram_addr = str2hex(buf);
	if (ram_addr > (BOARD_DRAM_BASE_ADDR + BOARD_DRAM_SIZE))
	{
		printf("Address is NOT in DRAM range!\n");
		return;
	}
	
	printf("Input Entry Point: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	entry_point = str2hex(buf);
	if (entry_point > (BOARD_DRAM_BASE_ADDR + BOARD_DRAM_SIZE))
	{
		printf("Address is NOT in DRAM range!\n");
		return;
	}
	
#if 1	
	printf("Input Max Size: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	max_size = str2hex(buf);
#ifdef BOARD_NAND_BOOT		
	if (max_size > (BOARD_FLASH_SIZE - BOARD_FLASH_BOOTIMG_SIZE))
#else	
	if (max_size > (BOARD_FLASH_SIZE - BOARD_FLASH_BOOT_SIZE))
#endif		
	{
		printf("Size is too large!\n");
		return;
	}
#endif
	
	total_size = ui_download(flash_addr, 0, 0);
	if (total_size > 0)
	{
#ifdef BOARD_NAND_BOOT			
		if ((strncasecmp(name, BOARD_FIS_DIRECTORY_NAME) == 0)&&(BOARD_FLASH_FISIMG_SIZE >= total_size))
#else	
		if ((strncasecmp(name, BOARD_FIS_DIRECTORY_NAME) == 0)&&(BOARD_FLASH_FIS_SIZE >= total_size))
#endif			
		{
			// get image files from FLASH
			hal_flash_enable();
			memcpy(fis_datap, BOARD_FLASH_FIS_ADDR, FIS_TOTAL_SIZE);
			hal_flash_disable();
		}
		else
		{
			total_size = max_size;
			memset((char *)img, 0, FIS_ENTRY_SIZE);
			strcpy(img->file.name, name);
			img->file.flash_base	= flash_addr;
			img->file.mem_base		= ram_addr;
			img->file.size			= max_size;//total_size;
			img->file.entry_point	= entry_point;
			img->file.data_length	= total_size;
			img->desc_cksum			= fis_desc_chksum(img);
			fis_write_flash();
		}
			
	}
}

/*----------------------------------------------------------------------
* fis_ui_over_write_image
*----------------------------------------------------------------------*/
void fis_ui_over_write_image(int type)
{
	FIS_T	*img;
	char	name[FIS_NAME_SIZE];
	char	buf[20];
	int		rc;
	int		max_size, total_size,img_size;
	unsigned long flash_addr, ram_addr, entry_point;

	printf("Input Image Name: ");
	name[0] = 0x00;
	rc = ui_gets(name, sizeof(name));
	if (rc == 0)
		return;
	img = fis_find_image(name);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", name, img);
		return;
	}

	
	printf("Input Flash Address: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	flash_addr = str2hex(buf);
#ifdef BOARD_NAND_BOOT		
	if (flash_addr < BOARD_FLASH_BASE_ADDR || 
		flash_addr > (BOARD_FLASH_BASE_ADDR + (info_flash[1]<<20)))
#else
	if (flash_addr < BOARD_FLASH_BASE_ADDR || 
		flash_addr > (BOARD_FLASH_BASE_ADDR + BOARD_FLASH_SIZE))
#endif
	{
		printf("Address is NOT in FLASH range!\n");
		return;
	}
	
	printf("Input Memory Address: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	ram_addr = str2hex(buf);
	if (ram_addr > (BOARD_DRAM_BASE_ADDR + BOARD_DRAM_SIZE))
	{
		printf("Address is NOT in DRAM range!\n");
		return;
	}
	
	printf("Input Entry Point: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	entry_point = str2hex(buf);
	if (entry_point > (BOARD_DRAM_BASE_ADDR + BOARD_DRAM_SIZE))
	{
		printf("Address is NOT in DRAM range!\n");
		return;
	}
	
#if 1	
	printf("Input Max Size: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	max_size = str2hex(buf);
#ifdef BOARD_NAND_BOOT		
	if (max_size > (BOARD_FLASH_SIZE - BOARD_FLASH_BOOTIMG_SIZE))
#else	
	if (max_size > (BOARD_FLASH_SIZE - BOARD_FLASH_BOOT_SIZE))
#endif		
	{
		printf("Size is too large!\n");
		return;
	}
#endif

#ifdef BOARD_NAND_BOOT	
	printf("Input Max Image Size: ");
	buf[0] = 0x00;
	rc = ui_gets(buf, sizeof(buf));
	if (rc == 0)
		return;
	img_size = str2hex(buf);	
	if (img_size > (BOARD_FLASH_SIZE - BOARD_FLASH_BOOTIMG_SIZE))
	{
		printf("Size is too large!\n");
		return;
	}
	
	total_size = ui_download(flash_addr, img_size, max_size);
#else
	total_size = ui_download(flash_addr, 0, 0);
	
#endif	
	if (total_size > 0)
	{
		total_size = max_size;
		memset((char *)img, 0, FIS_ENTRY_SIZE);
		strcpy(img->file.name, name);
		img->file.flash_base	= flash_addr;
		img->file.mem_base		= ram_addr;
		img->file.size			= max_size;//total_size;
		img->file.entry_point	= entry_point;
#ifdef BOARD_NAND_BOOT			
		img->file.data_length	= img_size;
#else
		img->file.data_length	= total_size;
#endif
		img->desc_cksum			= fis_desc_chksum(img);
		fis_write_flash();
	}
}

/*----------------------------------------------------------------------
* fis_ui_create_default
*----------------------------------------------------------------------*/
void fis_ui_create_default(int type)
{
	printf("Create default FIS\n");
	fis_create_default();
}



