/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ide.h
* Description	: Collect IDE definition
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	09/07/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _IDE_H_
#define _IDE_H_

#if defined(BOARD_SUPPORT_IDE0) && defined(BOARD_SUPPORT_IDE1)
	#define IDE_CONTROLLERS			2
#else
	#define IDE_CONTROLLERS			1
#endif

//#define IDE_CONTROLLERS			2
#define IDE_MAX_DISKS			1 //2 
#define IDE_SECTOR_SIZE 		512
#define IDE_MAX_PARTS_PER_DISK	4	//1
#define IDE_MAX_PARTITIONS		IDE_MAX_PARTS_PER_DISK*IDE_CONTROLLERS

//for usb device mode
	typedef struct CommandBlockWrapper
	{
		UINT32 u32Signature;
		UINT32 u32Tag;
		UINT32 u32DataTransferLength;
		UINT8 u8Flags;
		UINT8 u8LUN;
		UINT8 u8CBLength;
		UINT8 u8CB[16];
	} CBW;

	typedef struct CommandStatusWrapper
	{
		UINT32 u32Signature;
		UINT32 u32Tag;
		UINT32 u32DataResidue;
		UINT8 u8Status;
	} CSW;
#define MAX_BUFFER_SIZE	0x10000

#define IDE_BM_COMMAND(x)	*(volatile unsigned char*)(x + 0)
#define IDE_BM_STATUS(x)	*(volatile unsigned char*)(x + 2)
#define IDE_BM_PRD_BASE(x)	*(volatile unsigned long*)(x + 4)
#define IDE_BM_PIO_TIMING(x)	*(volatile unsigned char*)(x + 0x10)
#define IDE_BM_MW_TIMING(x)	*(volatile unsigned char*)(x + 0x11)
#define IDE_BM_UDMA_TIMING0(x)	*(volatile unsigned char*)(x + 0x12)
#define IDE_BM_UDMA_TIMING1(x)	*(volatile unsigned char*)(x + 0x13)
#define IDE_BM_CLOCK_MODE(x)	*(volatile unsigned char*)(x + 0x14)

#define SL2312_IDE_BM_OFFSET			0x00
#define SL2312_IDE_PIO_TIMING_OFFSET	0x10
#define SL2312_IDE_MDMA_TIMING_OFFSET	0x11		// only support 1 multi-word DMA device
#define SL2312_IDE_UDMA_TIMING0_OFFSET	0x12		// for master
#define SL2312_IDE_UDMA_TIMING1_OFFSET	0x13		// for slave
#define SL2312_IDE_CLK_MOD_OFFSET		0x14		
#define SL2312_IDE_CMD_OFFSET			0x20
#define SL2312_IDE_CTRL_OFFSET			0x36

#define PIO_TIMING_REG(x)	*(volatile unsigned char*)(x + SL2312_IDE_PIO_TIMING_OFFSET)
#define MDMA_TIMING_REG(x)	*(volatile unsigned char*)(x + SL2312_IDE_MDMA_TIMING_OFFSET)
#define UDMA_TIMING0_REG(x)	*(volatile unsigned char*)(x + SL2312_IDE_UDMA_TIMING0_OFFSET)
#define UDMA_TIMING1_REG(x)	*(volatile unsigned char*)(x + SL2312_IDE_UDMA_TIMING1_OFFSET)
#define CLK_MOD_REG(x)		*(volatile unsigned char*)(x + SL2312_IDE_CLK_MOD_OFFSET)



#define IDE_CMD_REG8(base,addr)			REG8(base+addr)
#define IDE_CMD_REG16(base,addr)		REG16(base+addr)
#define IDE_CMD_REG32(base,addr)		REG32(base+addr)
#define IDE_CTRL_REG8(base)				REG8(base)

// IDE Register Indices
#define IDE_REG_DATA      0
#define IDE_REG_ERROR     1
#define IDE_REG_FEATURES  1
#define IDE_REG_COUNT     2
#define IDE_REG_REASON    2  // ATAPI
#define IDE_REG_LBALOW    3
#define IDE_REG_LBAMID    4
#define IDE_REG_LBAHI     5
#define IDE_REG_DEVICE    6
#define IDE_REG_STATUS    7
#define IDE_REG_COMMAND   7

#define IDE_STAT_BSY      0x80
#define IDE_STAT_DRDY     0x40
#define IDE_STAT_SERVICE  0x10
#define IDE_STAT_DRQ      0x08
#define IDE_STAT_CORR     0x04
#define IDE_STAT_ERR      0x01

#define IDE_REASON_REL    0x04
#define IDE_REASON_IO     0x02
#define IDE_REASON_COD    0x01

/* flag values */
#define IDE_DEV_PRESENT  1  // Device is present
#define IDE_DEV_PACKET   2  // Supports packet interface
#define IDE_DEV_ADDR48   3  // Supports 48bit addressing

//
// Drive ID offsets of interest
//
#define IDE_DEVID_GENCONFIG      0
#define IDE_DEVID_SERNO         20
#define IDE_DEVID_MODEL         54
#define IDE_DEVID_LBA_CAPACITY 	120
#define IDE_LBA_48				(100*2)
#define IDE_CMD_SET				(83*2)

#define	ATA_ID_WORDS		 (256)
#define	ATA_ID_SERNO_OFS	 (10)
#define	ATA_ID_FW_REV_OFS	 (23)
#define	ATA_ID_PROD_OFS		 (27)
#define	ATA_ID_OLD_PIO_MODES	 (51)
#define	ATA_ID_FIELD_VALID	 (53)
#define	ATA_ID_MWDMA_MODES	 (63)
#define	ATA_ID_PIO_MODES	 (64)
#define	ATA_ID_EIDE_DMA_MIN	 (65)
#define	ATA_ID_EIDE_PIO		 (67)
#define	ATA_ID_EIDE_PIO_IORDY	 (68)
#define	ATA_ID_UDMA_MODES	 (88*2)
#define	ATA_ID_MAJOR_VER	 (80*2)
#define ATA_ID_COMMAND_SET0	 (82*2)
#define ATA_ID_COMMAND_SET1	 (83*2)
#define ATA_ID_COMMAND_SET2	 (84*2)

#define	ATA_SERNO_LEN		 20
#define	ATA_UDMA0		 1
#define	ATA_UDMA1		 ATA_UDMA0 << 1
#define	ATA_UDMA2		 ATA_UDMA1 << 1
#define	ATA_UDMA3		 ATA_UDMA2 << 1
#define	ATA_UDMA4		 ATA_UDMA3 << 1
#define	ATA_UDMA5		 ATA_UDMA4 << 1
#define	ATA_UDMA6		 ATA_UDMA5 << 1

#define XFER_UDMA_6		0x46	/* 0100|0110 */
#define XFER_UDMA_5		0x45	/* 0100|0101 */
#define XFER_UDMA_4		0x44	/* 0100|0100 */
#define XFER_UDMA_3		0x43	/* 0100|0011 */
#define XFER_UDMA_2		0x42	/* 0100|0010 */
#define XFER_UDMA_1		0x41	/* 0100|0001 */
#define XFER_UDMA_0		0x40	/* 0100|0000 */
#define XFER_MW_DMA_2	0x22	/* 0010|0010 */
#define XFER_MW_DMA_1	0x21	/* 0010|0001 */
#define XFER_MW_DMA_0	0x20	/* 0010|0000 */
#define XFER_SW_DMA_2	0x12	/* 0001|0010 */
#define XFER_SW_DMA_1	0x11	/* 0001|0001 */
#define XFER_SW_DMA_0	0x10	/* 0001|0000 */
#define XFER_PIO_4		0x0C	/* 0000|1100 */
#define XFER_PIO_3		0x0B	/* 0000|1011 */
#define XFER_PIO_2		0x0A	/* 0000|1010 */
#define XFER_PIO_1		0x09	/* 0000|1001 */
#define XFER_PIO_0		0x08	/* 0000|1000 */
#define XFER_PIO_SLOW	0x00	/* 0000|0000 */

// HD command
#define WIN_READ		0x20
#define WIN_WRITE		0x30
#define WIN_WRITE_VERIFY	0x3C
#define WIN_VERIFY		0x40
#define WIN_IDENTIFY		0xEC	/* ask drive to identify itself	*/
#define WIN_SETFEATURES		0xEF	/* set special drive features */
#define WIN_READDMA		0xC8	/* read sectors using DMA transfers */
#define WIN_WRITEDMA		0xCA	/* write sectors using DMA transfers */

// 48 bit LBA command
#define WIN_READDMA_EXT		0x25
#define WIN_WRITEDMA_EXT	0x35
#define WIN_READ_EXT		0x24
#define WIN_WRITE_EXT		0x34

// Kinds of disks
#define DISK_IDE_HD     		1
#define DISK_IDE_CDROM  		2
#define DISK_FLOPPY     		3

// DOS partition table as laid out in the MBR
//
typedef struct mbr_partition {
	UINT8	boot_ind;		// 0x80 == active
    UINT8	start_head;
    UINT8	start_sector;
    UINT8	start_cyl;
    UINT8	sys_ind;		// partition type
    UINT8	end_head;
    UINT8	end_sector;
    UINT8	end_cyl;
    UINT8	start_sect[4];	// starting sector counting from 0
    UINT8	sector_num[4];	// number of sectors in partition
} IDE_MBR_T;

#define MBR_PTABLE_OFFSET 		0x1be
#define MBR_MAGIC_OFFSET  		0x1fe
#define MBR_MAGIC         		0xaa55

#define IDE_PART_FAT12        	0x01
#define IDE_PART_FAT16_32M    	0x04
#define IDE_PART_EXTENDED     	0x05
#define IDE_PART_FAT16        	0x06
#define IDE_PART_NTFS	       	0x07
#define IDE_PART_FAT32	       	0x0C
#define IDE_PART_LINUX_MINIX   	0x81
#define IDE_PART_LINUX_SWAP   	0x82
#define IDE_PART_LINUX			0x83
#define IDE_PART_LINUX_EXTENDED	0x85
#define IDE_PART_LINUX_RAID	0xFD

struct disk_t;
struct ide_t;

typedef int			disk_read_f(struct disk_t *disk, UINT64 start, UINT32 byte_num, UINT16 *buf);
typedef int			disk_write_f(struct disk_t *disk, UINT64 start, UINT32 byte_num, UINT16 *buf);
typedef int			part_read_f(struct part_t *part, UINT64 start, UINT32 byte_num, UINT16 *buf);
typedef int			part_write_f(struct part_t *part, UINT64 start, UINT32 byte_num, UINT16 *buf);

typedef struct part_t {
	unsigned int	present;
	unsigned int	part_id;			// partition id in disk, 0,1, ...
    struct disk_t	*disk;
    part_read_f		*read;
    part_write_f	*write;
    UINT32			start_sector;		// first sector in partition
    UINT64			sector_num;			// number of sectors in partition
    UINT8			os;        			// FAT12, FAT16, Linux, etc.
    UINT8			bootflag;       	// not really used...
#ifdef BOARD_SUPPORT_RAID
    u8			status;			// to see if this partition be procecced.
    sector_t		sb_offset;		// super block offset
    u8			sb_page[4096];		// super block content
    UINT64		rdev_size;		// import size
#endif    
} IDE_PART_T;

extern IDE_PART_T	ide_partitions[IDE_MAX_PARTITIONS];
extern int			ide_part_num;

typedef struct disk_t{
	unsigned int	present;
	struct ide_t	*ide;
	disk_read_f	*read;		
	disk_write_f	*write;
#ifdef USB_DEV_MO
	unsigned char	raid_idx;
	disk_read_f	*read_dma;		
	disk_write_f	*write_dma;
	unsigned char	write_buff;
	unsigned char	look_ahead;
#endif	
	unsigned int	drive_id;
	UINT32		flags;
	int		kind;
	UINT64		sector_num;
	int		part_num;			// partition number per disk
	int		lba_48;
} IDE_DISK_T;

typedef struct ide_t{
	unsigned int	present;
	int		ide_id;
	int		irq_expect;
	unsigned int	bm_base;
	unsigned int	cmd_base;
	unsigned int	ctrl_base;
	int				disk_num;
	IDE_DISK_T		disk[IDE_MAX_DISKS];
} IDE_INFO_T;

extern IDE_INFO_T	ide_info[IDE_CONTROLLERS];

/*----------------------------------------------------------------------
* ide_get_uint16
*----------------------------------------------------------------------*/
static inline UINT16 ide_get_uint16(char *datap)
{
	return (((*(datap+1)) << 8) + (*(datap)));
}

/*----------------------------------------------------------------------
* ide_get_uint32
*----------------------------------------------------------------------*/
static inline UINT32 ide_get_uint32(char *datap)
{
	return (((*(datap+3)) << 24) + 
			((*(datap+2)) << 16) + 
			((*(datap+1)) << 8) +
			(*(datap+0)));
}

#define SWAB32(x)				\
	((((x) & 0xff) << 24)   |	\
	(((x) & 0xff00) <<  8)	|	\
	(((x) >> 8) & 0xff00)	|	\
	(((x) >> 24) & 0xff))

#define SWAB16(x) \
    ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))

#define	SWAB_LE32(x)	(x)
#define	SWAB_LE16(x)	(x)

#define DISK_READ(d,s,n,p)			((d)->read)((d),(s),(n),(UINT16 *)(p))

#ifdef USB_DEV_MO
#define PARTITION_READ(part,s,n,p)	\
		((part)->read)(part,(s) ,(n<<9),(UINT16 *)(p))
#else
#define PARTITION_READ(part,s,n,p)	\
		((part)->read)((part->disk),(s) + (part)->start_sector,(n),(UINT16 *)(p))
#endif

#define MALLOC_DMA(size)	((u32)malloc(size+0x10000) & ~0x10000)
#define MFREE_DMA(mem,size)	free(mem)


#endif // _IDE_H_
