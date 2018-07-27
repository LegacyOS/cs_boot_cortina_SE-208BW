/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: flash_drv.h
* Description	: 
*		Define for flash device driver
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/20/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _FLASH_DRV_H_
#define _FLASH_DRV_H_
/*
//  Bit 12:8 -> This field indicates the type of flash (RO)
//		This field is set by a strapping option.
//		11_1_11 - 16-bit nand flash, page 2KB, 256MB or up
//		11_1_10 - 16-bit nand flash, page 2KB, 128MB
//		11_1_01 - 16-bit nand flash, page 2KB, 64MB
//		11_1_00 - 16-bit nand flash, page 2KB, 32MB
//		11_0_11 - 8-bit nand flash, page 2KB, 256MB or up
//		11_0_10 - 8-bit nand flash, page 2KB, 128MB
//		11_0_01 - 8-bit nand flash, page 2KB, 64MB
//		11_0_00 - 8-bit nand flash, page 2KB, 32MB
//		10_1_11 - 16-bit nand flash, page 512B, 256MB or up
//		10_1_10 - 16-bit nand flash, page 512B, 128MB
//		10_1_01 - 16-bit nand flash, page 512B, 64MB
//		10_1_00 - 16-bit nand flash, page 512B, 32MB
//		10_0_11 - 8-bit nand flash, page 512B, 256MB or up
//		10_0_10 - 8-bit nand flash, page 512B, 128MB
//		10_0_01 - 8-bit nand flash, page 512B, 64MB
//		10_0_00 - 8-bit nand flash, page 512B, 32MB
//		01_1_XX - 16-bit parallel flash device
//		01_0_XX - 8-bit  parallel flash device
//		00_1_11 - serial flash Atmel-compatible 16MB or up
//		00_1_10 - serial flash Atmel-compatible 8MB
//		00_1_01 - serial flash Atmel-compatible 2MB or 4MB
//		00_1_00 - serial flash Atmel-compatible 1MB or less
//		00_0_1X - serial flash STMicroelectronic-compatible 32MB or up
//		00_0_0X - serial flash STMicroelectronic-compatible 16MB or less
//  Bit 7:0  -> Reserved. (RO)
*/
typedef enum {
	FLASH_ERR_OK,					// No error - operation complete
	FLASH_ERR_INVALID,				// Invalid FLASH address
	FLASH_ERR_ERASE,				// Error trying to erase
	FLASH_ERR_LOCK,					// Error trying to lock/unlock
	FLASH_ERR_PROGRAM,				// Error trying to program
	FLASH_ERR_PROTOCOL,				// Generic error
	FLASH_ERR_PROTECT,				// Device/region is write-protected
	FLASH_ERR_NOT_INIT,				// FLASH info not yet initialized
	FLASH_ERR_HWR,					// Hardware (configuration?) problem
	FLASH_ERR_ERASE_SUSPEND,		// Device is in erase suspend mode
	FLASH_ERR_PROGRAM_SUSPEND,		// Device is in in program suspend mode
	FLASH_ERR_DRV_VERIFY,			// Driver failed to verify data
	FLASH_ERR_DRV_TIMEOUT,			// Driver timed out waiting for device
	FLASH_ERR_DRV_WRONG_PART,		// Driver does not support device
	FLASH_ERR_LOW_VOLTAGE,			// Not enough juice to complete job
	FLASH_ERR_MAX
} FLASH_EER_T;

typedef struct {
	unsigned long		init;			// 1: if initialized
	unsigned long		vendor;
	unsigned long		chip_id;
	unsigned long       sub_id1;
	unsigned long       sub_id2;
	unsigned long		block_size;   	// Assuming fixed size "blocks"
	unsigned long		blocks;       	// Number of blocks
	unsigned long		buffer_size;  	// Size of write buffer (only defined for some devices)
	unsigned long		block_mask;
	void				*start;			// Address range
	void				*end;  			// Address range
	int 				(*erase_block)(void *, int);
	int 				(*program)(void *, void *, int);

} FLASH_INFO_T;

extern FLASH_INFO_T flash_info;

int flash_init(void);
int flash_erase(void *addr, int len, unsigned long *err_addr);
int flash_program(void *_addr, void *_data, int len, unsigned long *err_addr);
char *flash_errmsg(int err);
int flash_nand_program(void *_addr, void *_data, int len, unsigned long *err_addr,unsigned long bound);

#endif  // _FLASH_DRV_H_

