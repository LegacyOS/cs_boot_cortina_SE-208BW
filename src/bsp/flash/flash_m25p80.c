/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: flash_m25p80.c
* Description	: 
*		Define for flash device driver
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	05/04/2006	Middle	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
//#ifdef FLASH_TYPE_SERIAL	
//#if defined(FLASH_TYPE_SERIAL) && defined(FLASH_SERIAL_STM)

#include <sl2312.h>
#include "flash_drv.h"
#include "flash_m25p80.h"

int schip_en = 0;



static int m25p20_erase(void* block, unsigned int size);
static int m25p20_program(void* addr, void* data, int len);
static int m25p20_page_program(UINT32 address, UINT8 data, UINT32 schip_en);
extern void FLASH_CTRL_WRITE_REG(unsigned int addr,unsigned int data);
extern unsigned int FLASH_CTRL_READ_REG(unsigned int addr);



/* define read/write register utility */
/*
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
  */  


int flash_sst_init(FLASH_INFO_T *info)
{
    unsigned long *add; 

   // add = 0x54000010 ;		//Add by jason for flash access enable.
   // *add = 0x4000 ;		//Add by jason for flash access enable.
    FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET,0x4000);

    // flash_info.block_size = block_page * dpage_size ; //FLASH_BLOCK_SIZE;
    //flash_info.blocks = total_page / block_page;//FLASH_NUM_REGIONS;
    //flash_info.start = (void *)FLASH_BASE;
    //flash_info.end = (void *)(FLASH_BASE + total_page * dpage_size);//(void *)(FLASH_BASE+ (FLASH_NUM_REGIONS * FLASH_BLOCK_SIZE * FLASH_SERIES));
    // Hard wired for now
   	info->erase_block 	= (void *)m25p20_erase; //m25p20_bulk_erase;
    info->program 		= (void *)m25p20_program;
    info->block_size 	= SECTOR_SIZE ; //sector : 0x10000
    info->blocks 		= SECTOR_SIZE; //16 sectors
    info->start 		= (void *)FLASH_BASE;
    info->end 			= (void *)(FLASH_BASE + 0x100000);
    //info->vendor		= vendor;
    info->chip_id		= 0x0;
    info->sub_id1       = 0x0;
    info->sub_id2       = 0x0;
    return FLASH_ERR_OK;

}

static int m25p20_erase(void* block, unsigned int size)
{

     	//UINT32 i, *address;
     	int res = FLASH_ERR_OK;
     	//volatile FLASH_DATA_T* b_p = (FLASH_DATA_T*) (block);
     	unsigned char *address=block;
     	//address = block;
      //  for(i=0; i<16; i++)
        {
        	////middle //middle delay_ms(580);
        	//middle //middle delay_ms(2000);
       	  res = m25p20_sector_erase(address, schip_en);
       	  // address += SECTOR_SIZE;
       	  // printf(".");
       	   
       	}   
		
	return res;
}		

void m25p20_write_cmd(UINT8 cmd, UINT32 schip_en)
{
      UINT32 opcode,tmp;
      UINT32  status;
      
      
      
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE | cmd;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      while(tmp&0x80000000)
      {
          tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
          flash_delay();
      }
      //////
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ_STATUS;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      while(tmp&0x80000000)
      {
          tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
          flash_delay();
      }
      //middle delay_ms(130);
      status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      //printf("\ncmd =%x  status = %x",cmd,status);
      if(cmd==M25P20_WRITE_ENABLE)
      {
      	//printf("\n**-->enable**  status = %x",status);
      	//middle delay_ms(100);
      	   while((status&0x03) != 2)
      	   {
      	   	//if((status&0x9c)!=0)
      	  	//    printf("	M25P20_WRITE_ENABLE   Protect Status = %x\n",status);
      	  	
      	   	  FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	   	  tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    //flash_delay();
      			}
      	       status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      	       //printf("\n**enable**  status = %x",status);
      	       flash_delay();
      	       //middle delay_ms(100);
      	   }
      }
      else if(cmd==M25P20_WRITE_DISABLE)
      {
      	   //while((status&0x03) == 2)
      	   //   printf("\n**disable**  status = %x",status);
      	   //middle delay_ms(100);
      	   while((status&0x03) != 0)
      	   {
	       //m25p20_write_status((status&0xfd),schip_en);      	       
      	       FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	       tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      		while(tmp&0x80000000)
      		{
      		    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      		    flash_delay();
      		}
      	       status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      	       //printf("\n**disable**  status = %x",status);
      	       flash_delay();
      	       //middle delay_ms(50);
      	   }
      }
      else
      {
      	   //while((status&0x01) !=0)
      	   while((status&0x01) !=0)
      	   {
      	   	  FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	   	  tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      	       status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      	       flash_delay();
      	       //middle delay_ms(50);
      	   }
      }
      //////
      
      //printf("\n<--  status = %x",status);
}


void m25p20_read_status(UINT8 *data, UINT32 schip_en)
{
      UINT32 opcode,status;
      UINT32 value;
      UINT32 tmp;
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ_STATUS;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      
      status=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      while(status&0x80000000)
      {
          opcode=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
          flash_delay();
      }
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      while(status!=0xff)
      {
      	  FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	  tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
          status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
          flash_delay();
          //middle delay_ms(50);
      }
      
      value=FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      *data = value & 0xff;	
	
}

void m25p20_write_status(UINT8 data, UINT32 schip_en)
{
      UINT32 opcode,status;
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_WRITE_STATUS;
      FLASH_CTRL_WRITE_REG(FLASH_WRITE_DATA_OFFSET, data);
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      status=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      while(status&0x80000000)
      {
          status=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
          flash_delay();
      }
      
}

void m25p20_read(UINT32 address, UINT8 *data, UINT32 schip_en)
{
      UINT32 opcode,status;
      UINT32 value;
      
      //opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ;
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS_DATA | M25P20_READ;
      FLASH_CTRL_WRITE_REG(FLASH_ADDRESS_OFFSET, address);
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      status=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      while(status&0x80000000)
      {
          status=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
          flash_delay();
      }
      
      value=FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      *data = value & 0xff;	
}

static int m25p20_program(void* addr, void* data, int len)
{
      UINT32 opcode;
      UINT32  status;
      UINT32 tmp, i, j=0 ;
      unsigned char *paddr=(unsigned char *)addr;
	  unsigned char *pdata=(unsigned char *)data;
	if(paddr >= FLASH_BASE)
		paddr-=FLASH_BASE;
		
    int res = FLASH_ERR_OK;
	while (len > 0) {
		
		if(len >= PAGE_SIZE)
			tmp = PAGE_SIZE;
		else
			tmp = len;
		
	    for(i=0;i<tmp;i++)
	    {
	    	res = m25p20_page_program( (paddr+j*PAGE_SIZE+i),  *(pdata+j*PAGE_SIZE+i),  schip_en);
	    }
	    len -= PAGE_SIZE;
	    j++;
	};
	return res;
}

//void m25p20_page_program(UINT32 address, UINT8 data, UINT32 schip_en)
static int m25p20_page_program(UINT32 address, UINT8 data, UINT32 schip_en)
{
      UINT32 opcode;
      UINT32  status;
	  UINT32 tmp, i, j=0 ;
	  int res = FLASH_ERR_OK;
	  //volatile FLASH_DATA_T* data_ptr = (volatile FLASH_DATA_T*) data;
	  opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ_STATUS;
          #ifdef MIDWAY_DIAG
      	      opcode|=schip_en;
          #endif
          FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
          tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
          //middle delay_ms(130);
          status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
          if((status&0x02)==0x02)
      	  {
      	       //middle delay_ms(100);
               m25p20_write_cmd(M25P20_WRITE_DISABLE, schip_en);
          }
      
	
      m25p20_write_cmd(M25P20_WRITE_ENABLE, schip_en);
      ////middle delay_ms(10);
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS_DATA | M25P20_PAGE_PROGRAM;
      FLASH_CTRL_WRITE_REG(FLASH_ADDRESS_OFFSET, address);
      FLASH_CTRL_WRITE_REG(FLASH_WRITE_DATA_OFFSET, data);
      
      //status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      //while(status!=data)
      //{
      //    status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      //    //middle delay_ms(10);
      //}
      
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      //opcode=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ_STATUS;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      //while(status&0xfd)
      while(status&0x01)
      {
      	  //if((status&0x9c)!=0)
      	  //	printf("  m25p20_page_program	Protect Status = %x\n",status);
      	  FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	  tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
          status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
          flash_delay();
          //middle delay_ms(50);
      }
      //printf("status = %x, data = %x\n",status,data);
      if((status&0x02)==0x02)
      {
	  //middle delay_ms(100);      	  
          m25p20_write_cmd(M25P20_WRITE_DISABLE, schip_en);
      }
    //};//while (len > 0)
    return res;
}

static int m25p20_sector_erase(UINT32 address, UINT32 schip_en)
{
      UINT32 opcode;
      UINT32  status;
      UINT32 tmp;
      int res = FLASH_ERR_OK;
	//printf("\n-->m25p20_sector_erase");	
	if(address >= FLASH_BASE)
		address-=FLASH_BASE;
		
      m25p20_write_cmd(M25P20_WRITE_ENABLE, schip_en);
      //printf("\n     m25p20_sector_erase : after we-en");
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS | M25P20_SECTOR_ERASE;
      FLASH_CTRL_WRITE_REG(FLASH_ADDRESS_OFFSET, address);
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      			
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ_STATUS;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      //while(status&0xfd)
      while(status&0x01)
      {
      	  //if((status&0x9c)!=0)
      	  //	printf("  m25p20_sector_erase	Protect Status = %x\n",status);
      	  FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	  tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
          status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
          flash_delay();
          //middle delay_ms(50);
      }
      if((status&0x02)==0x02)
      {
      	  //middle delay_ms(100);
          m25p20_write_cmd(M25P20_WRITE_DISABLE, schip_en);
      }
      //printf("\n<--m25p20_sector_erase");
      return res;
}

void m25p20_bulk_erase(UINT32 schip_en)
{
      UINT32 opcode;
      UINT32  status;
      UINT32 tmp;
	
      m25p20_write_cmd(M25P20_WRITE_ENABLE, schip_en);
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE | M25P20_BULK_ERASE;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      //middle //middle delay_ms(2000);
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | M25P20_READ_STATUS;
      #ifdef MIDWAY_DIAG
      	opcode|=schip_en;
      #endif
      
      FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
      status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
      //while(status&0xfd)

      while(status&0x01)
      {
      	  //if((status&0x9c)!=0)
      	  //	printf("  m25p20_bulk_erase	Protect Status = %x\n",status);
      	  FLASH_CTRL_WRITE_REG(FLASH_ACCESS_OFFSET, opcode);
      	  tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			while(tmp&0x80000000)
      			{
      			    tmp=FLASH_CTRL_READ_REG(FLASH_ACCESS_OFFSET);
      			    flash_delay();
      			}
          status = FLASH_CTRL_READ_REG(FLASH_READ_DATA_OFFSET);
          flash_delay();
          //middle //middle delay_ms(50);
      }
      if((status&0x02)==0x02)
      {
      	  //middle //middle delay_ms(100);
          m25p20_write_cmd(M25P20_WRITE_DISABLE, schip_en);
      }
}

//#endif //FLASH_TYPE_SERIAL



