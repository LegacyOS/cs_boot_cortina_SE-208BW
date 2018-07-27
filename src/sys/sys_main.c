/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_main.c
* Description	: 
*		Main entry of C files
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/18/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include <sys_fis.h>
#include <ide/ide.h>
#include <ide/ext2fs.h>
#include <pci/pci.h>

#ifdef USB_DEV_MO
#include <../usb/OTGController.h>
#endif
#ifdef BOARD_SUPPORT_RAID
#include "../raid/md_p.h"
#endif

#define BOOT_CHK_KET_INTERVAL	(10 * 1000)		// 10ms 

UINT32 DRIVE_PCI_CLK = 0;

void sys_show_hw_cfg(void);
void sys_load_kernel(void);
void sys_load_kernel_from_flash(void);
void sys_load_kernel_from_ide(void);
int sys_verify_kernel_in_flash(void);

extern void board_show_version(void);
extern FILE_T *ext2fs_open(char *filepath);
extern unsigned char *fis_datap;
//extern int ide_init(void);

#ifdef USB_DEV_MO
extern int faraday_usb_main(void);
#endif

#ifdef BOARD_SUPPORT_RAID
extern int raid_init(void);
extern void sys_set_vctl_recover(UINT32 ipaddr, UINT32 netmask, UINT32 gateway,int recovery);
#endif

#ifdef RECOVER_FROM_BOOTP
//bootp
#include <net.h>
bool use_bootp=0;
extern bootp_header_t my_bootp_info;
extern int sts_mode, reset_sts;
//extern bool have_net;
extern ip_route_t     r;
#endif

void sys_run_apps(void *entry);

#ifdef BOARD_SUPPORT_TWO_CPU
extern char     _exception_handlers[];
extern char     _exception_handlers_end[];
#endif

#ifdef BOARD_SUPPORT_WEB
extern int web_wait;
#endif
extern int web_on;

int astel_serial_RS485 = 0;				// dhsul ASTEL
#define FLASH_TYPE_KERNEL	1
#define FLASH_TYPE_RAMDISK	2

/*----------------------------------------------------------------------
* sl_main
*----------------------------------------------------------------------*/
void sl_main(void) 
{
	extern void udelay();
	extern void init_astel();
	extern void slideBarMdin380(int, char); // ASTEL
	UINT64	delay_time;
	UINT32	delay_ticks, vbus = 0, regData;
	int boot_menu = 0,disk_present=0;
	int rc=0,s, i;
	UINT32 ipaddr,srvip;
	UINT8	*pbuf;
	
	hal_hardware_init();
	
	hal_flash_disable();

	if (REG32(SL2312_GPIO_BASE + 0x04) & 0x80000000)		// dhsul ASTEL
		astel_serial_RS485 = 1;
	else
		astel_serial_RS485 = 0;

	uart_init(BOARD_UART_BAUD_RATE);

	sys_init_memory((char *)BOARD_BOOT2_MALLOC_BASE, BOARD_BOOT2_MALLOC_SIZE);
	REG32(0x41000000+0x0c)=0;// disable wdt timer
#ifdef USB_DEV_MO
	vbus = REG32(SL2312_GPIO_BASE + 0x04)&BIT(17)  ; //read GPIO0[17]->data in 0x04
	if(vbus!=0)
	{
		vbus = REG32(SL2312_GPIO_BASE + 0x04)&BIT(18)  ; //read GPIO0[17]->data in 0x04
	}	//printf("vbus : %x\n",vbus);
	else
		vbus = 1;
#endif

#ifdef PWR_CIR_CTL	
	sl_cir_init();
#endif
	
	flash_init();
	
#ifdef BOARD_SUPPORT_FIS
	if (sys_fis_init() < 0)
		boot_menu = 1;
#endif
	
		
#ifdef BOARD_SUPPORT_VCTL
	sys_vctl_init();
#endif
	
	sys_init_cfg();

#ifdef BOARD_SUPPORT_WEB

	printf("\n\n");
	delay_ticks = ((BOARD_WEB_TIMEOUT+2) * BOARD_TPS);
	delay_time = sys_get_ticks() + delay_ticks;
	printf("==> Waiting for web upgrade detect within %d seconds ...... \n",(BOARD_WEB_TIMEOUT+2));
	while (sys_get_ticks() < delay_time)
	{
			UINT32 my_ip_addr;
			UINT32 my_ip_netmask;
			UINT32 my_ip_gateway;
			
			if(web_wait > (BOARD_WEB_TIMEOUT * BOARD_TPS))
			{
				int webip[4] = BOARD_DEFAULT_WEB_IP_ADDR;
				int netmask[4] = BOARD_DEFAULT_IP_NETMASK;
				int gateway[4] = BOARD_DEFAULT_GATEWAY;
 			
				// default value
				my_ip_addr = IPIV(webip[0], webip[1], webip[2], webip[3]);
				my_ip_netmask = IPIV(netmask[0], netmask[1], netmask[2], netmask[3]);
				my_ip_gateway = IPIV(gateway[0], gateway[1], gateway[2], gateway[3]);
				sys_set_ip_addr(my_ip_addr);
				sys_set_ip_netmask(my_ip_netmask);
				sys_set_ip_gateway(my_ip_gateway);
				web_on = 1;
				printf("********************************* \n");
				printf("**     Web upgrade enable      **\n");
				printf("********************************* \n");
				break;
			}
	}
#endif


#ifdef BOARD_SUPPORT_RAID	
#ifdef RECOVER_FROM_BOOTP
	if( (REG32(SL2312_GPIO_BASE + 0x04)&BIT(1)) == 0 )	// GPIO0_bit1 
		goto RECOVERY;
#endif
#endif	
	
#ifdef BOARD_SUPPORT_IDE
	if(vbus==0)
		disk_present = ide_init();
	else{
#ifdef LOAD_FROM_IDE	
	disk_present = ide_init();
#endif
	}

#ifdef LOAD_FROM_IDE
	
	if( disk_present == 0 )
		boot_menu = 1;			
#endif
#endif

#ifdef BOARD_SUPPORT_RAID
	rc = 0;
	if((vbus == 0) && (disk_present>0))
		rc = raid_init();
	if(rc==0)
		printf("No raid assembled\n");
	else
		printf("%d RAID device assembled\n",rc);
#endif

#ifdef USB_DEV_MO
		if((vbus==0)&&(disk_present>0))
			faraday_usb_main();
#endif
	
	printf("\n\n");
	board_show_version();
	REG32(SL2312_GLOBAL_BASE + 0x44)=0x80000000;
	sys_show_hw_cfg();
	sys_show_sw_cfg();

	// todo pci list
	scan_pci_bus();

	init_astel();
	
#ifdef BOARD_SUPPORT_WEB

			if(web_on == 1)
			{
				net_init();
				ui_menu();
			}

#endif		
	
	if (boot_menu || !sys_verify_kernel())
	{
		net_init();
		while (1)
		{
			ui_menu();
		}
	}

	if (astel_serial_RS485 == 0)
	{
		printf("==> enter ^C to abort booting within %d seconds ...... \n", 
				BOARD_BOOT_TIMEOUT);

		delay_ticks = (BOARD_BOOT_TIMEOUT * BOARD_TPS);
		delay_time = sys_get_ticks() + delay_ticks;

		i = 0;
		while (sys_get_ticks() < delay_time)
		{
			unsigned char c;
			if (uart_scanc(&c) && c == BOOT_BREAK_KEY)
			{
				net_init();
				ui_menu();
				break;
			}
			udelay(1000 * 1000);
			i++;
			slideBarMdin380(5+i, 0);
		}
	}


 	// load Kernel
	if (!boot_menu)
	{
#ifdef LOAD_FROM_IDE	
		if (ext2fs_init() == 0)
		{
			boot_menu = 1;
#ifdef RECOVER_FROM_BOOTP			
			//goto RECOVERY;
#endif			
			net_init();
			ui_menu();
		}
#endif
		slideBarMdin380(15, 0);
		sys_load_kernel();
	}

	printf("Reboot system...\n");
	hal_reset();					// dhsul ASTEL
	for (;;);

	///////////////////////////////////////////////////////////

	net_init();
	while (1)
	{
		ui_menu();
	}

#ifdef RECOVER_FROM_BOOTP

RECOVERY:	
	net_init();
	
	use_bootp = 1;
	if (use_bootp)//(use_bootp)
	{
		rc = 1;
		while(rc==1)
		{
			// Initialize the network [if present]
			memset(&my_bootp_info, 0, sizeof(my_bootp_info));
			bootp_get_ip(&my_bootp_info);
			if(my_bootp_info.bp_yiaddr.s_addr!=0)
				rc=0;
		}
		if (rc == 0) {
			ipaddr = (((my_bootp_info.bp_yiaddr.s_addr&0x000000ff)<<24) + ((my_bootp_info.bp_yiaddr.s_addr&0x0000ff00)<<8) + ((my_bootp_info.bp_yiaddr.s_addr&0x00ff0000)>>8) + ((my_bootp_info.bp_yiaddr.s_addr&0xff000000)>>24));
			sys_set_ip_addr(ipaddr);
			use_bootp = 0;
			//sys_set_ip_netmask(netmask);
		}
	}
	printf("serv ip:%d.%d.%d.%d serv mac:%02x:%02x:%02x:%02x:%02x:%02x\n", r.ip_addr[0], r.ip_addr[1], r.ip_addr[2], r.ip_addr[3], r.enet_addr[0], r.enet_addr[1], r.enet_addr[2], r.enet_addr[3], r.enet_addr[4], r.enet_addr[5]);
	printf("my ip:%d.%d.%d.%d\n", IP4(my_bootp_info.bp_yiaddr.s_addr),IP3(my_bootp_info.bp_yiaddr.s_addr),IP2(my_bootp_info.bp_yiaddr.s_addr),IP1(my_bootp_info.bp_yiaddr.s_addr));

	srvip = IPIV(r.ip_addr[0], \
			r.ip_addr[1], \
			r.ip_addr[2], \
			r.ip_addr[3]);
#ifdef BOARD_SUPPORT_RAID
	//--> Send message to windows utility
	s = udp_socket(srvip);
	pbuf = malloc(1400);
	if(!pbuf){
		printf(("No free memory!\n"));
	}
	memset(pbuf,0,1400);
	sprintf(pbuf,"Knox");
	pbuf[4]=0x01;
	pbuf[5]=0x07;
	pbuf[30] = 1;
	sprintf(&pbuf[35],"**Starting Recovery Procedure**");
	udp_sendto(s, (char *)pbuf, 1400, srvip, UDPPORT_FREECOME_MSG, UDPPORT_FREECOME_MSG);
	udp_close(s);
	//<-- send message
#endif
	rc = 1;
	while(rc==1){
		rc = tftp_load_image();
		hal_delay_us(1000*1000); 
	};
	ipaddr = IPIV(IP4(my_bootp_info.bp_yiaddr.s_addr), \
				IP3(my_bootp_info.bp_yiaddr.s_addr), \
				IP2(my_bootp_info.bp_yiaddr.s_addr), \
				IP1(my_bootp_info.bp_yiaddr.s_addr));
#ifdef BOARD_SUPPORT_RAID
	sys_set_vctl_recover(ipaddr,0xFFFFFF00,srvip,1);
#endif	
	sys_run_apps((void *)sys_get_kernel_addr());
#endif
}

/*----------------------------------------------------------------------
* sys_show_hw_cfg
*----------------------------------------------------------------------*/
void sys_show_hw_cfg(void)
{
	UINT32 i, j;
	int is_3316 = FALSE;
	char *status_msg[2]={"disabled", "enabled"};
#ifndef MIDWAY	
    int speed[]={125,116,108,100,91,83,75};
#else
	int speed[]={130,140,150,160,170,180,190,200};
#endif	
    i = (REG32(SL2312_GLOBAL_BASE + GLOBAL_ID) >> 0);
    printf("Processor: CS%x\n",i);
    if (i == GLOBAL_CHIP_ID)
    	is_3316 = TRUE;

	printf("CPU Rate: %d\n", hal_get_cpu_rate());

#if 0			
	printf("MMU: %s, I-CACHE: %s, D-CACHE: %s\n",
			status_msg[hal_get_mmu_status()],
			status_msg[hal_get_icache_status()],
			status_msg[hal_get_dcache_status()]);

	j = hal_detect_pci_clock();
    if (j > 8)
    	printf("Use External PCI Clock \n");
    else
    	printf("Use Internal PCI Clock \n");

	DRIVE_PCI_CLK = (j>8) ? 0 : BIT(5) ;

    i = REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) & BIT(16) ;		// IDE channel 0
    if (i)
    	printf("IDE0 Enable       ");
    else
    	printf("PCI Enable        ");
    	
    if(is_3316) {
    	i = REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) & BIT(17) ;		// IDE channel 1
    	if(i)
	    	printf("IDE1 Enable");
    	else
	    	printf("IDE1 Disable");
    }
    printf("\n");
#endif
    
#ifndef MIDWAY    
	i = REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) & (BIT(0)|BIT(1)|BIT(2)) ;
    printf("AHB Bus Clock:%dMHz    Ratio:",speed[i]);
    j = (REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) & (BIT(4)|BIT(5))) >> 4;   
    switch(j){
    	case 0:
    		printf("1/1\n");
    		break ;
    	case 1:
    		printf("3/2\n");
    		break ;
    	case 2:
    		printf("2/1\n");
    		break ;
    }
    
#else
	i = REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) & (BIT(15)|BIT(16)|BIT(17)) ;
    printf("AHB Bus Clock:%dMHz    Ratio:",speed[(i>>15)]);
	j = (REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) & (BIT(18)|BIT(19))) >> 18; 
	switch(j){
    	case 0:
    		printf("1/1\n");
    		break ;
    	case 1:
    		printf("3/2\n");
    		break ;
    	case 2:
    		printf("24/13\n");
    		break ;
    	case 3:
    		printf("2/1\n");
    		break ;
    }
    
#endif 
    i = REG32(SL2312_GLOBAL_BASE + GLOBAL_STATUS) ;
}

/*----------------------------------------------------------------------
* sys_load_kernel
*----------------------------------------------------------------------*/
void sys_load_kernel(void)
{
	#ifdef LOAD_FROM_IDE
		sys_load_kernel_from_ide();
	#else
		#ifdef BOARD_SUPPORT_FIS
    		fis_load_images();
		#else    
			sys_load_kernel_from_flash();
		#endif
	#endif
}

/*----------------------------------------------------------------------
* sys_run_apps
*----------------------------------------------------------------------*/
void sys_run_apps(void *entry)
{
    void (*apps_routine)(void);
    
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_DCACHE_INVALIDATE_ALL();
    HAL_ICACHE_DISABLE();
    HAL_DCACHE_DISABLE(); 	
#ifdef MIDWAY
	//disable 17
	REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |= BIT(4) | BIT(2) | BIT(1) | BIT(0) | DRIVE_PCI_CLK;		// Disable Parall Flash PAD
	
	REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) |= BIT(2)|BIT(5)|BIT(6)|BIT(24) ;
    
    hal_delay_us(2); 		//delay
    
    REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) &= ~BIT(24) ; // external device reset resume
    
    hal_delay_us(2); 		//delay
    
#else    	
    REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |= BIT(0) | DRIVE_PCI_CLK;		// Disable Parall Flash PAD
	REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) |= BIT(5)|BIT(16) ;
    
    hal_delay_us(2); 		//delay
    
    REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) &= ~BIT(16) ; // external device reset resume
    
    hal_delay_us(2); 		//delay
#endif

#ifdef BOARD_SUPPORT_TWO_CPU
//	printf("Start the 2nd CPU!\n");
//	REG32(GLOBAL_CTRL_CPU1_REG) &= ~CPU1_RESET_BIT_MASK;
//	hal_delay_us(10000); 		//delay
#endif

	apps_routine = (void (*))(entry);
	apps_routine();
}

/*----------------------------------------------------------------------
* sys_load_kernel_from_ide
*----------------------------------------------------------------------*/
#ifdef LOAD_FROM_IDE
void sys_load_kernel_from_ide(void)
{
    unsigned long size, nbytes;
    unsigned long old_ints;
    int	cnt, err;

	// load Kernel 7 initrd

	cnt = 5;
	err = 1;
	while (cnt--)
	{
		initrd_fp = ext2fs_open((char *)sys_get_initrd_name());
		if (!initrd_fp)
			continue;
		size = ext2fs_file_size(initrd_fp);
		printf("Load %s size %d...", (char *)sys_get_initrd_name(), size);
		nbytes = ext2fs_read((void *)initrd_fp, (char *)sys_get_initrd_addr(), size);
		if (nbytes != size)
			printf("Failed!\nExpect read %lu bytes, but got %lu bytes.\n", size, nbytes);
		else
		{
			printf("OK.\n");
			err = 0;
			break;
		}
		ext2fs_close(initrd_fp);
	}
	if (!err)
	{
		cnt = 5;
		err = 1;
		while (cnt--)
		{
			kernel_fp = ext2fs_open((char *)sys_get_kernel_name());
			if (!kernel_fp)
				continue;
			size = ext2fs_file_size(kernel_fp);
			printf("Load %s size %d...", (char *)sys_get_kernel_name(), size);
			nbytes = ext2fs_read((void *)kernel_fp, (char *)sys_get_kernel_addr(), size);
			if (nbytes != size)
				printf("Failed!\nExpect read %lu bytes, but got %lu bytes.\n", size, nbytes);
			else
			{
				printf("OK.\n");
				err = 0;
				break;
			}
			ext2fs_close(kernel_fp);
		}
	}

#ifdef BOARD_SUPPORT_TWO_CPU	
	cnt = 7;
	err = 1;
	while (cnt--)
	{
		initrd_fp = ext2fs_open((char *)sys_get_initrd1_name());
		if (!initrd_fp)
			continue;
		size = ext2fs_file_size(initrd_fp);
		printf("Load %s size %d...", (char *)sys_get_initrd1_name(), size);
		nbytes = ext2fs_read((void *)initrd_fp, (char *)sys_get_initrd1_addr(), size);
		if (nbytes != size)
			printf("Failed!\nExpect read %lu bytes, but got %lu bytes.\n", size, nbytes);
		else
		{
			printf("OK.\n");
			err = 0;
			break;
		}
		ext2fs_close(initrd_fp);
	}
	if (!err)
	{
		cnt = 7;
		err = 1;
		while (cnt--)
		{
			kernel_fp = ext2fs_open((char *)sys_get_kernel1_name());
			if (!kernel_fp)
				continue;
			size = ext2fs_file_size(kernel_fp);
			printf("Load %s size %d...", (char *)sys_get_kernel1_name(), size);
			nbytes = ext2fs_read((void *)kernel_fp, (char *)sys_get_kernel1_addr(), size);
			if (nbytes != size)
				printf("Failed!\nExpect read %lu bytes, but got %lu bytes.\n", size, nbytes);
			else
			{
				printf("OK.\n");
				err = 0;
				break;
			}
			ext2fs_close(kernel_fp);
		}
	}
#endif
	
	ext2fs_close(kernel_fp);
	ext2fs_close(initrd_fp);
	
	if(!err)
		sys_run_apps((void *)sys_get_kernel_addr());
	
}
#endif //  LOAD_FROM_IDE

/*----------------------------------------------------------------------
* sys_load_kernel_from_flash
*----------------------------------------------------------------------*/
#ifndef LOAD_FROM_IDE
void sys_load_kernel_from_flash(void)
{
	extern int checkAstelFlashCrcData(int);
	extern int whereIsFlashImage(int);
    void (*apps_routine)(void);
    unsigned long i, j, *dest, *srce, size;
    unsigned long old_ints;
#ifdef BOARD_SUPPORT_TWO_CPU
	unsigned char *p ;
	unsigned char *tgt0 ;
#endif    	
	unsigned int    value;
	int ret;

	hal_interrupt_mask_all();
    
    HAL_DISABLE_INTERRUPTS(old_ints);
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_DCACHE_INVALIDATE_ALL();
    HAL_ICACHE_ENABLE();
    HAL_DCACHE_ENABLE();
    
	// load Kernel
#ifdef BOARD_NAND_BOOT		
	printf("Load kernel from 0x%x to 0x%x size %d\n",
			BOARD_FLASH_KERNEL_ADDR, BOARD_DRAM_KERNEL_ADDR, BOARD_KERNELIMG_SIZE);
#else	
	printf("Load kernel from 0x%x to 0x%x size %d\n",
			BOARD_FLASH_KERNEL_ADDR, BOARD_DRAM_KERNEL_ADDR, BOARD_KERNEL_SIZE);
#endif	
	hal_flash_enable();

	ret = checkAstelFlashCrcData(FLASH_TYPE_KERNEL);

//printf(">>>>>>>>>> testSis kernel Load ret=%d \n", ret);
	if(ret == 0)
	  {
		ret = whereIsFlashImage(FLASH_TYPE_KERNEL);
		if(ret == 0)
			srce=(unsigned long *)(BOARD_FLASH_KERNEL_ADDR);
		else
			srce = (unsigned long *)ret;
	  }
	else
		srce=(unsigned long *)(BOARD_FLASH_KERNEL_ADDR);

    dest=(unsigned long *)BOARD_DRAM_KERNEL_ADDR;
#ifdef BOARD_NAND_BOOT		
	size = BOARD_KERNELIMG_SIZE / sizeof(unsigned long);
#else	
    size = BOARD_KERNEL_SIZE / sizeof(unsigned long);
#endif

//printf("ret=%x srce=%x \n", ret, srce);

	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;	
	if((value&0x1000)==0x1000)
#ifdef BOARD_NAND_BOOT		
		sys_memcpy(dest, srce, BOARD_KERNELIMG_SIZE );
#else		
		sys_memcpy(dest, srce, BOARD_KERNEL_SIZE );
#endif
	else
	{
    for (i = 0; i < size; i++)
    	*dest++=*srce++;
	}
	hal_flash_disable();
	
#if (BOARD_RAM_DISK_SIZE != 0)
	// load RAM Disk
#ifdef BOARD_NAND_BOOT	
	printf("Load RAM DISK from 0x%x to 0x%x size %d\n",
			BOARD_FLASH_RAM_DISK_ADDR, BOARD_DRAM_RAM_DISK_ADDR, BOARD_RAM_DISKIMG_SIZE);
#else
	printf("Load RAM DISK from 0x%x to 0x%x size %d\n",
			BOARD_FLASH_RAM_DISK_ADDR, BOARD_DRAM_RAM_DISK_ADDR, BOARD_RAM_DISK_SIZE);
#endif
	
	hal_flash_enable();

	ret = checkAstelFlashCrcData(FLASH_TYPE_RAMDISK);
	if(ret == 0)
	  {
		ret = whereIsFlashImage(FLASH_TYPE_RAMDISK);
		if(ret == 0)
			srce=(unsigned long *)(BOARD_FLASH_RAM_DISK_ADDR);
		else
			srce = (unsigned long *)ret;
	  }
	else
		srce=(unsigned long *)(BOARD_FLASH_RAM_DISK_ADDR);

    dest=(unsigned long *)BOARD_DRAM_RAM_DISK_ADDR;
#ifdef BOARD_NAND_BOOT	
	size = BOARD_RAM_DISKIMG_SIZE / sizeof(unsigned long);
#else
    size = BOARD_RAM_DISK_SIZE / sizeof(unsigned long);
#endif

	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;	
	if((value&0x1000)==0x1000)
#ifdef BOARD_NAND_BOOT			
		sys_memcpy(dest, srce, BOARD_RAM_DISKIMG_SIZE );
#else
		sys_memcpy(dest, srce, BOARD_RAM_DISK_SIZE );
#endif
	else
	{
    	for (i = 0; i < size; i++)
    		*dest++=*srce++;
	}
	hal_flash_disable();
#endif

#ifdef BOARD_SUPPORT_TWO_CPU
	// load CPU-1 image
	printf("Load CPU-2 code\n");

	FIS_T *img;
	int k;
	unsigned int cpu2_base=BOARD_DRAM_CPU2_ADDR;

	for (i=0; i<FIS_MAX_ENTRY; i++, img++)
	{
		if (img->file.name[0] != 0xff && img->file.name[0] != 0x00 &&
			img->desc_cksum == fis_desc_chksum(img) &&
			img->file.flash_base && img->file.mem_base)
		{
			if (strncasecmp(img->file.name, BOARD_FIS_CPU2_NAME) == 0)
			{
				cpu2_base = img->file.entry_point - CPU2_BOOT_OFFSET;
			}
		}
	}
	
	hal_flash_enable();   
	
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
	srce=(unsigned long *)(BOARD_FLASH_CPU2_ADDR);
	//dest=(unsigned long *)BOARD_DRAM_CPU2_ADDR;
	dest=(unsigned long *)cpu2_base;
#ifdef BOARD_NAND_BOOT	
	size = (BOARD_CPU2IMG_SIZE / sizeof(unsigned long))/2;
#else
	size = (BOARD_CPU2_SIZE / sizeof(unsigned long))/2;
#endif

	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;	
	if((value&0x1000)==0x1000)
#ifdef BOARD_NAND_BOOT			
		sys_memcpy(dest, srce, BOARD_CPU2IMG_SIZE );
#else
		sys_memcpy(dest, srce, BOARD_CPU2_SIZE );
#endif
	else
	{
    	for (i = 0; i < size; i++)
    		*dest++=*srce++;
	}
	hal_flash_disable();
	
	HAL_ICACHE_INVALIDATE_ALL();
	HAL_DCACHE_INVALIDATE_ALL();
	
	//REG32(0x66000040) = CPU2_RAM_BASE + CPU2_RAM_SIZE;
	if(cpu2_base > 0x4000000)
		REG32(0x66000040) = cpu2_base + 32;
	else
		REG32(0x66000040) = cpu2_base + 64;
	
#endif

	sys_run_apps((void *)BOARD_DRAM_KERNEL_START_ADDR);

}
#endif //  LOAD_FROM_IDE

/*----------------------------------------------------------------------
* sys_verify_kernel
*----------------------------------------------------------------------*/
int sys_verify_kernel(void)
{
	#ifdef LOAD_FROM_IDE
	#else
		return sys_verify_kernel_in_flash();
	#endif
}

/*----------------------------------------------------------------------
* sys_verify_kernel_in_flash
*----------------------------------------------------------------------*/
#ifndef LOAD_FROM_IDE
int sys_verify_kernel_in_flash(void)
{
	unsigned int    value;
	char *datap;
	unsigned int kaddr=0;
	
#ifdef BOARD_SUPPORT_FIS
	FIS_T	*img;

	img = fis_find_image(BOARD_FIS_KERNEL_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_KERNEL_NAME, img);
		return;
	}
	kaddr = img->file.flash_base;
	
#else
	kaddr = BOARD_FLASH_KERNEL_ADDR;
#endif

	hal_flash_enable();
	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;	
	if((value&0x1000)==0x1000)
	{
		//char datap[528];
		datap = (char *)malloc(528);
		if (!datap)
		{
			printf(("No free memory!\n"));
			return;
		}
		sys_memcpy(datap, kaddr, 0x200 );
	}	
	else
	{
		//char *datap;
		datap = (char *)kaddr;
	}

	if (*(UINT32 *)datap == 0xffffffff)
	{
		hal_flash_disable();
		printf("Illegal kernel image\n");
		if((value&0x1000)==0x1000)
			free(datap);
		return FALSE;
	}
	if((value&0x1000)==0x1000)
		free(datap);
	hal_flash_disable();
	return TRUE;
}
#endif

