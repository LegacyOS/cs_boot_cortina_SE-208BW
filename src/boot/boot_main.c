/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: boot_main.c
* Description	: 
*		Main entry of C files for BOOT module
*
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
#include "../../src/bsp/flash/flash_nand_parts.h"

//extern flash_query(void* data) ;
static const flash_dev_info_t* flash_dev_info;

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


void flash_query(UINT8 * data)
{
	UINT8* id = (UINT8*) data;
	UINT32 i, dent_bit;
	UINT8 m_code,d_code,extid=0;
	UINT32 opcode=0;
	UINT32 chip0_en=0,nwidth=0,ndirect=NFLASH_DIRECT,def_width=0;
	//data width
	dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
	if((dent_bit&NFLASH_WiDTH16)==NFLASH_WiDTH16)
		nwidth=NFLASH_WiDTH16;
	else
		nwidth=NFLASH_WiDTH8;
	
	
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x0); 		//set 31b = 0
	if((dent_bit&FLASH_SIZE_MASK)==0)
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000100); 		//set only command & address and two data
	else if((dent_bit&FLASH_SIZE_MASK) == FLASH_SIZE_MASK)
	{
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000400); 		//set only command & address and 4 data
		extid = 3;
	}
	else
	{
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000300); 		//set only command & address and 4 data
		extid = 2;
	}
	
	FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, FLASH_Read_ID); 	//write read id command
	FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, 0x0); 			//write address 0x00
	
	// read maker code
	opcode = FLASH_START_BIT | FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
	opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
	while(opcode&FLASH_START_BIT) //polling flash access 31b
	{
		opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
		//flash_delay();
	}
	if(nwidth == NFLASH_WiDTH8)
	{
		m_code=FLASH_CTRL_READ_REG(NFLASH_DATA);
		//FLASH_CTRL_WRITE_REG(NFLASH_DATA, 0x00000000);
		// read device code 
		opcode = FLASH_START_BIT | FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
		FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
		opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
		while(opcode&FLASH_START_BIT) //polling flash access 31b
		{
			opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
			//flash_delay();
		}
	
		d_code = (FLASH_CTRL_READ_REG(NFLASH_DATA)>>8);
	
		dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
		if((dent_bit&FLASH_SIZE_MASK)>0)
		{
			for(i=0;i<extid;i++)
			{
				//data cycle 3 & 4 ->not use
				opcode = FLASH_START_BIT | FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
				FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
				opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
				while(opcode&FLASH_START_BIT) //polling flash access 31b
				{
					opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
					//flash_delay();
				}
	
				dent_bit=FLASH_CTRL_READ_REG(NFLASH_DATA);
			}
		}
	}
	else	// 16-bit
	{
		dent_bit = FLASH_CTRL_READ_REG(NFLASH_DATA);
		m_code = dent_bit;
		d_code = dent_bit>>8;
	
		//data cycle 3 & 4 ->not use
		dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
		if(((dent_bit&FLASH_SIZE_MASK)>0x0)&&(nwidth == NFLASH_WiDTH16))
		{
			opcode = FLASH_START_BIT | FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
			opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
			while(opcode&FLASH_START_BIT) //polling flash access 31b
			{
				opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
				//flash_delay();
			}
	
			dent_bit=FLASH_CTRL_READ_REG(NFLASH_DATA);
		}
	}
	
	
	
	id[0] = m_code;	// vendor ID
	id[1] = d_code;	// device ID
	opcode = NFLASH_DIRECT;
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS,opcode);

}

/*----------------------------------------------------------------------
*	delay function befort init time module
*----------------------------------------------------------------------*/
void flash_read_delay()
{
	int k,j;
	for(j=0;j<0x2;j++)
		for(k=0;k<0x10;k++)
			k +=j*3+5;
						
}

/*----------------------------------------------------------------------
*	check bad block
*----------------------------------------------------------------------*/
int chk_block(unsigned long *srce, int oobsize, int markp)
{
	unsigned char isbad[2],*oobp;
	unsigned int data,i,j;
	

		for(j=0;j<2;j++) //check first and second page
		{
			//time delay for test board
			flash_read_delay();
			
			for(i=0;i<(oobsize+flash_dev_info->page_size);i++)
			{
				if(i==(flash_dev_info->page_size+markp))
					isbad[0]=*oobp++;
				else
					isbad[1]=*oobp++;
			}	
			
			if(isbad[0]!=0xff)
			{
				return 1;
			}
			oobp = (unsigned char*) ((unsigned char*)srce+flash_dev_info->page_size);
		}
		return 0;
						
}

/*----------------------------------------------------------------------
*	sl_main
*----------------------------------------------------------------------*/
void sl_main(void)
{
	int rc;
    void (*apps_routine)(void);
    unsigned long i, *dest, *srce,  size, noob[4],*tmp,k;//, len=0;
    unsigned int data, oobsize, markp, len;
    unsigned char isbad[2],*oobp,j,*adst,*asrc;
    //unsigned char *ndest, *nsrce, nsize, noob[1];

#ifdef _BOOT_ENABLE_CACHE
    HAL_ICACHE_DISABLE();
	HAL_DCACHE_DISABLE();
	// {int n=1; while(n!=0) n+=2;}; // debug
	hal_mmu_init();
	
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_DCACHE_INVALIDATE_ALL();
    HAL_ICACHE_ENABLE();
    HAL_DCACHE_ENABLE();
		
#endif
	// enable flash
	data = REG32(SL2312_FLASH_CTRL_BASE + FLASH_TYPE);
	if((data&FLASH_TYPE_MASK)==FLASH_TYPE_NAND)//nand flash 2k page
		REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~(GLOBAL_NFLASH_EN_BIT);
	else if((data&FLASH_TYPE_MASK)==FLASH_TYPE_NOR)//nand flash 2k page
		REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~GLOBAL_FLASH_EN_BIT; //~0x00000001;
	else if((data&FLASH_TYPE_MASK)==FLASH_TYPE_SERIAL)//nand flash 2k page
		REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~GLOBAL_SFLASH_EN_BIT; //~0x00000001;

	srce=(unsigned long *)(BOARD_FLASH_BOOT2_ADDR);
    dest=(unsigned long *)BOARD_DRAM_BOOT2_ADDR;
    size = (BOARD_FLASH_BOOT2_SIZE / 4);
	//	len = (BOARD_FLASH_BOOT2_SIZE / 4);
	
#ifndef MIDWAY
    for (i = 0; i < size; i++)
    	*dest++=*srce++;
#else
#ifdef BOARD_NAND_BOOT
	REG32(SL2312_FLASH_CTRL_BASE + NFLASH_ACCESS) &=  NFLASH_DIRECT;
	data = REG32(SL2312_FLASH_CTRL_BASE + FLASH_TYPE);
	
	if(data&FLASH_TYPE_MASK)
	{
		UINT8 id[5];
    	/*get flash ID*/	
    	flash_query(&id[0]);
    	

		// Look through table for device data
    	flash_dev_info = supported_devices;
    	for (i = 0; i < (sizeof(supported_devices)/sizeof(flash_dev_info_t)); i++) {
    	    if (flash_dev_info->device_id == id[1])
    	        break;
    	    flash_dev_info++;
    	}
	}

	/* Try to find boot2 start address, and boot2 start address is at 
	 * next good page of boot1 end page.
	 */
	 
	REG32(SL2312_FLASH_CTRL_BASE + NFLASH_ACCESS) &=  NFLASH_DIRECT;
#endif		
	data = REG32(SL2312_FLASH_CTRL_BASE + FLASH_TYPE);
	
	if((data&FLASH_TYPE_NAND)==FLASH_TYPE_NAND)//nand flash 2k page
	{
		//get boot address; need to get boot1 end and next page is boot2 start address
		//srce=(unsigned long *)(BOARD_FLASH_BOOT2_ADDR);
		srce=(unsigned long *)BOARD_FLASH_BOOT1_ADDR;
		size = BOARD_FLASH_BOOT1_SIZE;
		
		oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
		if(oobsize>PAGE512_OOB_SIZE)
			markp = 0;//first byte
		else
			markp = 5; //6th byte
				
		oobp = (unsigned char *)BOARD_FLASH_BOOT1_ADDR;
		
		
		while(size>0)
		{			
			data = (unsigned int)srce;
			oobp = (unsigned char *)srce;
			
			if((data & (flash_dev_info->block_size-1))==0)
			{
				while(data < BOARD_FLASH_BOOT_SIZE)
				{		
					if(chk_block(srce,  oobsize, markp)==0)
						break;
					else
					{
						srce += (flash_dev_info->block_size/4);
						data = (unsigned int)srce;
					}
				}
			}
						
			if(size > flash_dev_info->block_size)
			{
				srce += (flash_dev_info->block_size/4);
				size -= flash_dev_info->block_size;
			}
			else
			{
				srce += (size/4);
				size -= size;
			}
		}
		
		/* Start to copy boot data to last 2M of dram*/
		
    	//srce=(unsigned long *)(BOARD_FLASH_BOOT2_ADDR);
    	dest=(unsigned long *)BOARD_DRAM_BOOT2_ADDR;
    	size = BOARD_FLASH_BOOT2_SIZE;//(BOARD_FLASH_BOOT2_SIZE / 4);
    	adst = (unsigned char*)dest;
		while(size>0)
		{
			data = (unsigned int)srce;
			oobp = (unsigned char *)srce;
			
			if((data & (flash_dev_info->block_size-1))==0)
			{
				while(data < BOARD_FLASH_BOOT_SIZE)
				{		
					if(chk_block(srce,  oobsize, markp)==0)
						break;
					else
					{
						srce += (flash_dev_info->block_size/4);
						data = (unsigned int)srce;
					}
				}
			}
			asrc = (unsigned char*)srce;
			oobp = asrc;
			flash_read_delay();
			for(i=0;i<((flash_dev_info->page_size+oobsize));i++)
			{

				if(i<(flash_dev_info->page_size))
				{
					if(i < size)
						*adst++=*oobp++;
					else
						isbad[0]=*oobp++;
						
				}
				else
					isbad[0]=*oobp++;
			}
			srce += (flash_dev_info->page_size/4);
			size -= (flash_dev_info->page_size);
		}
	}
	else if((data&FLASH_TYPE_MASK)==FLASH_TYPE_NOR)//parallel flash 
	{
		    	REG32(SL2312_FLASH_CTRL_BASE + NFLASH_ACCESS) &=  0x4000;
    	for (i = 0; i < size; i++)
    		*dest++=*srce++;
	}
	else //serial flash
	{
		//printf("Serial flash not ready!\n");
		for (i = 0; i < size; i++)
    		*dest++=*srce++;
	}
#endif    

#ifdef _BOOT_ENABLE_CACHE    
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_DCACHE_INVALIDATE_ALL();
    HAL_ICACHE_DISABLE();
    HAL_DCACHE_DISABLE();
#endif

	apps_routine = (void (*))(BOARD_DRAM_BOOT2_ADDR);
    apps_routine();
}




