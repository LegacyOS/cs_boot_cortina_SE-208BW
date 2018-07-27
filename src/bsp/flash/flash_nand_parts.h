#ifndef _FLASH_NAND_PARTS_H
#define _FLASH_NAND_PARTS_H

#define FLASH_DEBUG  0
#define FLASHWORD(x)			(x & 0xff)
//----------------------------------------------------------------------------
// Common device details.
#define FLASH_Read_ID                   FLASHWORD(0x90)
#define FLASH_Reset                     FLASHWORD(0xFF)
#define FLASH_Read_Mode1                FLASHWORD(0x00)
#define FLASH_Read_Mode2                FLASHWORD(0x01)
#define FLASH_Read_Mode3                FLASHWORD(0x50)
#define FLASH_Program                   FLASHWORD(0x10)
#define FLASH_Send_Data                 FLASHWORD(0x80)
#define FLASH_Status                    FLASHWORD(0x70)
#define FLASH_Block_Erase               FLASHWORD(0x60)
#define FLASH_Start_Erase               FLASHWORD(0xD0)


/*
 * NAND Flash Manufacturer ID Codes
 */
 //#define CYGNUM_FLASH_ID_MANUFACTURER    FLASHWORD(0x98)
#define CYGNUM_FLASH_VENDOR_TOSHIBA    FLASHWORD(0x98)
#define CYGNUM_FLASH_VENDOR_SAMSUNG    FLASHWORD(0xec)
#define CYGNUM_FLASH_VENDOR_FUJITSU    FLASHWORD(0x04)
#define CYGNUM_FLASH_VENDOR_NATIONAL    FLASHWORD(0x8f)
#define CYGNUM_FLASH_VENDOR_RENESAS    FLASHWORD(0x07)
#define CYGNUM_FLASH_VENDOR_STMICRO    FLASHWORD(0x20)
#define CYGNUM_FLASH_VENDOR_HYNIX    FLASHWORD(0xad)
#define CYGNUM_FLASH_VENDOR_MICRON    FLASHWORD(0x2c)
#define CYGNUM_FLASH_VENDOR_AMD    FLASHWORD(0x01)



//--> storlink define
#define	FLASH_WORD_ID		        0x00000000
#define	FLASH_STATUS		        0x00000008
#define	FLASH_TYPE		        0x0000000C
#define	NFLASH_ACCESS		        0x00000030
#define	NFLASH_COUNT		        0x00000034
#define	NFLASH_COMMAND_ADDRESS         	0x00000038
#define	NFLASH_ADDRESS            	0x0000003C
#define	NFLASH_DATA		        0x00000040
#define	NFLASH_TIMING 	            	0x0000004C   
#define	NFLASH_ECC_STATUS		0x00000050
#define	NFLASH_ECC_CONTROL		0x00000054
#define	NFLASH_ECC_OOB			0x0000005c
#define	NFLASH_ECC_CODE_GEN0		0x00000060
#define	NFLASH_ECC_CODE_GEN1		0x00000064
#define	NFLASH_ECC_CODE_GEN2		0x00000068
#define	NFLASH_ECC_CODE_GEN3		0x0000006C
#define	NFLASH_FIFO_CONTROL		0x00000070
#define	NFLASH_FIFO_STATUS		0x00000074
#define	NFLASH_FIFO_ADDRESS		0x00000078
#define	NFLASH_FIFO_DATA		0x0000007c

#define NFLASH_CHIP0_EN            	0x00000000  // 16th bit = 0
#define NFLASH_CHIP1_EN            	0x00010000  // 16th bit = 1

#define	NFLASH_WiDTH8              	0x00000000
#define	NFLASH_WiDTH16             	0x00000400
#define	NFLASH_WiDTH32             	0x00000800

#define	NFLASH_DIRECT              	0x00004000 //0x00004000 0x00044000
#define	NFLASH_INDIRECT            	0x00000000

/* DMA Registers */
#define	DMA_MAIN_CFG 		   		0x00000024
#define	DMA_INT_TC_CLR				0x00000008
#define	DMA_TC						0x00000014

#define	DMA_CH0_CSR    				0x00000100
#define	DMA_CH0_CFG    				0x00000104
#define	DMA_CH0_SRC_ADDR    		0x00000108
#define	DMA_CH0_DST_ADDR    		0x0000010c
#define	DMA_CH0_LLP    				0x00000110
#define	DMA_CH0_SIZE    			0x00000114

#define PASS	   1
#define FAIL       2
#define ECCERROR   3
#define Sequential_Data_Input_cmd  0x80
#define Read1_cmd    0x00
#define Read2_cmd    0x50
#define ReadID_cmd   0x90
#define Reset_cmd    0xff
#define Page_Program_cmd  0x10
#define Block_Erase_cmd   0x60
#define Block_Erase_Confirm_cmd  0xd0
#define Read_Status_cmd   0x70
#define FLASH_INTERLEAVE 1

//nand count register
typedef union
{
	unsigned int bits32;
	struct bit_0034
	{
		unsigned int reserved		: 1;	// bit 31
		unsigned int oob_cnt		: 7;	// bit 30-24
		unsigned int reserved1		: 4;	// bit 23-20
		unsigned int data_cnt		: 12;	// bit 19:8
		unsigned int reserved2		: 1;	// bit 7
		unsigned int addr_cnt		: 3;	// bit 6:4
		unsigned int reserved3		: 2;	// bit 3-2
		unsigned int cmd_cnt		: 2;	// bit 1:0

	} bits;
} NFLASH_COUNT_T;


#define	PAGE512_SIZE              	0x200
#define	PAGE512_OOB_SIZE              	0x10
#define	PAGE512_RAW_SIZE              	0x210
#define	PAGE2K_SIZE              		0x800
#define	PAGE2K_OOB_SIZE              	0x40
#define	PAGE2K_RAW_SIZE              	0x840
#define	FLASH_TYPE_MASK             	0x1800
#define	FLASH_WIDTH_MASK             	0x0400
#define	FLASH_SIZE_MASK             	0x0300
#define	FLASH_SIZE_32 	            	0x0000
#define	FLASH_SIZE_64 	            	0x0100
#define	FLASH_SIZE_128 	            	0x0200
#define	FLASH_SIZE_256 	            	0x0300

#define	FLASH_TYPE_NAND             	0x1000
#define	FLASH_TYPE_NOR             	0x0800
#define	FLASH_TYPE_SERIAL             	0x0000


#define	FLASH_START_BIT             	0x80000000
#define	FLASH_RD             	0x00002000
#define	FLASH_WT             	0x00003000
#define	ECC_CHK_MASK             	0x00000003
#define	ECC_UNCORRECTABLE             	0x00000003
#define	ECC_1BIT_DATA_ERR             	0x00000001
#define	ECC_1BIT_ECC_ERR             	0x00000002
#define	ECC_NO_ERR             	0x00000000
#define	ECC_ERR_BYTE             	0x0000ff80
#define	ECC_ERR_BIT             	0x00000078

#define	ECC_CLR             	0x00000001
#define	ECC_PAUSE_EN             	0x00000002

#define	STS_WP             	0x80
#define	STS_READY             	0x40
#define	STS_TRUE_READY             	0x40

// <--

//typedef unsigned long long UINT64;

typedef struct flash_dev_info {
    UINT8	device_id;
    UINT8	vendor_id;
    UINT8	extid;	
    UINT32	block_size;
    UINT32	page_size;
    UINT32  block_count;
    UINT32  base_mask;
    UINT32  device_size;
} flash_dev_info_t;

static const flash_dev_info_t supported_devices[] = {

{  // 64Mb (8MB)
    device_id  : FLASHWORD(0xe6),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x2000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x800000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x800000 * FLASH_INTERLEAVE - 1),
},
{  // 64Mb (8MB)
    device_id  : FLASHWORD(0x6d),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x2000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x800000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x800000 * FLASH_INTERLEAVE - 1),
},
{  // 64Mb (8MB)
    device_id  : FLASHWORD(0x70),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x2000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x800000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x800000 * FLASH_INTERLEAVE - 1),
},
{  // 64Mb (8MB)
    device_id  : FLASHWORD(0x39),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x2000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x800000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x800000 * FLASH_INTERLEAVE - 1),
},

{  // 128Mb (16MB)
    device_id  : FLASHWORD(0x73),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x1000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x1000000 * FLASH_INTERLEAVE - 1),
},
{  // 128Mb (16MB)
    device_id  : FLASHWORD(0xf2),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x1000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x1000000 * FLASH_INTERLEAVE - 1),
},
{  // 128Mb (16MB)
    device_id  : FLASHWORD(0x33),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x1000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x1000000 * FLASH_INTERLEAVE - 1),
},
{  // 128Mb (16MB)
    device_id  : FLASHWORD(0x53),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x1000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x1000000 * FLASH_INTERLEAVE - 1),
},

{  // 256Mb (32MB)
    device_id  : FLASHWORD(0xf4),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 2048,
    device_size: 0x2000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x2000000 * FLASH_INTERLEAVE - 1),
},
{  // 256Mb (32MB)
    device_id  : FLASHWORD(0x75),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 2048,
    device_size: 0x2000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x2000000 * FLASH_INTERLEAVE - 1),
},
{  // 256Mb (32MB)
    device_id  : FLASHWORD(0x35),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 2048,
    device_size: 0x2000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x2000000 * FLASH_INTERLEAVE - 1),
},
{  // 256Mb (32MB)
    device_id  : FLASHWORD(0x45),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 2048,
    device_size: 0x2000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x2000000 * FLASH_INTERLEAVE - 1),
},
{  // 256Mb (32MB)
    device_id  : FLASHWORD(0x55),
    vendor_id  : FLASHWORD(0xec),
    extid : 0,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 2048,
    device_size: 0x2000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x2000000 * FLASH_INTERLEAVE - 1),
},

{  // 512Mb (64MB)
    device_id  : FLASHWORD(0xf7),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 4096,
    device_size: 0x4000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x4000000 * FLASH_INTERLEAVE - 1),
},
{  // 512Mb (64MB)
    device_id  : FLASHWORD(0x76),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 4096,
    device_size: 0x4000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x4000000 * FLASH_INTERLEAVE - 1),
},
{  // 512Mb (64MB)
    device_id  : FLASHWORD(0x36),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 4096,
    device_size: 0x4000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x4000000 * FLASH_INTERLEAVE - 1),
},
{  // 512Mb (64MB)
    device_id  : FLASHWORD(0x56),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 4096,
    device_size: 0x4000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x4000000 * FLASH_INTERLEAVE - 1),
},

{  // 1024Mb (128MB) small page
    device_id  : FLASHWORD(0xf8),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 8192,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (128MB) small page
    device_id  : FLASHWORD(0x79),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 8192,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (128MB) small page
    device_id  : FLASHWORD(0x78),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 8192,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (128MB) small page
    device_id  : FLASHWORD(0x74),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 8192,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (128MB) small page
    device_id  : FLASHWORD(0xb1),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 8192,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (128MB) small page
    device_id  : FLASHWORD(0xc1),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x4000 * FLASH_INTERLEAVE,
    page_size  : 0x200 * FLASH_INTERLEAVE,
    block_count: 8192,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},


{  // 1024Mb (128MB) large page
    device_id  : FLASHWORD(0xA1),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x20000 * FLASH_INTERLEAVE,
    page_size  : 0x800 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (128MB) large page
    device_id  : FLASHWORD(0xf1),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x20000 * FLASH_INTERLEAVE,
    page_size  : 0x800 * FLASH_INTERLEAVE,
    block_count: 1024,
    device_size: 0x8000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x8000000 * FLASH_INTERLEAVE - 1),
},
{  // 2048Mb (256MB) large page
    device_id  : FLASHWORD(0xda),
    vendor_id  : FLASHWORD(0xec),
    extid : 2,
    block_size : 0x20000 * FLASH_INTERLEAVE,
    page_size  : 0x800 * FLASH_INTERLEAVE,
    block_count: 2048,
    device_size: 0x10000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x10000000 * FLASH_INTERLEAVE - 1),
},
{  // 1024Mb (1024MB) large page
    device_id  : FLASHWORD(0xD3),
    vendor_id  : FLASHWORD(0x20),
    extid : 3,
    block_size : 0x40000 * FLASH_INTERLEAVE,
    page_size  : 0x800 * FLASH_INTERLEAVE,
    block_count: 4096,
    device_size: 0x40000000 * FLASH_INTERLEAVE,
    base_mask  : ~(0x40000000 * FLASH_INTERLEAVE - 1),
},
};


void flash_delay(void);

#endif // ifndef CYGONCE_DEVS_FLASH_TOSHIBA_TC58XXX_PARTS_INL


