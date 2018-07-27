/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ui_upgrade.c
* Description	: 
*		Handle upgrade function for UI
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/21/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <Xmodem/Xmodem.h>
#include <sys_fis.h>

#define UPGRADE_RETRY_TIME		3
//#define UPGRADE_COMPARE		1

// const char * const upgrade_type_msg[]={"BOOT", "KERNEL", "FULL", "RAM Disk", "Application"};
char tftp_ip_str[20], tftp_filename[128];

int web_upgrade(int type, int got_size, unsigned char *filep);
extern int ui_gets(char *buf, int size);
extern char *flash_errmsg(int err);
extern unsigned char *fis_datap;
extern void fis_write_flash(void);
extern unsigned long fis_desc_chksum(FIS_T *img);

#ifdef MIDWAY
#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
extern void toe_gmac_disable_tx_rx(void);
extern void toe_gmac_enable_tx_rx(void);
#else
extern void gmac_disable_tx_rx(void);
extern void gmac_enable_tx_rx(void);
#endif
extern void emac_enable_tx_rx(void);
extern void emac_disable_tx_rx(void);
#endif
int ui_download(char *destp, int erase_size, int max_size);

#define IMGHDR_SIZE			32
#define IMGHDR_NAME_SIZE	16
#define IMGHDR_NAME			"CS-BOOT-001"
typedef struct {
	unsigned char 	name[IMGHDR_NAME_SIZE];
	unsigned long	file_size;
	unsigned char	reserved[IMGHDR_SIZE-IMGHDR_NAME_SIZE-4-2];
	unsigned short	checksum;
} IMGHDR_T;

extern unsigned short sys_crc16(unsigned short crc, unsigned char *datap, unsigned long len);

/*----------------------------------------------------------------------
* ui_disable_mac
*----------------------------------------------------------------------*/
static void ui_disable_mac(void)
{
#ifdef MIDWAY	
#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
	toe_gmac_disable_tx_rx();
#else
	gmac_disable_tx_rx();
#endif	
#else
	emac_disable_tx_rx();
#endif	
}

/*----------------------------------------------------------------------
* ui_enable_mac
*----------------------------------------------------------------------*/
static void ui_enable_mac(void)
{
#ifdef MIDWAY	
#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
	toe_gmac_enable_tx_rx();
#else
	gmac_enable_tx_rx();
#endif	
#else
	emac_enable_tx_rx();
#endif	
}

/*----------------------------------------------------------------------
* ui_upgrade_boot
*	return	0: if quit
*			1: X-modem
*			2: TFTP
*----------------------------------------------------------------------*/
static int ui_get_upgrade_method(void)
{
	UINT8 key;
	
	printf("\n\n");
	printf("1  : Download by X-modem\n");
	printf("2  : Download by TFTP\n");
	printf("ESC: Return \n");
	printf("==> ");
	do
	{
		key = ui_getc();
		if (key == '1')
		{
			printf("1\n");
			return 1;
		}
		else if (key == '2')
		{
			printf("2\n");
			return 2;
		}
		else if (key == 0x1b)
		{
			printf("Quit!\n");
			return 0;
		}
	} while (1);
}

/*----------------------------------------------------------------------
* do_xmodem_download
*  return 
*		TRUE if OK, FALSE if failed
*----------------------------------------------------------------------*/
int do_xmodem_download(char *bufp, int buf_size, int *size)
{
	int err;
	
	ui_disable_mac();
	printf("Start the file transfer of Terminal...\n");
	if (Xmodem_open(&err) != 0)
	{
		Xmodem_terminate(1);
		Xmodem_close(&err);
		printf("\nError to open X-modem! (%d) %s\n", err, Xmodem_get_error_msg(err));
		ui_enable_mac();
		return FALSE;
	}
	*size = Xmodem_read(bufp, buf_size, &err);
	
	if (err < 0 && err != XMODEM_ERR_EOF)
	{
		Xmodem_terminate(1);
		Xmodem_close(&err);
		printf("\nFaile to upgrade! (%d) %s!\n", err, Xmodem_get_error_msg(err));
		ui_enable_mac();
		return FALSE;
	}
	Xmodem_terminate(0);
	Xmodem_close(&err);
	
	printf("\n\nSuccessful to download! Size=%d", *size);
	
	if(*size < 100)
	{
		printf("No Data Received!\n");	
		ui_enable_mac();
		return FALSE;
	}
	printf("\n");
	ui_enable_mac();
	return TRUE;
}


/*----------------------------------------------------------------------
* do_tftp_download
*  return 
*		TRUE if OK, FALSE if failed
*----------------------------------------------------------------------*/
int do_tftp_download(char *bufp, int buf_size, int *size)
{
	int rc;
	UINT32 ipaddr;
	
	printf("TFTP Server IP Address: ");
	rc = ui_gets(tftp_ip_str, sizeof(tftp_ip_str));
	ipaddr = str2ip(tftp_ip_str);
	if (rc == 0 || ipaddr == 0 || ipaddr == 0xffffffff)
	{
		printf("Illegal IP address!\n");
		return FALSE;
	}	
	printf("Image Path and name(e.g. /image_path/image_name): ");
	rc = ui_gets(tftp_filename, sizeof(tftp_filename));
	if (rc == 0)
	{
		printf("Illegal image name!\n");
		return FALSE;
	}	
	
	printf("TFTP Download %s from %d.%d.%d.%d ...",
			tftp_filename, IP1(ipaddr), IP2(ipaddr), IP3(ipaddr), IP4(ipaddr));
			
	if ((rc = tftpc_get(tftp_filename, ipaddr, bufp, buf_size, size)) != 0)
	{
		printf("\n\nFailed for TFTP! (%d) %s\n", rc, tftp_err_msg(rc));
    	return FALSE;
	}
	
	printf("\n\nSuccessful to download by TFTP! Size=%d", *size);
	
	printf("\n");
	
	return TRUE;
}

/*----------------------------------------------------------------------
* ui_upgrade_boot
*	int type: 
*		0: Boot
*		1: Kernel
*		2: Full
*		3: RAM Disk
*		4: Application
*----------------------------------------------------------------------*/
void ui_upgrade(int type)
{
    char	*destp;
    int		erase_size;
    int 	max_size;
	unsigned int addr=0, size=0;
	FIS_T	*img;
	
	switch (type)
	{
		case 0:	// Boot
			destp = (char *)BOARD_FLASH_BOOT_ADDR;
			#ifdef BOARD_NAND_BOOT	
				max_size = BOARD_FLASH_BOOT_SIZE;
				erase_size = BOARD_FLASH_BOOTIMG_SIZE;
			#else
				max_size = erase_size = BOARD_FLASH_BOOT_SIZE;
			#endif
			break;
		case 1:	// Kernel
#ifndef LOAD_FROM_IDE		
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_KERNEL_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_KERNEL_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_KERNEL_ADDR;
	size = BOARD_KERNEL_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
			max_size = BOARD_KERNEL_SIZE;
			erase_size = BOARD_KERNELIMG_SIZE;
	#else
			max_size = erase_size = size;
	#endif
#endif			
			break;
		case 2:	// Full Image
#ifndef LOAD_FROM_IDE		
			destp = (char *)BOARD_FLASH_BASE_ADDR;
			max_size = erase_size = BOARD_FLASH_SIZE;
#endif			
			break;
		case 3:	// RAM Disk
#ifndef LOAD_FROM_IDE		
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_RAM_DISK_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_RAM_DISK_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_RAM_DISK_ADDR;
	size = BOARD_RAM_DISK_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
			max_size = BOARD_RAM_DISK_SIZE;
			erase_size = BOARD_RAM_DISKIMG_SIZE;
	#else
			max_size = erase_size = size;
	#endif
#endif			
			break;
		case 4:	// Application
#ifndef LOAD_FROM_IDE	



#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_APPS_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_APPS_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_APPS_ADDR;
	size = BOARD_APPS_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_APPS_SIZE;
		erase_size = BOARD_APPSIMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
		
#endif 
			break;
#ifdef BOARD_SUPPORT_TWO_CPU			
		case 5:	// Kernel #1
#ifndef LOAD_FROM_IDE		
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_CPU2_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_CPU2_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_CPU2_ADDR;
	size = BOARD_CPU2_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_CPU2_SIZE;
		erase_size =BOARD_CPU2IMG_SIZE ;
	#else
		max_size = erase_size = size;
	#endif
#endif			
			break;
		case 6:	// RAM Disk #1
#ifndef LOAD_FROM_IDE	
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_RAM_DISK2_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_RAM_DISK2_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_CPU2_RD_ADDR;
	size = BOARD_CPU2_RD_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_CPU2_RD_SIZE;
		erase_size = BOARD_CPU2_RDIMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
#endif			
			break;	
#endif			
		default:
			printf("Unknown upgrade type!\n");
			return ;
	}
	
	ui_download(destp, erase_size, max_size);
}	

/*----------------------------------------------------------------------
* verify_image_file
*	return 	0: checksum OK
*			1: Incorrect header
*			-1; Checksum error
*----------------------------------------------------------------------*/
static int verify_image_file(char *datap)
{
	IMGHDR_T 		*img;
	unsigned short	crc16;
	
	img = (IMGHDR_T *)datap;
	if (strcmp(img->name, IMGHDR_NAME) != 0)
		return 1;
		
	datap += IMGHDR_SIZE;
	crc16 = 0xffff;
	crc16 = sys_crc16(crc16, datap, img->file_size);
	crc16 ^= 0xffff;
	
	return (img->checksum == crc16) ? 0 : -1;
}

/*----------------------------------------------------------------------
* ui_download
*----------------------------------------------------------------------*/
int ui_download(char *destp, int erase_size, int max_size)
{
	int				i, j, stat, tmp=0;
    unsigned long	err_addr;
	int				buf_size;
	int				got_size = 0;
	int				err;
	int				method;
	char			*bufp, *filep, *rbuf;
	FIS_T *img;
	UINT64	delay_time;
	UINT32	delay_ticks;
	
	if (erase_size)
		buf_size = erase_size + 1024;
	else
		buf_size = (BOARD_FLASH_SIZE);
	filep = bufp = (char *)malloc(buf_size);
	if (!bufp)
	{
		printf("No free memory to upgrade (0x%x)!\n", destp);
		return 0;
	}
#ifdef UPGRADE_COMPARE
	rbuf = (char *)malloc(buf_size);
	if (!rbuf)
	{
		printf("No free memory to compare (0x%x)!\n", destp);
		return 0;
	}
#endif	

	method = ui_get_upgrade_method();
	switch (method)
	{
		case 1: // Upgrade by X-modem
			if (!do_xmodem_download(bufp, buf_size, &got_size))
			{
				free(bufp);
				return 0;
			}
			break;
		case 2: // Upgrade by TFTP
			if (!do_tftp_download(bufp, buf_size, &got_size))
			{
				free(bufp);
				return 0;
			}
			break;
		default:
			printf("Unknown!\n");
			free(bufp);
			return 0;
	}
	
	if (max_size)
	{
		if (got_size > max_size)
		{
			printf("File is too large!\n");
			free(bufp);
			return 0;
		}
	}
	ui_disable_mac();
	if ((unsigned long)destp == BOARD_FLASH_BOOT_ADDR)
	{
		int rc = verify_image_file(filep);
		if (rc == 0)
		{
			filep += IMGHDR_SIZE;
			got_size -= IMGHDR_SIZE;
		}
		else if (rc < 0)
		{
			printf("\nChecksum error!!!\n");
			free(bufp);
			ui_enable_mac();
			return 0;
		} 
		else // if (rc == 1) 
		{
			printf("\nUnknown image header!!\n");
			printf("Are you sure to continue (Y/N) ? ");
			if (!ui_get_confirm_key())
			{
				printf("\nAborted by user!\n");	
				free(bufp);
				ui_enable_mac();
				return 0;
			}
		}
	}		
	
	printf("\nDo not power-off this device while flash programming is proceeding!!\n");
	//printf("Are you sure to program flash 0x%x (Y/N) ? ", destp);
	//if (!ui_get_confirm_key())
	//{
	//	printf("\nAborted by user!\n");	
	//	free(bufp);
	//	ui_enable_mac();
	//	return 0;
	//}
	
	printf("==> enter ^C to abort program flash 0x%x within %d seconds ...... \n", 
			destp, BOARD_BOOT_TIMEOUT);

	delay_ticks = (BOARD_BOOT_TIMEOUT * BOARD_TPS);
	delay_time = sys_get_ticks() + delay_ticks;
	
	while (sys_get_ticks() < delay_time)
	{
		unsigned char c;
		if (uart_scanc(&c) && c == BOOT_BREAK_KEY)
		{
				printf("\nAborted by user!\n");	
				free(bufp);
				ui_enable_mac();
				return 0;
		}
	}
		
	printf("\n");
	
	if (erase_size == 0)
		erase_size = got_size;
	for (i=0, stat=1; i<UPGRADE_RETRY_TIME && stat!=0; i++)
	{
#ifdef BOARD_NAND_BOOT		
	if (erase_size < got_size)
	{
		printf("file to large(Got size : 0x%x  Max Image size: 0x%x) !!\n",got_size,erase_size); 
		continue;
	}

	#ifdef BOARD_SUPPORT_YAFFS2
		if ((stat = flash_fs_nand_program((void *)(destp), (void *)filep, erase_size,  (unsigned long *)&err_addr, max_size)) != 0)
		{
			printf(" FAILED at 0x%x: ", err_addr);
			printf((char *)flash_errmsg(stat));
			printf("\n"); 
			continue;
		}
		
	
	#else 
		//printf("\nProgram flash (0x%x): Size=%u ", BOARD_FLASH_FIS_ADDR, fis_size);
		if ((stat = flash_nand_program((void *)(destp), (void *)filep, erase_size,  (unsigned long *)&err_addr, max_size)) != 0)
		{
			printf(" FAILED at 0x%x: ", err_addr);
			printf((char *)flash_errmsg(stat));
			printf("\n"); 
			continue;
		}
		
	#endif
		else
		{
			printf(" OK!\n"); 
			continue;
		}
#else			
		printf("Erase flash (0x%x): Size=%u ", destp, erase_size); 
    	if ((stat = flash_erase((void *)(destp), 
    							erase_size,
    							(unsigned long *)&err_addr)) != 0)
    	{
			printf(" FAILED at 0x%x: ", err_addr);
			printf((char *)flash_errmsg(stat));
			printf("\n"); 
    		continue;
    	}
		printf(" OK!\n"); 
		printf("Program flash (0x%x): Size=%u ", destp, got_size); 
    	if ((stat = flash_program((void *)(destp), 
    							  (void *)filep, 
    							  got_size, 
    							  (unsigned long *)&err_addr)) != 0)
    	{
			printf(" FAILED at 0x%x: ", err_addr);
			printf((char *)flash_errmsg(stat));
			printf("\n"); 
			continue;
    	}
    	else
			printf(" OK!\n"); 
#endif //BOARD_NAND_BOOT
#ifdef UPGRADE_COMPARE
		printf("Compare (0x%x): Size=%u ", destp, got_size); 
			hal_flash_enable();
    		memcpy(rbuf, destp, max_size);
    		hal_flash_disable();
    		for(j=0;j<got_size;j++)
    		{
    			if((j%0x10000)==0)
    				printf(".");
    				
    			if(*(rbuf+j)!=*(filep+j))
    			{
    				tmp = 1;
    				break;
    			}
    		}
    		if(tmp)
    			printf("Compare error at 0x%x:",j);
    		else
				printf(" OK!\n"); 
			free(rbuf);
#endif		
	}

#ifndef LOAD_FROM_IDE	
	//if ((got_size > 0)&&((got_size!=BOARD_FLASH_SIZE)||(got_size!=0x800000)))
	//	if ((got_size > 0)&&(got_size!=BOARD_FLASH_SIZE))
	if (((got_size > 0)&&(got_size!=BOARD_FLASH_SIZE)) && ((unsigned long)destp != BOARD_FLASH_FIS_ADDR))	
	{
		//memset((char *)img, 0, FIS_ENTRY_SIZE);
		img = (FIS_T *)fis_datap;
		
		for (j=0; j<FIS_MAX_ENTRY; j++, img++)
		{
			if (img->file.flash_base == destp)
			{
#ifdef BOARD_NAND_BOOT					
				img->file.data_length	= erase_size;
#else
				img->file.data_length	= got_size; //max_size;//got_size;
#endif				
				img->file.size			= max_size; //got_size;
				img->desc_cksum			= fis_desc_chksum(img);
							
			}
		}
		fis_write_flash();
	}
#endif		

    if (stat!=0)
    {
    	printf("Failed to upgrade (0x%x)!\n", destp);
		free(bufp);
		ui_enable_mac();
    	return 0;
    }
    else
    {
    	printf("Successful to upgrade (0x%x)!\n", destp);
		free(bufp);
		ui_enable_mac();
    	return got_size;
    }
}

int web_upgrade(int type, int got_size, unsigned char *filep)
{
    unsigned char	*destp;
    int		erase_size;
    int 	max_size;
	unsigned int addr=0, size=0;	
	FIS_T	*img;
	
	switch (type)
	{
		case 0:	// Boot
			destp = (unsigned char *)BOARD_FLASH_BOOT_ADDR;
			
			#ifdef BOARD_NAND_BOOT	
				max_size = BOARD_FLASH_BOOT_SIZE;
				erase_size = BOARD_FLASH_BOOTIMG_SIZE;
			#else
				max_size = erase_size = BOARD_FLASH_BOOT_SIZE;
			#endif
			break;
		case 1:	// Kernel
#ifndef LOAD_FROM_IDE		
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_KERNEL_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_KERNEL_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_KERNEL_ADDR;
	size = BOARD_KERNEL_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_KERNEL_SIZE;
		erase_size = BOARD_KERNELIMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
#endif		
			break;
		case 2:	// Full Image
#ifndef LOAD_FROM_IDE		
			destp = (unsigned char *)BOARD_FLASH_BASE_ADDR;
			max_size = erase_size = BOARD_FLASH_SIZE;
#endif			
			break;
		case 3:	// RAM Disk
#ifndef LOAD_FROM_IDE		
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_RAM_DISK_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_RAM_DISK_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_RAM_DISK_ADDR;
	size = BOARD_RAM_DISK_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_RAM_DISK_SIZE;
		erase_size = BOARD_RAM_DISKIMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
#endif			
			break;
		case 4:	// Application
#ifndef LOAD_FROM_IDE	
#ifndef BOARD_NAND_BOOT	 //app
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_APPS_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_APPS_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_APPS_ADDR;
	size = BOARD_APPS_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_APPS_SIZE;
		erase_size = BOARD_APPSIMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
#endif //app	
#endif				
			break;
#ifdef BOARD_SUPPORT_TWO_CPU			
		case 5:	// Kernel #1
#ifndef LOAD_FROM_IDE		
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_CPU2_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_CPU2_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_CPU2_ADDR;
	size = BOARD_CPU2_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_CPU2_SIZE;
		erase_size = BOARD_CPU2IMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
#endif	
			break;
		case 6:	// RAM Disk #1
#ifndef LOAD_FROM_IDE	
#ifdef BOARD_SUPPORT_FIS

	img = fis_find_image(BOARD_FIS_RAM_DISK2_NAME);
	if (!img)
	{
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_RAM_DISK2_NAME, img);
		return;
	}
	addr = img->file.flash_base;
	size = img->file.size;
	
#else
	addr = BOARD_FLASH_CPU2_RD_ADDR;
	size = BOARD_CPU2_RD_SIZE;
#endif
			destp = (char *)addr;
	#ifdef BOARD_NAND_BOOT	
		max_size = BOARD_CPU2_RD_SIZE;
		erase_size = BOARD_CPU2_RDIMG_SIZE;
	#else
		max_size = erase_size = size;
	#endif
#endif			
			break;	
#endif			
		default:
			printf("Unknown upgrade type!\n");
			return 0;
	}
	
	return web_program(destp, erase_size, max_size, got_size, filep);
}	

/*----------------------------------------------------------------------
* ui_download
*-----web_program-----------------------------------------------------------------*/
int web_program(unsigned char *destp, int erase_size, int max_size, int got_size, unsigned char *filep)
{
	int				i, j, stat, tmp=0;
    unsigned long	err_addr;
	int				buf_size;
	int				err;
	int				method;
	unsigned char			*bufp;
	FIS_T *img;
	UINT64	delay_time;
	UINT32	delay_ticks;
	
	ui_disable_mac();
	if ((unsigned long)destp == BOARD_FLASH_BOOT_ADDR)
	{
		bufp = filep;
		int rc = verify_image_file(bufp);
		if (rc == 0)
		{
			bufp += IMGHDR_SIZE;
			got_size -= IMGHDR_SIZE;
		}
		else if (rc < 0)
		{
			printf("\nChecksum error!!!\n");
			ui_enable_mac();
			return 0;
		} 
		else // if (rc == 1) 
		{
			printf("\nUnknown image header!!\n");
			//printf("Are you sure to continue (Y/N) ? ");
			//if (!ui_get_confirm_key())
			//{
			//	printf("\nAborted by user!\n");	
			//	ui_enable_mac();
			//	return 0;
			//}
		}
	}		
	
	printf("\nDo not power-off this device while flash programming is proceeding!!\n");
	if (max_size)
	{
		if (got_size > max_size)
		{
			printf("File is too large!\n");
			free(bufp);
			return 0;
		}
	}
	
	if (erase_size == 0)
		erase_size = got_size;
	for (i=0, stat=1; i<UPGRADE_RETRY_TIME && stat!=0; i++)
	{
#ifdef BOARD_NAND_BOOT		
	if (erase_size < got_size)
	{
		printf("file to large(Got size : 0x%x  Max Image size: 0x%x) !!\n",got_size,erase_size); 
		return;
	}
		
	//printf("\nProgram flash (0x%x): Size=%u ", BOARD_FLASH_FIS_ADDR, fis_size);
	if ((stat = flash_nand_program((void *)(destp), (void *)filep, erase_size,  (unsigned long *)&err_addr, max_size)) != 0)
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
		printf("Erase flash (0x%x): Size=%x ", destp, erase_size); 
    	if ((stat = flash_erase((void *)(destp), erase_size, (unsigned long *)(&err_addr))) != 0)
    	{
				printf(" FAILED at 0x%x: ", err_addr);
				printf((char *)flash_errmsg(stat));
				printf("\n"); 
    		continue;
    	}
		printf(" OK!\n"); 
		//hal_delay_us(1000); 
		printf("Program flash (0x%x): Size=%x ", destp, got_size); 
		//hal_delay_us(2000); 
    	if ((stat = flash_program((void *)(destp), (void *)(filep), got_size, (unsigned long *)(&err_addr))) != 0)
    	{
				printf(" FAILED at 0x%x: ", err_addr);
				printf((char *)flash_errmsg(stat));
				printf("\n"); 
			continue;
    	}
    	else
			printf(" OK!\n"); 
#endif		
	
	}

#ifndef LOAD_FROM_IDE	
	//if ((got_size > 0)&&((got_size!=BOARD_FLASH_SIZE)||(got_size!=0x800000)))
	if ((got_size > 0)&&(got_size!=BOARD_FLASH_SIZE))
	{
		//memset((char *)img, 0, FIS_ENTRY_SIZE);
		img = (FIS_T *)fis_datap;
		
		for (j=0; j<FIS_MAX_ENTRY; j++, img++)
		{
			if (img->file.flash_base == destp)
			{
				img->file.data_length	= max_size;//got_size;
				img->file.size			= got_size;
				img->desc_cksum			= fis_desc_chksum(img);
							
			}
		}
		fis_write_flash();
	}
#endif		

    if (stat!=0)
    {
    	printf("Failed to upgrade (0x%x)!\n", destp);
		ui_enable_mac();
    	return 0;
    }
    else
    {
    	printf("Successful to upgrade (0x%x)!\n", destp);
		ui_enable_mac();
    	return got_size;
    }
}

