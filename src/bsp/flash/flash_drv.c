/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: flash_drv.c
* Description	: 
*		Handle device driver
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
#include <sl2312.h>
#include "flash_drv.h"
#include "flash_nand_parts.h"


extern int flash_amd_init(FLASH_INFO_T *info);
extern int flash_hwr_init(FLASH_INFO_T *info);
extern int flash_sst_init(FLASH_INFO_T *info);
extern int flash_stm_init(FLASH_INFO_T *info);

typedef struct {
	int		(*init)(FLASH_INFO_T *);
} FLASH_INIT_T;
	
FLASH_INFO_T flash_info;
FLASH_INIT_T flash_init_routines[]=
{
	    {flash_amd_init},
#ifdef BOARD_NAND_BOOT	    	
	    {flash_hwr_init},
#endif	    
		{flash_sst_init},	
		{flash_stm_init},	
	

	// {flash_intel_init},	// coming soon in future
	{NULL}
};
/*
FLASH_INIT_T flash_init_routines[]=
{
#ifdef FLASH_TYPE_PARALLEL	
	{flash_amd_init},
#endif	
#ifdef FLASH_TYPE_NAND	
	{flash_hwr_init},
#endif
#ifdef FLASH_TYPE_SERIAL	
	#ifdef FLASH_SERIAL_STM
		{flash_sst_init},	
	#endif
	#ifndef FLASH_SERIAL_STM
		{flash_stm_init},	
	#endif
#endif

	// {flash_intel_init},	// coming soon in future
	{NULL}
};
*/

static const char *flash_err_msg[]=
{
	"OK",									// FLASH_ERR_OK,
	"Invalid FLASH address",                // FLASH_ERR_INVALID,			
	"Error to erase",						// FLASH_ERR_ERASE,			
	"Error to lock/unlock",          		// FLASH_ERR_LOCK,				
	"Error to program",              		// FLASH_ERR_PROGRAM,			
	"Generic error",                        // FLASH_ERR_PROTOCOL,			
	"Device/region is write-protected",     // FLASH_ERR_PROTECT,			
	"FLASH is not yet initialized",       	// FLASH_ERR_NOT_INIT,			
	"Hardware (configuration?) problem",    // FLASH_ERR_HWR,				
	"Device is in erase suspend mode",      // FLASH_ERR_ERASE_SUSPEND,	
	"Device is in in program suspend mode", // FLASH_ERR_PROGRAM_SUSPEND,	
	"Failed to verify data",         		// FLASH_ERR_DRV_VERIFY,		
	"Driver timed out waiting for device",  // FLASH_ERR_DRV_TIMEOUT,		
	"Driver does not support device",       // FLASH_ERR_DRV_WRONG_PART,	
	"Low voltage to complete job"      		// FLASH_ERR_LOW_VOLTAGE,		
};

unsigned int FLASH_CTRL_READ_REG(unsigned int addr)
{
    unsigned int *base;
    unsigned int data;
    
    base = (unsigned int *)(SL2312_FLASH_CTRL_BASE + addr);
    data = *base;
    return (data);
}

void FLASH_CTRL_WRITE_REG(unsigned int addr,unsigned int data)
{
    unsigned int *base;
    
    base = (unsigned int *)(SL2312_FLASH_CTRL_BASE + addr);
    *base = data;
    return;
}

void flash_delay()
{
	int i,j;
	for(i=0;i<0x100;i++)
		j=i*3+5;
}

/*----------------------------------------------------------------------
* 	flash_init
*----------------------------------------------------------------------*/
int flash_init(void)
{
    int err = FLASH_ERR_DRV_WRONG_PART;
   unsigned int    value;
   
    FLASH_INIT_T *init_routine;
    // int test=1;
	
	memset((char *)&flash_info, 0, sizeof(FLASH_INFO_T));
	
	hal_flash_enable();
	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;
	if((value&0x1000)==0x1000)
		init_routine = (FLASH_INIT_T *)&flash_init_routines[1];
	else if((value&0x1800)==0x800)	
		init_routine = (FLASH_INIT_T *)&flash_init_routines[0];
	else if((value&0x1c00)==0x400)	
		init_routine = (FLASH_INIT_T *)&flash_init_routines[3];
	else if((value&0x1c00)==0x000)	
		init_routine = (FLASH_INIT_T *)&flash_init_routines[2];
	else
		init_routine = (FLASH_INIT_T *)&flash_init_routines[0];
	
    while (init_routine->init && (err == FLASH_ERR_DRV_WRONG_PART))
    {
    	// if (test == 1) while(1);
    	if (((err = (*init_routine->init)(&flash_info)) == FLASH_ERR_OK))
    	{
    		break;
    	}
    	init_routine++;
    }
    
    hal_flash_disable();
    
    if (err)
    {
    	printf("Flash init error!\n%d: %s\n", err, flash_err_msg[err]);
    	return err;
    }
    
    flash_info.block_mask = ~(flash_info.block_size-1);
    flash_info.init = 1;
    
    //printf("Flash Vendor = 0x%x, Chip-ID = 0x%x, Sub-ID = 0x%x\n",
    //		flash_info.vendor, flash_info.chip_id, flash_info.sub_id);

    return FLASH_ERR_OK;
}

/*----------------------------------------------------------------------
* 	flash_get_block_info
*----------------------------------------------------------------------*/
void flash_get_block_info(UINT32 *block_size, UINT32 *num_blocks)
{
	*block_size = flash_info.block_size;
	*num_blocks = flash_info.blocks;
}

/*----------------------------------------------------------------------
* 	flash_erase
*----------------------------------------------------------------------*/
int flash_erase(void *addr, int len, unsigned long *err_addr)
{
    unsigned short *block, *end_addr;
    int stat = 0;

	
	
    if (!flash_info.init) 
        return FLASH_ERR_NOT_INIT;

    block = (unsigned short *)((unsigned long)addr & flash_info.block_mask);
    end_addr = (unsigned short *)((unsigned long)addr+len);
	*err_addr = (unsigned long)addr;
	hal_flash_enable();
    while (block < end_addr) {
        int i;
        unsigned char *dp;
        //int erased = TRUE;
        int erased = FALSE;

        dp = (unsigned char *)block;
        //for nand flash R/W issue(page read)
        //for (i = 0;  i < flash_info.block_size;  i++) {
        //    if (*dp++ != (unsigned char)0xFF) {
        //        erased = FALSE;
        //        break;
        //    }
        //}
        if (!erased) {
            stat =flash_info.erase_block(block, flash_info.block_size);
        }
        if (stat) {
            *err_addr = (unsigned long)block;
            break;
        }
        block += flash_info.block_size / sizeof(*block);
        printf(".");
    }
    
    hal_flash_disable();
    
    if (stat)
    {
    	printf("Flash erase error!\n%d: %s\n", stat, flash_err_msg[stat]);
    }
    return (stat);
}

/*----------------------------------------------------------------------
* 	flash_program
*----------------------------------------------------------------------*/
int flash_program(void *_addr, void *_data, int len, unsigned long *err_addr)
{
    int stat = 0;
    int size;
    unsigned char *addr;
    unsigned char *data;
    unsigned long tmp;

	*err_addr = (unsigned long)addr;
	
    if (!flash_info.init) 
        return FLASH_ERR_NOT_INIT;

    addr = (unsigned char *)_addr;
    data = (unsigned char *)_data;

	hal_flash_enable();
    while (len > 0) {
        size = len;
        if (size > flash_info.block_size) size = flash_info.block_size;

        tmp = (unsigned long)addr & ~flash_info.block_mask;
        if (tmp) {
                tmp = flash_info.block_size - tmp;
                if (size>tmp) size = tmp;
        }

        stat = flash_info.program(addr, data, size);

        if (stat) {
            *err_addr = (unsigned long)addr;
            break;
        }
        len -= size;
        addr += size/sizeof(*addr);
        data += size/sizeof(*data);
        printf(".");
    }
   
	hal_flash_disable();
   	
    if (stat )
    {
    	printf("Flash program error!\n%d: %s\n", stat, flash_err_msg[stat]);
    }
    return (stat);
}

/*----------------------------------------------------------------------
* 	flash_drv_errmsg
*----------------------------------------------------------------------*/
#ifdef BOARD_NAND_BOOT
int flash_nand_program(void *_addr, void *_data, int len, unsigned long *err_addr, unsigned long bound)
{
	int stat = 0,i;
    unsigned int size,blocka;
    unsigned char *addr;
    unsigned char *data,*adr,tmp_buf[2112],*obdata;
    unsigned long tmp;


	
	
    if (!flash_info.init) 
        return FLASH_ERR_NOT_INIT;
	
#ifdef BOARD_SUPPORT_NAND_INDIRECT
	addr = (unsigned char *)(_addr-SL2312_FLASH_BASE);
#else
      addr = (unsigned char *)_addr;
#endif	
      data = (unsigned char *)_data;
	*err_addr = (unsigned long)addr;
	bound += addr;

	printf("Program flash (0x%x): Size=%u ", addr, len); 
	
	hal_flash_enable();
    while (len > 0) {
        size = len;
        if (size > flash_info.block_size) size = flash_info.block_size;

        //tmp = (unsigned long)addr & ~flash_info.block_mask;
        //if (tmp) {
        //        tmp = flash_info.block_size - tmp;
        //        if (size>tmp) size = tmp;
        //}
	       
SKIP_BLOCK:		
        	obdata = data;
		blocka = (unsigned int)addr;
	if((blocka &(flash_info.block_size-1))==0)
	{
		while(VeffBad(addr)==FAIL)
		{
			//mark bad block
			addr += flash_info.block_size;
			if(addr>bound)
				goto ERR_OUT;
		}
	}	
		stat =flash_info.erase_block(addr, flash_info.block_size);
        
        if (stat) {
            *err_addr = (unsigned long)addr;
			nand_block_markbad(addr);
			addr += flash_info.block_size;
            goto SKIP_BLOCK;
        }
	//	adr = addr;
       //for(i=0;i<2112;i++)		
	//{			//hal_delay_us(7);			
	//		tmp_buf[i] = *adr++;		
	//}
		
        stat = flash_info.program(addr, data, size);

        if (stat) {
        	 *err_addr = (unsigned long)addr;
        	nand_block_markbad(addr);
	   	addr += flash_info.block_size;
		data = obdata;
            	goto SKIP_BLOCK;
        }
        len -= size;
        addr += size/sizeof(*addr);
        data += size/sizeof(*data);
        printf(".");
    }
	
ERR_OUT:   
	hal_flash_disable();
   	
    if (stat )
    {
    	printf("Flash program error!\n%d: %s\n", stat, flash_err_msg[stat]);
    }
	//printf(" OK!\n"); 
    return (stat);
}


	
#endif

/*----------------------------------------------------------------------
* 	flash_drv_errmsg
*----------------------------------------------------------------------*/
char *flash_errmsg(int err)
{
	if (err >= FLASH_ERR_MAX)
		return "Unknown error!";
	else
		return (char *)flash_err_msg[err];
}
