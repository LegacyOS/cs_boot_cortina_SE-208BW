/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ide.c
* Description	: 
*		Handle IDE functions
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/07/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#ifdef BOARD_SUPPORT_IDE
#include "ide.h"

#ifdef BOARD_SUPPORT_IDE	// backward compatiable
#if !defined(BOARD_SUPPORT_IDE0) && !defined(BOARD_SUPPORT_IDE1)
	#define BOARD_SUPPORT_IDE0	0
	#define BOARD_SUPPORT_IDE1	1
#elif !(defined(BOARD_SUPPORT_IDE0) && defined(BOARD_SUPPORT_IDE1))
	#error "Must define BOARD_SUPPORT_IDE0 and BOARD_SUPPORT_IDE1 in board_config.h"
#endif
#endif	// BOARD_SUPPORT_IDE

#define IDE_DETECT_TIME		(20)//(180)		// 180 seconds

IDE_INFO_T	ide_info[IDE_CONTROLLERS];
IDE_PART_T	ide_partitions[IDE_MAX_PARTITIONS];
int			ide_part_num;

static int ide_detect(IDE_INFO_T *ide);
static int ide_reset(IDE_INFO_T *ide);
static int ide_init_disks(IDE_INFO_T *ide);
static int ide_ident(IDE_INFO_T *ide, int dev, UINT16 *buf);
static int ide_find_partitions(IDE_DISK_T *disk);
static int ide_disk_read_sectors(IDE_DISK_T *disk, UINT64 start, UINT32 count, UINT16 *buf);
static int ide_disk_write_sectors(IDE_DISK_T *disk, UINT64 start, UINT32 bytes, UINT16 *buf);
#ifdef USB_DEV_MO
int ide_disk_read_dma(IDE_DISK_T *disk, UINT64 start, UINT32 byte_count, UINT8 *buf);
int ide_disk_write_dma(IDE_DISK_T *disk, UINT64 start, UINT byte_count, UINT8 *buf);
int ide_partition_read_dma(IDE_PART_T *part, UINT64 start, UINT32 n_byte, UINT8 *buf);
int ide_partition_write_dma(IDE_PART_T *part, UINT64 start, UINT32 n_byte, UINT8 *buf);
#endif
static int ide_wait_busy(IDE_INFO_T *ide, UINT32 us);

extern void hal_delay_us(UINT32 us);
extern int uart_scanc(unsigned char *c);

int ide_initialized = 0;

extern int ide_present;

#ifdef SATA_LED
unsigned sata_timer;
#endif

static unsigned char PIO_TIMING[5] = { 0xaa, 0xa3, 0xa1, 0x33, 0x31 };
static unsigned char TIMING_MDMA_50M[3] = { 0x66, 0x22, 0x21 };
static unsigned char TIMING_MDMA_66M[3] = { 0x88, 0x32, 0x31 };
static unsigned char TIMING_UDMA_50M[6] = { 0x33, 0x31, 0x21, 0x21, 0x11, 0x91 };
static unsigned char TIMING_UDMA_66M[7] = { 0x44, 0x42, 0x31, 0x21, 0x11, 0x91,  0x91};

//--> for usb device mode code
#ifdef USB_DEV_MO
#define MAX_PRD		16
#define PRD_SIZE	8		
#define ADD_PRD_ENTRY(prd, phy, cnt, eot) \
		*(unsigned int *) prd = (unsigned int) phy;		\
		*((unsigned int *) prd + 1) = (unsigned int) cnt | (eot << 31);
		
void ide_build_prd(unsigned int *base, unsigned char *buff, unsigned int length);
int set_xfer_mode(IDE_DISK_T *disk,u8 *dev_id);
void ide_interrupt(u32 irq_status);
void ide_interrupt2(u32 irq_status);
IDE_DISK_T	*first_disk=NULL;
IDE_DISK_T	*second_disk=NULL;
extern u64 dev_sectors;
extern u8 dev_id[512];
//unsigned int prd_pool[MAX_PRD*PRD_SIZE/sizeof(int)+1];
unsigned int *prd_base=0;
unsigned int *prd_base2=0;

unsigned int	async_io=0;

#endif
//<-- for usb device mode code

/*----------------------------------------------------------------------
* ide_init
*----------------------------------------------------------------------*/
int ide_init(void)
{
	IDE_INFO_T		*ide;
	int			i=0, total, wait_cnt, rc;
	UINT64			ticks;
	unsigned char 	c;
	
	if (ide_initialized) 
		return;
#ifdef SATA_LED
REG32(SL2312_GPIO_BASE + 0x08) |= BIT(GPIO_SATA0_LED); //Wilson
REG32(SL2312_GPIO_BASE + 0x00) |= BIT(GPIO_SATA0_LED); //Wilson
#endif


#ifndef BOARD_SUPPORT_RAID
	//REG32(SL2312_SATA_BASE + 0x18) = 0x3;  //mode 2
	REG32(SL2312_SATA_BASE + 0x1c) = 0x3;  //mode 3
	REG32(SL2312_SATA_BASE + 0x18) |= BIT(4);  //mode 3
#else
	REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &= 0xF8FFFFFF;	// io-mux mode 0
	REG32(SL2312_SATA_BASE + 0x18) = 0x11;  //enable INT, master mode
	REG32(SL2312_SATA_BASE + 0x1C) = 0x11;  //enable INT, master mode
#endif
	printf("Waiting for disk ready & detect ...\n");
	
	ticks = sys_get_ticks();
	sys_sleep(BOARD_TPS * 2);
	
	ide_part_num = 0;
	total = wait_cnt = 0;
	
	memset(&ide_partitions,0,sizeof(IDE_PART_T)*IDE_MAX_PARTITIONS);
	
	while (total == 0 && (wait_cnt++ < IDE_DETECT_TIME))	// 20 seconds
	{
#ifdef BOARD_SUPPORT_RAID
		if(i==IDE_CONTROLLERS)	// no disk attached !
			break;
#endif
			
		if ((sys_get_ticks() - ticks) > (IDE_DETECT_TIME * BOARD_TPS))
		{
			printf("Timeout!\n");
			return 0;
		}
		
		hal_delay_us(1000000);
		if (uart_scanc(&c) && c == BOOT_BREAK_KEY)
		{
			printf("Aborted by user!\n");
			return 0;
		}
		for (i=0; i<IDE_CONTROLLERS; i++, ide++)
		{
			ide = (IDE_INFO_T *)&ide_info[i];
			memset((void *)ide, 0, sizeof(IDE_INFO_T));
			ide->ide_id = i;
			
			
			if (i==0)
			{
#ifdef BOARD_SUPPORT_RAID
				if((REG32(SL2312_SATA_BASE + 0x08)&0x01)==0)
					continue;
#endif
#if (BOARD_SUPPORT_IDE0==0)
				continue;
#endif				
				ide->bm_base = (unsigned int)SL2312_IDE0_BASE;
				ide->cmd_base = (unsigned int)SL2312_IDE0_BASE + 0x20;
				ide->ctrl_base = (unsigned int)SL2312_IDE0_BASE + 0x36;
			}
			else
			{
#ifdef BOARD_SUPPORT_RAID
				if((REG32(SL2312_SATA_BASE + 0x0C)&0x01)==0)
					continue;
#endif
#if (BOARD_SUPPORT_IDE1==0)
				continue;
#endif				
				ide->bm_base = (unsigned int)SL2312_IDE1_BASE;
				ide->cmd_base = (unsigned int)SL2312_IDE1_BASE + 0x20;
				ide->ctrl_base = (unsigned int)SL2312_IDE1_BASE + 0x36;
			}
	    /*	
			// detect attached IDE drive
			rc = ide_detect(ide);
			if ( rc == 0)
			{
				//printf("No IDE drive is found!");
				continue;
			}
			else if (rc == -1)
			{
				printf("Aborted by user!\n");
				return 0;
			}
			*/
			// Reset IDE controller
			rc = ide_reset(ide);
			if (rc == -1)
			{
				printf("Aborted by user!\n");
				return 0;
			}
			//else if (!rc)
			//{
			//	// printf("Failed to reset IDE controller!\n");
			//	continue;
			//}
			
			if (ide_init_disks(ide) == 0)
			{
				// printf("Failed to init IDE disks!\n");
				continue;
			}
			ide->present = 1;
			total++;
			ide_present++;
#ifndef BOARD_SUPPORT_RAID			
			break;
#endif
		}
	}
	
	if (!total)
		printf("No IDE drive is found!");
		
	ide_initialized = total;
	
	return total;
}

/*----------------------------------------------------------------------
* ide_detect
*	return total active drive
*----------------------------------------------------------------------*/
static int ide_detect(IDE_INFO_T *ide)
{
	UINT8	sel, val ,status , signlow, signhigh;
	int		i, total, x;
	static  int retry=0;
	unsigned char c;

	total = 0;
	for (x=0; x<(IDE_DETECT_TIME * 1); x++)
	{
    	for (i = 0; i <IDE_MAX_DISKS; i++)
    	{
    		if (ide->disk[i].present) continue;
			sel = (i << 4) | 0xA0;
			hal_delay_us(100000);
			IDE_CMD_REG8(ide->cmd_base, IDE_REG_DEVICE) = 0;
			IDE_CMD_REG8(ide->cmd_base, IDE_REG_DEVICE) = sel;
			hal_delay_us(100000);
			val =0;
			val = IDE_CMD_REG8(ide->cmd_base, IDE_REG_DEVICE);
			ide->disk[i].present = 0;
			//printf("val: %x  sel: %x\n",val,sel);
			status = IDE_CMD_REG8(ide->cmd_base, 0x16);	
			if ((status == 0xff))		
				continue; //return DEVICE_TYPE_NONE;	
			IDE_CMD_REG8(ide->cmd_base, IDE_REG_COUNT) = 0xa5;	
			val = IDE_CMD_REG8(ide->cmd_base, IDE_REG_COUNT);
			if (val != 0xa5)		
				continue; //return DEVICE_TYPE_NONE;	
			signlow = IDE_CMD_REG8(ide->cmd_base, IDE_REG_LBAMID);	
			signhigh = IDE_CMD_REG8(ide->cmd_base, IDE_REG_LBAHI);	
			if ((signlow == 0x14) && (signhigh == 0xeb))		
				continue; //return DEVICE_TYPE_CDROM;	
			else	
			{
				//return DEVICE_TYPE_HDD;
				ide->disk[i].present = 1;
	    			printf("IDE %d Detect disk %d\n", ide->ide_id, i);
	    			total++;
			}
				
			/*
			//if ((val&0xa0) == 0xA0)//sel)
			//{
				
	    		if (i)
	    		{
					IDE_CMD_REG8(ide->cmd_base, IDE_REG_DEVICE) = 0;
				}
	    		
			}
			*/
			if (uart_scanc(&c) && c == BOOT_BREAK_KEY)
			{
				return -1;
			}
		}
		if (ide->disk[1].present)
			break;
    }
    retry = 1;
    return total;
}
/*----------------------------------------------------------------------
* ide_reset
*----------------------------------------------------------------------*/
static int ide_reset(IDE_INFO_T *ide)
{
	UINT8	status;
	int		i;

    IDE_CTRL_REG8(ide->ctrl_base) |= 0x4;	// reset asserted
    hal_delay_us(5000);
    IDE_CTRL_REG8(ide->ctrl_base) &= ~0x4;	// reset cleared
    hal_delay_us(50000);

    // wait 10 seconds 
    for (i = 0; i < 100; ++i)
    {
		unsigned char c;
		hal_delay_us(100000);
		status = IDE_CMD_REG8(ide->cmd_base, IDE_REG_STATUS);
		//if (!(status & IDE_STAT_BSY) && (status & IDE_STAT_DRDY))
		if (!(status & IDE_STAT_BSY) )
			return 1;
		if (uart_scanc(&c) && c == BOOT_BREAK_KEY)
		{
			// printf("Aborted by user!\n");
			return -1;
		}
    }
	//if (!(status & IDE_STAT_BSY))
	//	return 0;

    return 0;
}

/*----------------------------------------------------------------------
* ide_init_disks
*----------------------------------------------------------------------*/
static int ide_init_disks(IDE_INFO_T *ide)
{
	int 		i, total;
	IDE_DISK_T	*disk;
    UINT32		buf[IDE_SECTOR_SIZE/sizeof(UINT32)];
    UINT32		total_bytes;
    UINT16		data16;
	int  		rc;
	int 		found,irq_no;
	rc = ide_detect(ide);
	irq_no = ide->ide_id+4;
	total = 0;

		disk = (IDE_DISK_T *)&ide->disk[0];
		for (i = 0; i < IDE_MAX_DISKS; i++, disk++)
		{
			found = 0;
		    disk->drive_id = i;
		    disk->flags = 0;
		    disk->ide = ide;
		    disk->part_num = 0;
		    disk->read = (void *)ide_disk_read_sectors;
		    disk->write = (void *)ide_disk_write_sectors;
#ifdef USB_DEV_MO
			disk->read_dma = (void *)ide_disk_read_dma;
			disk->write_dma = (void *)ide_disk_write_dma;
#endif

		    if (!disk->present)
		    	continue;

		    if (ide_ident(ide, i, (UINT16 *)buf) <= 0)
		    {
			    disk->flags = 0;
			    disk->present = 0;
			    continue;  // can't identify device
		    }
						
		    	total ++;
			disk->kind = DISK_IDE_HD;  // until proven otherwise
			disk->flags |= IDE_DEV_PRESENT;
			data16 = REG16((char *)buf + IDE_CMD_SET);
			disk->lba_48 = (data16 >> 10) & 1;
			if (disk->lba_48)
				disk->sector_num = (UINT64)(REG16((char *)buf + IDE_LBA_48)) +
								   ((UINT64)(REG16((char *)buf + IDE_LBA_48 + 2)) << 16) +
								   ((UINT64)(REG16((char *)buf + IDE_LBA_48 + 4)) << 32) +
								   ((UINT64)(REG16((char *)buf + IDE_LBA_48 + 6)) << 48);
			else
				disk->sector_num = (UINT64)(REG32((char *)buf + IDE_DEVID_LBA_CAPACITY));
								   
			// total_bytes = disk->sector_num * IDE_SECTOR_SIZE / 1024 / 1024;
			total_bytes = disk->sector_num / (1000000 / IDE_SECTOR_SIZE);
#if 0		
			printf("Disk Drive: IDE-%d, Device-%d, %lld Sectors IDE_CMD_SET = 0x%04X ", 
					ide->ide_id, disk->drive_id, disk->sector_num, data16);
#else
			printf("Disk Drive: IDE-%d, Device-%d, %lld Sectors ", 
					ide->ide_id, disk->drive_id, disk->sector_num);
#endif				

#ifdef USB_DEV_MO
			if(!first_disk)	{	// get first disk
				first_disk = disk;
				hal_register_irq_entry((void *)ide_interrupt,irq_no);
				memcpy(dev_id,buf,IDE_SECTOR_SIZE);
			}
			else{
				second_disk = disk;
				hal_register_irq_entry((void *)ide_interrupt2,irq_no);
			}

			dev_sectors = disk->sector_num;
			set_xfer_mode(disk,buf);
			hal_interrupt_configure(irq_no,1,1);
			//hal_register_irq_entry((void *)ide_interrupt,irq_no);
			hal_interrupt_unmask(irq_no);
			disk->raid_idx = -1;
#endif			
			if (total_bytes > 1000)
			{
				UINT32 Gb, Mb;
				Gb = total_bytes / 1000;
				Mb = total_bytes % 1000;
				printf("%d GB %d MB\n", Gb, Mb);
			}
			else
			{
				printf("%d MB\n", total_bytes);
			}
			
			found = ide_find_partitions(disk);
#ifndef BOARD_SUPPORT_RAID
			if(found)
				break;
#endif
		}

	
	ide->disk_num = total;
	return total;
}

/*----------------------------------------------------------------------
* ide_wait_drq
*----------------------------------------------------------------------*/
static int ide_wait_drq(IDE_INFO_T *ide, UINT32 us)
{
    UINT8	status;
    UINT32	tries;

    for (tries=0; tries<(us/10); tries++)
    {
		status = IDE_CMD_REG8(ide->cmd_base, IDE_REG_STATUS);
        if (!(status & IDE_STAT_BSY))
        {
            if (status & IDE_STAT_DRQ)
                return 1;
        }
    	hal_delay_us(10);
    }
    
    printf("disk(%x) : DRQ timeout!\n",ide->ide_id);
    return 0;
}

/*----------------------------------------------------------------------
* ide_wait_busy
*----------------------------------------------------------------------*/
static int ide_wait_busy(IDE_INFO_T *ide, UINT32 us)
{
    UINT8	status;
    UINT32	tries;
    
    for (tries=0; tries < (us/10); tries++)
    {
         hal_delay_us(10);
         status = IDE_CMD_REG8(ide->cmd_base, IDE_REG_STATUS);
         if ((status & IDE_STAT_BSY) == 0)
              return 1;
    }
    return 0;   
}

/*----------------------------------------------------------------------
* ide_wait_busy
*----------------------------------------------------------------------*/
static int ide_wait_interrupt(IDE_INFO_T *ide, UINT32 us)
{
    UINT8	status;
    UINT32	tries;
    
    for (tries=0; tries < (us/10); tries++)
    {
	hal_delay_us(10);
        if(ide->irq_expect)
		return 0;
    }
    return 1;   
}

/*----------------------------------------------------------------------
* ide_ident
*----------------------------------------------------------------------*/
static int ide_ident(IDE_INFO_T *ide, int dev, UINT16 *buf)
{
    int i;
	UINT32 *data;

    IDE_CMD_REG8(ide->cmd_base, IDE_REG_DEVICE) =  dev << 4;
    IDE_CMD_REG8(ide->cmd_base, IDE_REG_COMMAND) = 0xEC;
    hal_delay_us(50000);

    if (!ide_wait_drq(ide, 5000000))
    {
		//printf("%s: NO DRQ for IDE-%d\n",
        //                __FUNCTION__, ide->ide_id);
		return 0;
	}

	data = (UINT32 *)buf;
    for (i = 0; i < (IDE_SECTOR_SIZE / sizeof(*data)); i++, data++)
		*data = IDE_CMD_REG32(ide->cmd_base, IDE_REG_DATA);

    return 1;
}

/*----------------------------------------------------------------------
* ide_find_partitions
*----------------------------------------------------------------------*/
static int ide_find_partitions(IDE_DISK_T *disk)
{
    UINT16		buf[IDE_SECTOR_SIZE / sizeof(UINT16)];
    UINT16		magic;
    IDE_PART_T	*part;
    int 		i,j, found = 0;
    UINT32		total_bytes;
    IDE_MBR_T	*mbr;
    UINT32		s, n;
    
/*
	//Perform r/w test
	UINT8 *buf2,*buf3,pat='0';
	buf2 = malloc(32*1024*2);
	buf3 = malloc(32*1024*2);
	for(i=0;i<disk->sector_num;i+=64){
		memset(buf2,pat++,32*1024*2);
		(*disk->write_dma)(disk, i+100, 32*1024*2, buf2) ;
		(*disk->read_dma)(disk, i+100, 32*1024*2, buf3);	
		if(memcmp(buf2,buf3,32*1024*2))
			printf("Data mismatch\n");
	}
*/

    // read Master Boot Record
//    if ((*disk->read_dma)(disk, 1, IDE_SECTOR_SIZE, buf) <= 0)
//		return 0;
#ifdef USB_DEV_MO
		(*disk->read_dma)(disk, 1, IDE_SECTOR_SIZE, buf);
#else
		(*disk->read)(disk, 1, 1, buf);
#endif
	j=1;
#if 0
	printf("\n");
	printf("*******IDE sector 1 Data **********\n");
	for (i=0;i<256;i++)
	{
	if (i==(8*j))
	 {
	  printf("\n");
	  j++;
      }
	printf("%04x ",buf[i]);
    }
#endif
#ifdef USB_DEV_MO
		if ((*disk->read_dma)(disk, 0, IDE_SECTOR_SIZE, buf) <= 0)
		return 0;
#else		
		if ((*disk->read)(disk, 0, 1, buf) <= 0)
		return 0;		
#endif		
    // Read MBR
    magic = ide_get_uint16((char *)buf + MBR_MAGIC_OFFSET);
    if (magic != MBR_MAGIC)
    {
		printf("Unknown partition type!\n");
		return 0;
    }
    		
    mbr = (IDE_MBR_T *)((char *)buf + MBR_PTABLE_OFFSET);
    for (i = 0; i < IDE_MAX_PARTS_PER_DISK && ide_part_num<IDE_MAX_PARTITIONS; i++, mbr++)
    {
    	if (mbr->sys_ind == 0)
    		continue;
		
		printf("Partition %d: ", i+1);
		switch (mbr->sys_ind)
		{
			case IDE_PART_LINUX_MINIX:		printf("Linux/MINIX");	break;
			case IDE_PART_LINUX_SWAP:		printf("Linux Swap");	break;
			case IDE_PART_LINUX:			printf("Linux");		break;
			case IDE_PART_LINUX_EXTENDED: 	printf("Linux Extended");	break;
			case 0x01:						printf("FAT12");		break;
			case 0x04:						printf("FAT16 <32M");	break;
			case 0x05:						printf("Extended");		break;
			case 0x06:						printf("FAT16");		break;
			case 0x07:						printf("HPFS/NTFS");	break;
			case 0x0b:						printf("FAT32");	break;
			case 0x0c:						printf("FAT32");	break;
			case 0x0e:						printf("FAT16");	break;
			case 0x0f:						printf("Win95 Ext'd");	break;
			case IDE_PART_LINUX_RAID: 	printf("Linux RAID");	break;
			default:						printf("(0x%02X)", mbr->sys_ind); break;
		}
    	if (mbr->sys_ind != IDE_PART_LINUX)
	    {
	    	printf("\n");
#ifndef BOARD_SUPPORT_RAID
	    	continue;
#endif
	    }
		
		s = ide_get_uint32((char *)(&mbr->start_sect));
		n = ide_get_uint32((char *)(&mbr->sector_num));
		if (s==0 || n ==0)
			continue;
		
		part = (IDE_PART_T *)&ide_partitions[ide_part_num];
		part->present 		= 1;
		part->part_id		= i;
		part->disk			= disk;
		part->start_sector	= s;
		part->sector_num	= n;
		part->os			= mbr->sys_ind;
		part->bootflag		= mbr->boot_ind;
#ifdef USB_DEV_MO
		part->read			= (void *)ide_partition_read_dma;
		part->write			= (void *)ide_partition_write_dma;
#else
		part->read			= (void *)ide_disk_read_sectors;
		part->write			= (void *)ide_disk_write_sectors;
#endif
		ide_part_num++;
		disk->part_num++;
		found++;
		
	    printf(" %lld Sectors ", part->sector_num);
		// total_bytes = part->sector_num * IDE_SECTOR_SIZE / 1024 / 1024;
		total_bytes = (UINT64)part->sector_num / (1000000 / IDE_SECTOR_SIZE);
		if (total_bytes > 1000)
		{
			int Gb, Mb;
			Gb = total_bytes / 1000;
			Mb = total_bytes % 1000;
			printf("%d GB %d MB\n", Gb, Mb);
		}
		else
		{
			printf("%d MB\n", total_bytes);
		}
	}
    return found;
}

/*----------------------------------------------------------------------
* ide_disk_read_sectors
*----------------------------------------------------------------------*/
static int ide_disk_read_sectors(IDE_DISK_T *disk, UINT64 start, UINT32 count, UINT16 *buf)
{
    int				i, j;
    //UINT32			*p;
    UINT16			*p;
    unsigned int	cmd_base;

	if(!ide_wait_busy(disk->ide, 2000000))
	{
		printf("IDE %d is BUSY!\n", disk->ide->ide_id);
		return 0;
	}
    
    //for sector count not byte count
    //count = count>>9 ;
	
	cmd_base = disk->ide->cmd_base;
    if (!disk->lba_48)
    {
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= count;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	=  start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= ((start >> 24) & 0xf) | (disk->drive_id << 4) | 0x40;
		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= 0x20;
	}
	else
	{
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= (count >> 8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= count & 0xff;
		
		// high 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= (start >> 24) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >> 32) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 40) & 0xff;
		
		// low 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= (disk->drive_id << 4) | 
											  	   0x40;

		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= 0x24;
	}
	
	//p = (UINT32 *)buf;
	p = buf;
    for(i = 0; i < count; i++)
    {
        if (!ide_wait_drq(disk->ide, 5000000))
        {
            printf("%s: NO DRQ for ide%d, device %d LBA %u\n",
                        __FUNCTION__, disk->ide->ide_id, disk->drive_id, start);
            return 0;
        }

        for (j = 0; j < (IDE_SECTOR_SIZE / sizeof(*p)); j++, p++)
            *p = IDE_CMD_REG16(cmd_base, IDE_REG_DATA);
            //*p = IDE_CMD_REG32(cmd_base, IDE_REG_DATA);
    }
    return 1;
}

/*----------------------------------------------------------------------
* ide_disk_read_sectors
*----------------------------------------------------------------------*/
static int ide_disk_write_sectors(IDE_DISK_T *disk, UINT64 start, UINT32 bytes, UINT16 *buf)
{
    int				i, len;
    UINT32			data;
    UINT32			*cp;
    unsigned int	cmd_base;

	if(!ide_wait_busy(disk->ide, 2000000))
	{
		printf("IDE %d is BUSY!\n", disk->ide->ide_id);
		return 0;
	}
	
	cmd_base = disk->ide->cmd_base;
    
    if (!disk->lba_48)
    {
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= 1; 
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= ((start >> 24) & 0xf) | 
											  	   (disk->drive_id << 4) | 
											  	   0x40;
		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= 0x30;
	}
	else
	{
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= 0; 	// high 8 bits
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= 1; 	// low 8 bits
		
		// high 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= (start >> 24) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >> 32) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 40) & 0xff;
		
		// low 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= (disk->drive_id << 4) | 
											  	   0x40;
		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= 0x34;
	}
		

    if (!ide_wait_drq(disk->ide, 5000000))
    {
		printf("%s: NO DRQ for ide%d, device %d.\n",
				__FUNCTION__, disk->ide->ide_id, disk->drive_id);
		return 0;
	}
    
    cp = (UINT32 *)buf; 
    for (i = 0, len=0 ; i < (IDE_SECTOR_SIZE / sizeof(*cp)); i++)
    {
    	if (len < bytes)
    	{
    		IDE_CMD_REG32(cmd_base, IDE_REG_DATA) = *cp;
    		cp++;
    	}
    	else
    		IDE_CMD_REG32(cmd_base, IDE_REG_DATA) = 0;
		len += sizeof(UINT32);
    }
    return 1;
}

#ifdef USB_DEV_MO
void print_read_error_reg(char *prefix, unsigned char error)
{
	printf("%s", prefix);
	if (error & 0x80)
		printf("ICRC:");
	if (error & 0x40)
		printf("UNC:");
	if (error & 0x20)
		printf("MC:");
	if (error & 0x10)
		printf("IDNF:");
	if (error & 0x08)
		printf("MCR:");
	if (error & 0x04)
		printf("ABRT:");
	if (error & 0x02)
		printf("NM:");
	printf("\n");
}

void print_write_error_reg(char *prefix, unsigned char error)
{
	printf("%s", prefix);
	if (error & 0x80)
		printf("ICRC:");
	if (error & 0x40)
		printf("WP:");
	if (error & 0x20)
		printf("MC:");
	if (error & 0x10)
		printf("IDNF:");
	if (error & 0x08)
		printf("MCR:");
	if (error & 0x04)
		printf("ABRT:");
	if (error & 0x02)
		printf("NM:");
	printf("\n");
}

void ide_interrupt(u32 irq_status)
{
	u32 status;
	first_disk->ide->irq_expect = 1;
	status = IDE_CMD_REG8(first_disk->ide->cmd_base, IDE_REG_STATUS);
	
#ifdef USB_SPEED_UP	
	if (status & IDE_STAT_ERR) {
		printf("ERROR: IDE[Read] STATUS REG error bit is 1\n");
		print_read_error_reg("ERROR Reg:", IDE_CMD_REG8(first_disk->ide->cmd_base, IDE_REG_ERROR));
	}
#endif
	return ;
}

void ide_interrupt2(u32 irq_status)
{
	u32 status;
	second_disk->ide->irq_expect = 1;
	status = IDE_CMD_REG8(second_disk->ide->cmd_base, IDE_REG_STATUS);
	
#ifdef USB_SPEED_UP	
	if (status & IDE_STAT_ERR) {
		printf("ERROR: IDE[Read] STATUS REG error bit is 1\n");
		print_read_error_reg("ERROR Reg:", IDE_CMD_REG8(second_disk->ide->cmd_base, IDE_REG_ERROR));
	}
#endif
	return ;
}

/*----------------------------------------------------------------------
* ide_disk_read_dma
*----------------------------------------------------------------------*/
int ide_disk_read_dma(IDE_DISK_T *disk, UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	int		i, j;
	unsigned int	cmd_base,bm_base;
	UINT8		ide_status;
	UINT16 count;
	

	if(n_byte&0x1ff)
		n_byte = (n_byte+512)&(~0x1ff);
		
	if(!ide_wait_busy(disk->ide, 2000000))
	{
		printf("IDE %d is BUSY to read!\n", disk->ide->ide_id);
		return 0;
	}

#ifdef USB_SPEED_UP	
	volatile unsigned int *pbuf = buf; 	
	for(i=0;i<(n_byte/SW_PIPE_SIZE);i++)
		pbuf[(i*SW_PIPE_SIZE+SW_PIPE_SIZE)/4-1] = CHECK_PATTERN1;
	if(((n_byte/SW_PIPE_SIZE)>=1)&&(n_byte&(SW_PIPE_SIZE-1)))
		pbuf[(i*SW_PIPE_SIZE+(n_byte&(SW_PIPE_SIZE-1)))/4-1] = CHECK_PATTERN1;
#endif
    
	bm_base = disk->ide->bm_base;
	cmd_base = disk->ide->cmd_base;
	
	count = n_byte>>9 ;
	
	if (!disk->lba_48)
	{
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= count;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	=  start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= ((start >> 24) & 0xf) | (disk->drive_id << 4) | 0x40;
		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= WIN_READDMA;
	}
	else
	{
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= (count >> 8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= count & 0xff;
		
		// high 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= (start >> 24) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >> 32) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 40) & 0xff;
		
		// low 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= (disk->drive_id << 4) | 0x40;

		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= WIN_READDMA_EXT;
	}
	IDE_BM_COMMAND(bm_base) |=(1<<3); // ATA read
	// setup PRD
	disk->ide->irq_expect = 0;
	if(disk->ide->ide_id==0){
		ide_build_prd(prd_base,buf,n_byte);
		IDE_BM_PRD_BASE(bm_base)= prd_base;
	}
	else{
		ide_build_prd(prd_base2,buf,n_byte);
		IDE_BM_PRD_BASE(bm_base)= prd_base2;
	}
	IDE_BM_STATUS(bm_base) |= 0x26;	// DMA capable
	
	IDE_BM_COMMAND(bm_base) |= 0x01;  // start DMA

	if(async_io==1)
		return 1;
#ifdef SATA_LED
	sata_timer = sys_get_ticks();
	if(sata_timer&0x04)
		REG32(SL2312_GPIO_BASE + 0x10)=BIT(GPIO_SATA0_LED); // off
#endif

#ifdef USB_SPEED_UP
	if(n_byte >= SW_PIPE_SIZE){
		hal_delay_us(50);
		return 1;
	}
	else{
		if(ide_wait_interrupt(disk->ide,1000000)){
			printf("IDE[Read] INT lost\n ");
			return 0;
		}
			
		ide_status = IDE_CMD_REG8(cmd_base, IDE_REG_STATUS);
		
		if (ide_status & IDE_STAT_ERR) {
			printf("ERROR: IDE%d[Read] STATUS REG error bit is 1\n",disk->ide->ide_id);
			print_read_error_reg("ERROR Reg:", IDE_CMD_REG8(cmd_base, IDE_REG_ERROR));
			return 0;
		}
	}
#else

	if(ide_wait_interrupt(disk->ide,2000000)){
		printf("IDE[Read] INT lost\n ");
		return 0;
	}
		
	ide_status = IDE_CMD_REG8(cmd_base, IDE_REG_STATUS);
	
	if (ide_status & IDE_STAT_ERR) {
		printf("ERROR: IDE%d[Read] STATUS REG error bit is 1\n",disk->ide->ide_id);
		print_read_error_reg("ERROR Reg:", IDE_CMD_REG8(cmd_base, IDE_REG_ERROR));
		return 0;
	}
	
#endif	

    return 1;
}

/*----------------------------------------------------------------------
* ide_partition_read_dma
*----------------------------------------------------------------------*/
int ide_partition_read_dma(IDE_PART_T *part, UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	if(start > part->sector_num)
		printf("partition read: over boundary(limit:%d [%d])\n",part->sector_num,start);
	return ide_disk_read_dma(part->disk, start+part->start_sector, n_byte, buf);
}

/*----------------------------------------------------------------------
* ide_disk_write_dma
*----------------------------------------------------------------------*/
int ide_disk_write_dma(IDE_DISK_T *disk, UINT64 start, UINT32 n_byte, UINT8 *buf)
{
	int				i, len;
	unsigned int	cmd_base,bm_base;
	UINT8		ide_status;    
	UINT16 count;

	if(n_byte&0x1ff)
		n_byte = (n_byte+512)&(~0x1ff);
		
	if(!ide_wait_busy(disk->ide, 2000000))
	{
		printf("IDE %d is BUSY to write!\n", disk->ide->ide_id);
		return 0;
	}
	
	bm_base = disk->ide->bm_base;
	cmd_base = disk->ide->cmd_base;
	
	count = n_byte>>9;
    	
	if (!disk->lba_48)
	{
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= count; 
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= ((start >> 24) & 0xf) | 
							  (disk->drive_id << 4) | 0x40;
		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= WIN_WRITEDMA;
	}
	else
	{
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= (count>>8)&0xFF; 	// high 8 bits
		IDE_CMD_REG8(cmd_base, IDE_REG_COUNT)	= count&0xff; 	// low 8 bits
		
		// high 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= (start >> 24) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= 0;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= 0;
		
		// low 24 bits of LBA
		IDE_CMD_REG8(cmd_base, IDE_REG_LBALOW)	= start & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAMID)	= (start >>  8) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_LBAHI)	= (start >> 16) & 0xff;
		IDE_CMD_REG8(cmd_base, IDE_REG_DEVICE)	= (disk->drive_id << 4) | 0x40;
		IDE_CMD_REG8(cmd_base, IDE_REG_COMMAND)	= WIN_WRITEDMA_EXT;
	}

	IDE_BM_COMMAND(bm_base) = 0;	// ATA write
	// setup PRD 
	disk->ide->irq_expect = 0;
	if(disk->ide->ide_id==0){
		ide_build_prd(prd_base,buf,n_byte);
		IDE_BM_PRD_BASE(bm_base)= prd_base;
	}
	else{
		ide_build_prd(prd_base2,buf,n_byte);
		IDE_BM_PRD_BASE(bm_base)= prd_base2;
	}
	IDE_BM_STATUS(bm_base) |= 0x26;	// DMA capable	
	
	IDE_BM_COMMAND(bm_base) |= 0x01;

	if(async_io==1)
		return 1;
#ifdef SATA_LED
	sata_timer = sys_get_ticks();
	if(sata_timer&0x04)
		REG32(SL2312_GPIO_BASE + 0x10)=BIT(GPIO_SATA0_LED); // off
#endif


	if(ide_wait_interrupt(disk->ide,2000000)){
		printf("IDE[W] INT lost\n ");
		return 0;
	}
	
	ide_status = IDE_CMD_REG8(cmd_base, IDE_REG_STATUS);
	
	if (ide_status & IDE_STAT_ERR) {
		printf("ERROR: IDE%d[Write] STATUS REG error bit is 1\n",disk->ide->ide_id);
		print_write_error_reg("ERROR Reg:", IDE_CMD_REG8(cmd_base, IDE_REG_ERROR));
		return 0;
	}
	
    return 1;
}

int ide_partition_write_dma(IDE_PART_T *part, UINT64 start, UINT32 n_byte, UINT8 *buf)
{
#ifdef BOARD_SUPPORT_RAID
	if((start+(n_byte>>9)) > (part->rdev_size<<1+1)){
		printf("partition write: over boundary(limit:[%d] [%d] %d)\n",part->rdev_size<<1,start,n_byte);
		//return 0;
	}
#else
	if((start+(n_byte>>9) > part->sector_num)){
		printf("partition write: over boundary(limit:%d [%d])\n",part->sector_num,start);
		return 0;
	}
#endif
	return ide_disk_write_dma(part->disk, start+part->start_sector, n_byte, buf);
}


/*
* set_xfer_mode() Set chip xfer timing and config HD xfer mode
* disk   - disk argument
* dev_id - HD identify data
*/
int set_xfer_mode(IDE_DISK_T *disk,u8 *dev_id)
{
	u8 speed,device,status;
	unsigned int bm_base,i;
	
	bm_base = disk->ide->bm_base;
	device = disk->drive_id;
	// tune chip
	printf("[UDMA");
	speed = (UINT8)(REG16((char *)dev_id + ATA_ID_UDMA_MODES)) &0x7f;
	if(speed&ATA_UDMA6){
		speed = XFER_UDMA_6;
		printf("6] ");
	}
	else if(speed&ATA_UDMA5){
		speed = XFER_UDMA_5;
		printf("5] ");
	}
	else if(speed&ATA_UDMA4){
		speed = XFER_UDMA_4;
		printf("4] ");
	}
	else if(speed&ATA_UDMA3){
		speed = XFER_UDMA_3;
		printf("3] ");
	}
	else if(speed&ATA_UDMA2){
		speed = XFER_UDMA_2;
		printf("2] ");
	}
	else if(speed&ATA_UDMA1){
		speed = XFER_UDMA_1;
		printf("1] ");
	}
	else if(speed&ATA_UDMA0){
		speed = XFER_UDMA_0;
		printf("0] ");
	}
	
	if (speed & XFER_UDMA_0) // ultra dma
		CLK_MOD_REG(bm_base) |= (1 << (4 + device));
	else if (speed & XFER_MW_DMA_0)
		CLK_MOD_REG(bm_base) &= ~(1 << (4 + device));
	
	if(speed & XFER_PIO_0)
		PIO_TIMING_REG(bm_base) = PIO_TIMING[3];
	
	
	else
	{
		// for better performance
		if ((speed == XFER_UDMA_6) || (speed == XFER_UDMA_3) || (speed == XFER_UDMA_4) || (speed & XFER_MW_DMA_0)) {
			CLK_MOD_REG(bm_base) |= (1 << device);
			if (speed & XFER_MW_DMA_0)
				MDMA_TIMING_REG(bm_base) = TIMING_MDMA_66M[speed & ~XFER_MW_DMA_0];
	
			else {
				if (!device)
					UDMA_TIMING0_REG(bm_base) = TIMING_UDMA_66M[speed & ~XFER_UDMA_0];
				else
					UDMA_TIMING1_REG(bm_base) = TIMING_UDMA_66M[speed & ~XFER_UDMA_0];			
			}
		}
		else {
			CLK_MOD_REG(bm_base) &= ~(1 << device);
			if (speed & XFER_MW_DMA_0)
				MDMA_TIMING_REG(bm_base) = TIMING_MDMA_50M[speed & ~XFER_MW_DMA_0];
			else {
				if (!device)
				UDMA_TIMING0_REG(bm_base) = TIMING_UDMA_50M[speed & ~XFER_UDMA_0];				
				else
				UDMA_TIMING1_REG(bm_base) = TIMING_UDMA_50M[speed & ~XFER_UDMA_0];
			}
		}
	}	
	
	// set device mode
	if(!ide_wait_busy(disk->ide, 2000000))
	{
		printf("ERROR:IDE %d is BUSY!\n", disk->ide->ide_id);
		return -1;
	}
	IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_DEVICE) = (device << 4);
	
	//expecting_int(channel);
	IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_COUNT)	= speed;
	IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_FEATURES)= 0x03;
	IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_COMMAND)	= WIN_SETFEATURES;
	
	if(!ide_wait_busy(disk->ide, 1000000))
	{
		printf("ERROR:IDE %d is BUSY after set xfer mode!\n", disk->ide->ide_id);
		return -1;
	}
	
	status = IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_STATUS);
	if(status&0x01){		// ERROR
		printf("ERROR: 0x%x",status);
		return -1;
	}
	
	// initial prd table
	if(!prd_base){
		prd_base = MALLOC_DMA(MAX_PRD*PRD_SIZE);
		if(!prd_base)
			printf("PRD1 alloc fail\n");
		memset(prd_base,0,MAX_PRD*PRD_SIZE);
	}
	if(!prd_base2){
		prd_base2 = MALLOC_DMA(MAX_PRD*PRD_SIZE);
		if(!prd_base2)
			printf("PRD2 alloc fail\n");
		memset(prd_base2,0,MAX_PRD*PRD_SIZE);
	}
	
	disk->write_buff = (UINT8)(REG16((char *)dev_id + ATA_ID_COMMAND_SET0)) & BIT(5);
	disk->look_ahead = (UINT8)(REG16((char *)dev_id + ATA_ID_COMMAND_SET0)) & BIT(6);
/*	
	// write cache gain nothing, disable it and we don't worry about flush issue
	if(disk->write_buff){
		// enable write-cache
		if(!ide_wait_busy(disk->ide, 1000000))
		{
			printf("ERROR:IDE %d is BUSY!\n", disk->ide->ide_id);
			return -1;
		}
		IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_DEVICE) = (device << 4);
		
		IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_FEATURES)= 0x02;
		IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_COMMAND)	= WIN_SETFEATURES;
		
		if(!ide_wait_busy(disk->ide, 1000000))
		{
			printf("ERROR:IDE %d is BUSY after enable write buffer!\n", disk->ide->ide_id);
			return -1;
		}
		
		status = IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_STATUS);
		if(status&0x01){		// ERROR
			printf("ERROR: 0x%x",status);
			return -1;
		}
	}
	if(disk->look_ahead){
		// enable look-ahead
		if(!ide_wait_busy(disk->ide, 1000000))
		{
			printf("ERROR:IDE %d is BUSY!\n", disk->ide->ide_id);
			return -1;
		}
		IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_DEVICE) = (device << 4);
		
		IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_FEATURES)= 0xAA;
		IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_COMMAND)	= WIN_SETFEATURES;
		
		if(!ide_wait_busy(disk->ide, 1000000))
		{
			printf("ERROR:IDE %d is BUSY after enable write buffer!\n", disk->ide->ide_id);
			return -1;
		}
		
		status = IDE_CMD_REG8(disk->ide->cmd_base, IDE_REG_STATUS);
		if(status&0x01){		// ERROR
			printf("ERROR: 0x%x",status);
			return -1;
		}
	}
*/
	return 0;
}

void ide_build_prd(unsigned int *base, unsigned char *buff, unsigned int length)
{
	unsigned int *pprd_base = (unsigned int *) base;

	while (length) {
		pprd_base[0] = 0;
		pprd_base[1] = 0;
		if (length > 0x10000) {
			ADD_PRD_ENTRY(pprd_base, buff, 0x0000, 0);
			buff += 0x10000;
			length -= 0x10000;
		}
		else {
			ADD_PRD_ENTRY(pprd_base, buff, length, 1);
			length = 0;
		}
		pprd_base += 2;
	}
}
#endif

#define IDE_TEST_RW_SIZE	512
#define IDE_TEST_RW_SECTOR	0x1000000
/*----------------------------------------------------------------------
*  ide_ui_test_cmd
*	CLI command for IDE
*	Write 512-byte data (1-sector) to sector 0x1000000, then
*   Read back and verify
*----------------------------------------------------------------------*/
void ide_ui_test_cmd(char argc, char *argv[])
{
	char	*buf1, *buf2, *cp;
	int		i;
	
	if (ide_partitions[0].present == 0)
	{
		printf("No active partition!\n");
		return;
	}
	buf1 = (char *)malloc(IDE_TEST_RW_SIZE);
	if (!buf1)
	{
		printf("No free memeory!\n");
		return;
	}
	buf2 = (char *)malloc(IDE_TEST_RW_SIZE);
	if (!buf2)
	{
		free(buf1);
		printf("No free memeory!\n");
		return;
	}
	
	cp = buf1;
	for (i=0; i<IDE_TEST_RW_SIZE; i++)
		*cp++ = i;
	
	ide_disk_write_sectors(ide_partitions[0].disk, (UINT64)IDE_TEST_RW_SECTOR, (UINT32)IDE_TEST_RW_SIZE, buf1);
	ide_disk_read_sectors(ide_partitions[0].disk, (UINT64)IDE_TEST_RW_SECTOR, (UINT8)(IDE_TEST_RW_SIZE/IDE_SECTOR_SIZE), (UINT16 *)buf2);
	
	for (i=0; i<IDE_TEST_RW_SIZE; i++)
	{
		if (buf1[i] != buf2[i])
		{
			printf("Content error!\n");
			free(buf1);
			free(buf2);
			return;
		}
	}
	
	printf("Content correct!\n");
	free(buf1);
	free(buf2);
	return;
}

#endif // BOARD_SUPPORT_IDE
