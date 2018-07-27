/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: flash_amd_parts.h
* Description	: 
*		Collect supported AMD-like flash chip, including
*		(1) AMD
*		(2) MX
*		(3) AMIC
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/20/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _FLASH_AMD_PARTS_H
#define _FLASH_AMD_PARTS_H

#define FLASH_VENDOR_AMD		0x01
#define FLASH_VENDOR_MX			0xC2
#define FLASH_VENDOR_AMIC_04	0x04
#define FLASH_VENDOR_AMIC_37	0x37

typedef struct flash_dev_info 
{
    unsigned short 		vendor_id;
    unsigned short 		device_id;
    int					vendor_id_addr;
    int					device_id_addr;
    int					sub_id1;		// -1 if not supported
	int					sub_id2;		// -1 if not supported
    int					sub_id1_addr;	// -1 if not supported
    int					sub_id2_addr;	// -1 if not supported
    int					wp_addr;
    unsigned long   	block_size;
    unsigned long    	block_count;
    unsigned long   	base_mask;
    unsigned long   	device_size;
    int     			bootblock;
    unsigned long   	bootblocks[12];	// 0 is bootblock offset, 1-11 sub-sector sizes (or 0)
    int     			banked;
    unsigned long   	banks[2];       // bank offets, highest to lowest (lowest should be 0)
                                // (only one entry for now, increase to support devices
                                // with more banks).
} FLASH_DEV_INFO_T;

static const FLASH_DEV_INFO_T *flash_dev_info;
static const FLASH_DEV_INFO_T cfi_devices;
static const FLASH_DEV_INFO_T supported_devices[] = {
#ifdef FLASH_SUPPORT_MX29LV400
    {   // Bottom
    	vendor_id  		: FLASH_VENDOR_MX,
        device_id		: 0x22BA,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 2,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 0x8,				// 512K / 64K
        device_size		: 0x80000,			// 512K Bytes
        base_mask  		: ~(0x80000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0				// ending
						},
        banked     		: FALSE
    },
    {   // Top
    	vendor_id  		: FLASH_VENDOR_MX,
        device_id		: 0x22B9,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x02,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 0x8,				// 512K / 64K
        device_size		: 0x80000,			// 512K Bytes
        base_mask		: ~(0x80000 - 1),
        bootblock		: TRUE,
        bootblocks		: { 0x070000,		// Starting offset
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_MX29LV400
#ifdef FLASH_SUPPORT_MX29LV800
    {   // Bottom
    	vendor_id  		: FLASH_VENDOR_MX,
        device_id		: 0x225B,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 2,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 0x10,				// 1M / 64K
        device_size		: 0x100000,			// 1M Bytes
        base_mask  		: ~(0x100000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0				// ending
						},
        banked     		: FALSE
    },
    {   // Top
    	vendor_id  		: FLASH_VENDOR_MX,
        device_id		: 0x22DA,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x02,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 0x10,				// 1M / 64K
        device_size		: 0x100000,			// 1M Bytes
        base_mask  		: ~(0x100000 - 1),
        bootblock		: TRUE,
        bootblocks		: { 0x0F0000,		// Starting offset
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_MX29LV800

#ifdef FLASH_SUPPORT_S29DL800
    {   // SPANSION: S29DL800-B
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x22CB,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 2,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 0x10,				// 1M / 64K
        device_size		: 0x100000,			// 1M Bytes
        base_mask  		: ~(0x100000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
						},
        banked     		: FALSE
    },
    {   // SPANSION: S29DL800-T
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x224A,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x02,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 0x10,				// 1M / 64K
        device_size		: 0x100000,			// 1M Bytes
        base_mask		: ~(0x100000 - 1),
        bootblock		: TRUE,
        bootblocks		: { 0x0e0000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_S29DL800
#ifdef FLASH_SUPPORT_AM29DL400
    {   // Bottom
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x220F,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x12,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 8,				// 512K / 64K
        device_size		: 0x80000,			// 512K Bytes
        base_mask  		: ~(0x80000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
						},
        banked     		: FALSE
    },
    {   // TOP
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x220C,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x12,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 8,				// 512K / 64K
        device_size		: 0x80000,			// 512K Bytes
        base_mask		: ~(0x80000 - 1),
        bootblock		: TRUE,
        bootblocks		: { 0x060000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_AM29DL400
#ifdef FLASH_SUPPORT_S29AL004D
    {   // SPANSION: S29AL004D-B
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x22BA,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x12,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 8,				// 512K / 64K
        device_size		: 0x80000,			// 512K Bytes
        base_mask  		: ~(0x80000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x004000,		// 16K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x008000,		// 32K Bytes
                       		0				// ending
						},
        banked     		: FALSE
    },
    {   // SPANSION: S29AL004D-T
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x22B9,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 2,				// 0x12,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 8,				// 512K / 64K
        device_size		: 0x80000,			// 512K Bytes
        base_mask		: ~(0x80000 - 1),
        bootblock		: TRUE,
        bootblocks		: { 0x070000,		// Starting offset
                       		0x008000,		// 32K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x004000,		// 16K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_S29AL004D
#ifdef FLASH_SUPPORT_AM29LV640M
    {   // AM29LV640MB
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x227E,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: 0x2210,
        sub_id2			: 0x2200,			// bottom boot
        sub_id1_addr	: 0x0e,
        sub_id2_addr	: 0x0f,			
        wp_addr			: 2,				// 0x2,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 128,				// 8M / 64K
        device_size		: 0x800000,			// 8M Bytes
        base_mask  		: ~(0x800000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0				// ending
						},
        banked     		: FALSE
    },
    {   // AM29LV640MT
    	vendor_id  		: FLASH_VENDOR_AMD,
        device_id		: 0x227E,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: 0x2210,
        sub_id2			: 0x2201,			// top boot
        sub_id1_addr	: 0x0e,
        sub_id2_addr	: 0x0f,			
        wp_addr			: 2,				// 0x2,
        block_size		: 0x10000,			// 64K Bytes
        block_count		: 128,				// 8M / 64K
        device_size		: 0x800000,			// 8M Bytes
        base_mask		: ~(0x800000 - 1),
        bootblock		: TRUE,
        bootblocks		: { 0x7f0000,		// Starting offset
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_AM29LV640M
#ifdef FLASH_SUPPORT_MX29LV640BT
    {   // MX29LV640BT	// TOP
    	vendor_id  		: FLASH_VENDOR_MX,
        device_id  		: 0x22C9,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 0,				// 0x12,
        block_size 		: 0x10000,			// 64K Bytes
        block_count		: 128,				// 8M / 64K
        device_size		: 0x800000,			// 8M Bytes
        base_mask  		: ~(0x800000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x7F0000,		// Starting offset
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_MX29LV640BT

#ifdef FLASH_SUPPORT_MX29LV640BB
    {   // MX29LV640BB	// Bottom
    	vendor_id  		: FLASH_VENDOR_MX,
        device_id  		: 0x22CB,
        vendor_id_addr	: 0x00,
        device_id_addr	: 0x01,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 0,				// 0x12,
        block_size 		: 0x10000,			// 64K Bytes
        block_count		: 128,				// 8M / 64K
        device_size		: 0x800000,			// 8M Bytes
        base_mask  		: ~(0x800000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_MX29LV640BB

#ifdef FLASH_SUPPORT_AMIC_AM29LV640MB
    {   // AM29LV640MB
    	vendor_id  		: FLASH_VENDOR_AMIC_04,
        device_id  		: 0x227E,
        vendor_id_addr	: 0x10,
        device_id_addr	: 0x11,
        sub_id1			: -1,
        sub_id2			: -1,
        sub_id1_addr	: -1,
        sub_id2_addr	: -1,			
        wp_addr			: 0,				// 0x12,
        block_size 		: 0x10000,			// 64K Bytes
        block_count		: 128,				// 8M / 64K
        device_size		: 0x800000,			// 8M Bytes
        base_mask  		: ~(0x800000 - 1),
        bootblock  		: TRUE,
        bootblocks 		: { 0x000000,		// Starting offset
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0x002000,		// 8K Bytes
                       		0				// ending
                     		},
        banked     		: FALSE
    },
#endif // FLASH_SUPPORT_AMIC_AM29LV640MB
};
#define NUM_DEVICES (sizeof(supported_devices)/sizeof(FLASH_DEV_INFO_T))

#endif // _FLASH_AMD_PARTS_H

