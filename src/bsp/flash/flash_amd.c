/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: flash_amd.h
* Description	: 
*		Handle AMD-like flash chip, including
*		(1) AMD
*		(2) MX
*		(3) amd
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/20/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
//#ifdef FLASH_TYPE_PARALLEL	

#include <sl2312.h>
#include "flash_drv.h"
#include "flash_amd_parts.h"

#define FLASH_BASE 				(SL2312_FLASH_SHADOW)
#define FLASH_WIDTH				(16)
#define FLASH_INTERLEAVE		(1)

typedef unsigned short			FLASH_DATA_T;
#define FLASH_BLANKVALUE		0xffff
#define FLASHWORD(x)			(x & 0xff)
#define FLASH_POLLING_TIMEOUT	(3000000)

//----------------------------------------------------------------------------
// Common device details.
#define FLASH_READ_ID                   FLASHWORD( 0x90 )
#define FLASH_WP_STATE                  FLASHWORD( 0x90 )
#define FLASH_RESET                     FLASHWORD( 0xF0 )
#define FLASH_PROGRAM                   FLASHWORD( 0xA0 )
#define FLASH_BLOCK_ERASE               FLASHWORD( 0x30 )
#define FLASH_Query						FLASHWORD( 0x98 ) // Add by Jason for CFI support

#define FLASH_DATA                      FLASHWORD( 0x80 ) // Data complement
#define FLASH_BUSY                      FLASHWORD( 0x40 ) // "Toggle" bit
#define FLASH_ERR                       FLASHWORD( 0x20 )
#define FLASH_SECTOR_ERASE_TIMER        FLASHWORD( 0x08 )

#define FLASH_UNLOCKED                  FLASHWORD( 0x00 )

#define FLASH_SETUP_ADDR1              	(0x555)
#define FLASH_SETUP_ADDR2              	(0x2AA)
#define FLASH_RESET_ADDR				(0x100)
//#define FLASH_VENDORID_ADDR            	(0x10)
//#define FLASH_DEVICEID_ADDR            	(0x11)
//#define FLASH_WP_ADDR                  	(0x12)
#define FLASH_SETUP_CODE1               FLASHWORD( 0xAA )
#define FLASH_SETUP_CODE2               FLASHWORD( 0x55 )
#define FLASH_SETUP_ERASE               FLASHWORD( 0x80 )

#define FLASH_P2V( _a_ ) ((volatile FLASH_DATA_T *)((unsigned long)((_a_))))

static int flash_qry_cfi(FLASH_DEV_INFO_T *flash_dev_info);
static int flash_amd_erase_block(void* block, unsigned int size);
static int flash_amd_program(void* addr, void* data, int len);

/*----------------------------------------------------------------------
* flash_amd_query
*----------------------------------------------------------------------*/
static void flash_amd_query(UINT32 *vendor, UINT32 *device_id, UINT32 *sub1, UINT32 *sub2)
{
    volatile FLASH_DATA_T *ROM;
    volatile FLASH_DATA_T *f_s1, *f_s2;
    FLASH_DATA_T w, tmp;

    ROM = (FLASH_DATA_T*) FLASH_BASE;
    f_s1 = FLASH_P2V(ROM+FLASH_SETUP_ADDR1);
    f_s2 = FLASH_P2V(ROM+FLASH_SETUP_ADDR2);

    *f_s1 = FLASH_RESET;
	hal_delay_us(2);
    w = *(FLASH_P2V(ROM+FLASH_RESET_ADDR));

    *f_s1 = FLASH_SETUP_CODE1;
	hal_delay_us(2);
    *f_s2 = FLASH_SETUP_CODE2;
	hal_delay_us(2);
    *f_s1 = FLASH_READ_ID;
	hal_delay_us(2);

	*vendor = 0;
	*device_id = 0;
	*sub1 = -1;
	*sub2 = -1;
	
    // Manufacturers' code
    tmp = *(FLASH_P2V(ROM+flash_dev_info->vendor_id_addr));
    *vendor = tmp;
    // Part number
    tmp = *(FLASH_P2V(ROM+flash_dev_info->device_id_addr));
    *device_id = tmp;

    // sub id 1
    if (flash_dev_info->sub_id1_addr != -1)
    {
    	tmp = *(FLASH_P2V(ROM+flash_dev_info->sub_id1_addr));
    	*sub1 = tmp;
    }

    // sub id 2
    if (flash_dev_info->sub_id2_addr != -1)
    {
    	tmp = *(FLASH_P2V(ROM+flash_dev_info->sub_id2_addr));
    	*sub2 = tmp;
	}    
    
    *(FLASH_P2V(ROM+FLASH_RESET_ADDR)) = FLASH_RESET;

    // Stall, waiting for flash to return to read mode.
    while (w != *(FLASH_P2V(ROM+FLASH_RESET_ADDR)));
    
}

/*----------------------------------------------------------------------
* flash_qry_cfi
*----------------------------------------------------------------------*/
static int flash_qry_cfi(FLASH_DEV_INFO_T *flash_dev_info)
{
    volatile FLASH_DATA_T *ROM;
    volatile FLASH_DATA_T *f_s1;
    FLASH_DATA_T cfi[17], qry[3];
    FLASH_DATA_T w;
    int i,erase_blocks[4],block_size[4],max_block_size;
    long timeout = 500000;

    ROM = (FLASH_DATA_T*) FLASH_BASE;
    f_s1 = FLASH_P2V(ROM+FLASH_SETUP_CODE2);

    *f_s1 = FLASH_RESET;
	hal_delay_us(2);
    w = *(FLASH_P2V(ROM));

    *f_s1 = FLASH_Query;
	hal_delay_us(2);

    for	(i=0;i<3;i++)
    	qry[i] = *(FLASH_P2V(ROM+0x10+i));
    if (qry[0] != 'Q' || qry[1] != 'R' || qry[2] != 'Y')
    	return -1;

    for	(i=0;i<17;i++)
    	cfi[i] = *(FLASH_P2V(ROM+0x2C+i));
    
//    for(i=0;i<12;i++)
//    	diag_printf("cfi test %x\n",cfi[i]);
    
    max_block_size = 0;
    flash_dev_info->device_size=0;
    flash_dev_info->block_count = 0 ;
    for(i=0;i<cfi[0];i++)
    {	
    	erase_blocks[i]= (cfi[i*4+2]<<8)|cfi[i*4+1];
    	block_size[i]= ((cfi[i*4+4]<<8)|cfi[i*4+3] )*256;
    	flash_dev_info->block_count += erase_blocks[i]+1;
    	flash_dev_info->device_size += block_size[i]*(erase_blocks[i]+1);
    	if( block_size[i] > max_block_size )	max_block_size = block_size[i] ;
    }
    flash_dev_info->base_mask=~(flash_dev_info->device_size-1);
    flash_dev_info->block_size=max_block_size;
    
    if(cfi[0]>1){
    	flash_dev_info->bootblock=1;
    	if(block_size[0]<block_size[1]){
    		flash_dev_info->bootblocks[0]=0x0000;
    		for(i=1;i< (erase_blocks[0]+2);i++)
    			flash_dev_info->bootblocks[i]=block_size[0];
    		flash_dev_info->bootblocks[i] = -1 ;
    	}
    	else {
    		flash_dev_info->bootblocks[0]=block_size[1]*erase_blocks[1];
    		for(i=1;i<(erase_blocks[1]+2);i++)
    			flash_dev_info->bootblocks[i]=block_size[1];
    		flash_dev_info->bootblocks[i] = -1 ;
    	}
    }
	
	flash_dev_info->banked = 0;

    *(FLASH_P2V(ROM)) = FLASH_RESET;

    // Stall, waiting for flash to return to read mode.
    while ((--timeout != 0) && (w != *(FLASH_P2V(ROM)))) ;
    
    return 0;
}
/*----------------------------------------------------------------------
* 	flash_amd_init
*----------------------------------------------------------------------*/
int flash_amd_init(FLASH_INFO_T *info)
{
    FLASH_DATA_T id[2];
    UINT32 vendor, device_id, sub_id1, sub_id2;
    int i;

    flash_dev_info = supported_devices;
    for (i = 0; i < NUM_DEVICES; i++)
    {
    	flash_amd_query(&vendor, &device_id, &sub_id1, &sub_id2);
        if (flash_dev_info->vendor_id == vendor &&
        	flash_dev_info->device_id == device_id &&
        	flash_dev_info->sub_id1 == sub_id1 &&
        	flash_dev_info->sub_id2 == sub_id2)
        {
            break;
        }
        flash_dev_info++;
    }

    if (NUM_DEVICES == i)
    {
    	flash_dev_info = &cfi_devices;
    	if (flash_qry_cfi((FLASH_DEV_INFO_T *)flash_dev_info) != 0)
        	return FLASH_ERR_DRV_WRONG_PART;
    }

    // Hard wired for now
   	info->erase_block 	= (void *)flash_amd_erase_block;
    info->program 		= (void *)flash_amd_program;
    info->block_size 	= flash_dev_info->block_size;
    info->blocks 		= flash_dev_info->block_count;
    info->start 		= (void *)FLASH_BASE;
    info->end 			= (void *)(FLASH_BASE+ (flash_dev_info->device_size));
    info->vendor		= vendor;
    info->chip_id		= device_id;
    info->sub_id1       = sub_id1;
    info->sub_id2       = sub_id2;
    
    return FLASH_ERR_OK;
}

/*----------------------------------------------------------------------
* 	flash_amd_erase_block
*----------------------------------------------------------------------*/
static int flash_amd_erase_block(void* block, unsigned int size)
{
    volatile FLASH_DATA_T* ROM, *BANK;
    volatile FLASH_DATA_T* b_p = (FLASH_DATA_T*) (block);
    volatile FLASH_DATA_T *b_v;
    volatile FLASH_DATA_T *f_s0, *f_s1, *f_s2;
    int timeout = 50000;
    int len, len_ix = 1;
    int res = FLASH_ERR_OK;
    FLASH_DATA_T state;
    int bootblock;
    unsigned long bank_offset;

    BANK = ROM = (volatile FLASH_DATA_T*)((unsigned long)block & flash_dev_info->base_mask);

    if (flash_dev_info->banked) {
        int b = 0;
        bank_offset = (unsigned long)block & ~(flash_dev_info->block_size-1);
        bank_offset -= (unsigned long) ROM;
        for(;;) {
            if (bank_offset >= flash_dev_info->banks[b]) {
                BANK = (volatile FLASH_DATA_T*) ((unsigned long)ROM + flash_dev_info->banks[b]);
                break;
            }
            b++;
        }
    }

    f_s0 = FLASH_P2V(BANK);
    f_s1 = FLASH_P2V(BANK + FLASH_SETUP_ADDR1);
    f_s2 = FLASH_P2V(BANK + FLASH_SETUP_ADDR2);

    bootblock = (flash_dev_info->bootblock &&
                 (flash_dev_info->bootblocks[0] == ((unsigned long)block - (unsigned long)ROM)));
    if (bootblock) {
        len = flash_dev_info->bootblocks[len_ix++];
    } else {
        len = flash_dev_info->block_size;
    }
    
    while (len > 0) 
    {
        if (flash_dev_info->wp_addr)
        {
        	*f_s1 = FLASH_SETUP_CODE1;
        	hal_delay_us(3);
        	*f_s2 = FLASH_SETUP_CODE2;
        	hal_delay_us(3);
        	*f_s1 = FLASH_WP_STATE;
        	hal_delay_us(3);
        	state = *FLASH_P2V(b_p+flash_dev_info->wp_addr);
        	*f_s0 = FLASH_RESET;
    		hal_delay_us(10);

        	if (FLASH_UNLOCKED != state)
        	{
        		*FLASH_P2V(ROM+FLASH_RESET_ADDR) = FLASH_RESET;
            	return FLASH_ERR_PROTECT;
        	}
		}
        b_v = FLASH_P2V(b_p);
    
        *f_s1 = FLASH_SETUP_CODE1;
    	hal_delay_us(8);
        *f_s2 = FLASH_SETUP_CODE2;
    	hal_delay_us(8);
        *f_s1 = FLASH_SETUP_ERASE;
    	hal_delay_us(8);
        *f_s1 = FLASH_SETUP_CODE1;
    	hal_delay_us(8);
        *f_s2 = FLASH_SETUP_CODE2;
    	hal_delay_us(8);
        *b_v  = FLASH_BLOCK_ERASE;
    	hal_delay_us(50);

        timeout = FLASH_POLLING_TIMEOUT;    
        while (TRUE) {
            state = *b_v;
            if ((state & FLASH_SECTOR_ERASE_TIMER)
				== FLASH_SECTOR_ERASE_TIMER) break;
			hal_delay_us(1);
            if (--timeout == 0) {
                res = FLASH_ERR_DRV_TIMEOUT;
                break;
            }
        }

        if (FLASH_ERR_OK == res) {
            timeout = FLASH_POLLING_TIMEOUT;
            while (TRUE) {
                state = *b_v;
                if (FLASH_BLANKVALUE == state) {
                    break;
                }
				hal_delay_us(1);
                if (--timeout == 0) {
                    res = FLASH_ERR_DRV_TIMEOUT;
                    break;
                }
            }
        }

        if (FLASH_ERR_OK != res)
        {
            *FLASH_P2V(ROM+FLASH_RESET_ADDR) = FLASH_RESET;
			return res;
        }

        while (len > 0) {
            b_v = FLASH_P2V(b_p++);
            if (*b_v != FLASH_BLANKVALUE) {
                if (FLASH_ERR_OK == res) res = FLASH_ERR_DRV_VERIFY;
                return res;
            }
            len -= sizeof(*b_p);
        }

        if (bootblock)
            len = flash_dev_info->bootblocks[len_ix++];
    }
    return res;
}

/*----------------------------------------------------------------------
* 	flash_amd_program
*----------------------------------------------------------------------*/
static int flash_amd_program(void* addr, void* data, int len)
{
    volatile FLASH_DATA_T* ROM;
    volatile FLASH_DATA_T* BANK;
    volatile FLASH_DATA_T* data_ptr = (volatile FLASH_DATA_T*) data;
    volatile FLASH_DATA_T* addr_v;
    volatile FLASH_DATA_T* addr_p = (FLASH_DATA_T*) (addr);
    volatile FLASH_DATA_T *f_s1, *f_s2;
    unsigned long   bank_offset;
    int timeout;
    int res = FLASH_ERR_OK;

    if ((unsigned long)addr & (FLASH_WIDTH / 8 - 1))
        return FLASH_ERR_INVALID;

    BANK = ROM = (volatile FLASH_DATA_T*)((unsigned long)addr_p & flash_dev_info->base_mask);

    if (flash_dev_info->banked) {
        int b = 0;
        bank_offset = (unsigned long)addr & ~(flash_dev_info->block_size-1);
        bank_offset -= (unsigned long) ROM;
        for(;;) {
            if (bank_offset >= flash_dev_info->banks[b]) {
                BANK = (volatile FLASH_DATA_T*) ((unsigned long)ROM + flash_dev_info->banks[b]);
                break;
            }
            b++;
        }
    }

    f_s1 = FLASH_P2V(BANK + FLASH_SETUP_ADDR1);
    f_s2 = FLASH_P2V(BANK + FLASH_SETUP_ADDR2);

    while (len > 0) {
        int state;

        addr_v = FLASH_P2V(addr_p++);

        *f_s1 = FLASH_SETUP_CODE1;
    	hal_delay_us(2);
        *f_s2 = FLASH_SETUP_CODE2;
    	hal_delay_us(2);
        *f_s1 = FLASH_PROGRAM;
    	hal_delay_us(2);
        *addr_v = *data_ptr;
		hal_delay_us(10);

        timeout = FLASH_POLLING_TIMEOUT;
        while (TRUE) {
            state = *addr_v;
            if (*data_ptr == state) {
                break;
            }
			hal_delay_us(1);
            if (--timeout == 0) {
                res = FLASH_ERR_DRV_TIMEOUT;
                break;
            }
        }

        if (FLASH_ERR_OK != res)
        {
            *FLASH_P2V(ROM+FLASH_RESET_ADDR) = FLASH_RESET;
           	return res;
        }

        if (*addr_v != *data_ptr++) {
            if (FLASH_ERR_OK == res) res = FLASH_ERR_DRV_VERIFY;
            break;
        }
        len -= sizeof(*data_ptr);
    }

    return res;
}

//#endif	// FLASH_TYPE_PARALLEL	
