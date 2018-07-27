
//#include <pkgconf/hal.h>
//#include <cyg/hal/hal_arch.h>
//#include <cyg/hal/hal_cache.h>
//#include CYGHWR_MEMORY_LAYOUT_H
//
//#define  _FLASH_PRIVATE_
//#include <cyg/io/flash.h>

#include <define.h>
#include <board_config.h>
//#if defined(FLASH_TYPE_SERIAL) && !defined(FLASH_SERIAL_STM)

#include <sl2312.h>
#include "flash_drv.h"

#define FLASH_INTERLEAVE (1)
#define FLASH_SERIES     (1)
#define FLASH_BASE       (SL2312_FLASH_SHADOW)
// Platform code must define the below
// #define FLASH_INTERLEAVE      : Number of interleaved devices (in parallel)
// #define FLASH_SERIES          : Number of devices in series
// #define FLASH_BASE            : Address of first device
// And select one of the below device variants

//#ifdef CYGPKG_DEVS_FLASH_ATMEL_AT45DB321B
#define PAGE_SIZE			0x200		// 512 bytes
#define FLASH_BLOCK_SIZE               (PAGE_SIZE * 8)	// 4096 bytes
#define FLASH_NUM_REGIONS              (0x400)		// 1024 blocks
#define FLASH_BASE_MASK         (0xFFFFF000u)
#define FLASH_WIDTH             (16)
#define FLASH_BLANK             (1)
#define FLASH_ID_MANUFACTURER   FLASHWORD(0x1F)
#define FLASH_ID_DEVICE         FLASHWORD(0x26)
//#endif

//---> Add by Jason
#define SERIAL_FLASH_OK      		0
#define SERIAL_FLASH_FAIL     		1
#define BLOCK_ERASE                     0x50
#define BUFFER1_READ                    0x54
#define BUFFER2_READ                    0x56
#define PAGE_ERASE                      0x81
#define MAIN_MEMORY_PAGE_READ           0x52
#define MAIN_MEMORY_PROGRAM_BUFFER1     0x82
#define MAIN_MEMORY_PROGRAM_BUFFER2     0x85
#define BUFFER1_TO_MAIN_MEMORY          0x83
#define BUFFER2_TO_MAIN_MEMORY          0x86
#define BUFFER1_TO_MAIN_MEMORY_ERASED   0x88
#define BUFFER2_TO_MAIN_MEMORY_ERASED   0x89
#define MAIN_MEMORY_TO_BUFFER1          0x53
#define MAIN_MEMORY_TO_BUFFER2          0x55
#define BUFFER1_WRITE                   0x84
#define BUFFER2_WRITE                   0x87
#define AUTO_PAGE_REWRITE_BUFFER1       0x58
#define AUTO_PAGE_REWRITE_BUFFER2       0x59
#define READ_STATUS                     0x57

#define MAIN_MEMORY_PAGE_READ_SPI       0xD2
#define BUFFER1_READ_SPI                0xD4
#define BUFFER2_READ_SPI                0xD6
#define READ_STATUS_SPI                 0xD7


#define	SFLASH_ACCESS_OFFSET	        0x00000010
#define	SFLASH_ADDRESS_OFFSET            0x00000014
#define	SFLASH_WRITE_DATA_OFFSET         0x00000018
#define	SFLASH_READ_DATA_OFFSET          0x00000018
#define	SFLASH_TIMING_OFFSET             0x0000001c

#define SERIAL_FLASH_CHIP1_EN            0x00010000  // 16th bit = 1
#define SERIAL_FLASH_CHIP0_EN            0x00000000  // 16th bit = 0 
#define AT45DB321		         0x0  
#define AT45DB642		         0x1  
#define CONTINUOUS_MODE		         0x00008000  

#define FLASH_ACCESS_ACTION_OPCODE                        0x0000
#define FLASH_ACCESS_ACTION_OPCODE_DATA                   0x0100
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS                 0x0200
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_DATA            0x0300

#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_X_DATA          0x0400
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_2X_DATA         0x0500
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_3X_DATA         0x0600
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_4X_DATA         0x0700
//<---

#define FLASH_DEVICE_SIZE               (FLASH_BLOCK_SIZE*FLASH_NUM_REGIONS)
#define FLASH_DEVICES            (FLASH_INTERLEAVE*FLASH_SERIES)

//----------------------------------------------------------------------------
// Now that device properties are defined, include magic for defining
// accessor type and constants.
//#include <cyg/io/flash_dev.h>
unsigned int sachip_en,samode,sacont,sachip;//,era_mode;
unsigned int sector_0_byte,sector_1_byte,sector_n_byte,sector_0_block;
unsigned int sector_1_block,sector_n_block,sector_0_page,sector_1_page;
unsigned int sector_n_page,block_page,page_size,page_addr,dpage_size,total_page;
//----------------------------------------------------------------------------
// Functions that put the flash device into non-read mode must reside
// in RAM.
void flash_query(void* data);
static int  sflash_erase_block(void* block, unsigned int size, unsigned int chip_en) ;
static int  sflash_program_buf(void* addr, void* data, int len, unsigned int chip_en);
//int  flash_read_buf(void* addr, void* data, int len);

void flash_delay(void);
unsigned char read_status(unsigned char cmd, unsigned char *data, unsigned int chip_en);
extern void FLASH_CTRL_WRITE_REG(unsigned int addr,unsigned int data);
extern unsigned int FLASH_CTRL_READ_REG(unsigned int addr);
unsigned char buffer_write(unsigned char cmd, unsigned short offset, unsigned char data, unsigned int chip_en);
unsigned char buffer_to_main_memory(unsigned char cmd, unsigned short page, unsigned int chip_en);
unsigned char address_to_page(unsigned int address, unsigned short *page, unsigned short *offset);


//----------------------------------------------------------------------------
// Initialize driver details
int flash_stm_init(FLASH_INFO_T *info)
{
    unsigned long *add; 

   // add = 0x54000010 ;		//Add by jason for flash access enable.
#if 1
					sachip_en = SERIAL_FLASH_CHIP0_EN;
    				sachip = AT45DB321;
    				sector_0_byte = 4224;
				sector_1_byte = 266112;   
				sector_n_byte = 270336;
				sector_0_block = 1;
				sector_1_block = 63;
				sector_n_block = 64;
				sector_0_page = 8;
				sector_1_page = 504;
				sector_n_page = 512;
				block_page = 8;
				page_size = 528;
				dpage_size = 512;
				page_addr = 10;
				total_page = 8192;
#else
    				sachip = AT45DB642;
    				sector_0_byte = 8448;
				sector_1_byte = 261888;   
				sector_n_byte = 270336;
				sector_0_block = 1;
				sector_1_block = 31;
				sector_n_block = 32;
				sector_0_page = 8;
				sector_1_page = 248;
				sector_n_page = 256;
				block_page = 8;
				page_size = 1056;
				dpage_size = 1024;
				page_addr = 11;
				total_page = 8192;
#endif
   // *add = 0x4000 ;		//Add by jason for flash access enable.
    FLASH_CTRL_WRITE_REG(SFLASH_ACCESS_OFFSET,0x4000);

    // flash_info.block_size = block_page * dpage_size ; //FLASH_BLOCK_SIZE;
    //flash_info.blocks = total_page / block_page;//FLASH_NUM_REGIONS;
    //flash_info.start = (void *)FLASH_BASE;
    //flash_info.end = (void *)(FLASH_BASE + total_page * dpage_size);//(void *)(FLASH_BASE+ (FLASH_NUM_REGIONS * FLASH_BLOCK_SIZE * FLASH_SERIES));
    // Hard wired for now
   	info->erase_block 	= (void *)sflash_erase_block;
    info->program 		= (void *)sflash_program_buf;
    info->block_size 	= block_page * dpage_size ;
    info->blocks 		= total_page / block_page;
    info->start 		= (void *)FLASH_BASE;
    info->end 			= (void *)(FLASH_BASE + total_page * dpage_size);
    //info->vendor		= vendor;
    info->chip_id		= 0x0;
    info->sub_id1       = 0x0;
    info->sub_id2       = 0x0;
    return FLASH_ERR_OK;

}

//----------------------------------------------------------------------------
// Map a hardware status to a package error
static int
flash_hwr_map_error(int e)
{
    return e;
}


//----------------------------------------------------------------------------
// See if a range of FLASH addresses overlaps currently running code

static bool
flash_code_overlaps(void *start, void *end)
{
    extern unsigned char _stext[], _etext[];

    return ((((unsigned long)&_stext >= (unsigned long)start) &&
             ((unsigned long)&_stext < (unsigned long)end)) ||
            (((unsigned long)&_etext >= (unsigned long)start) &&
             ((unsigned long)&_etext < (unsigned long)end)));
}

//----------------------------------------------------------------------------
// Erase Block
static int
sflash_erase_block(void* block, unsigned int len, unsigned int chip_en)
{

	unsigned int opcode;
	unsigned int address;
	unsigned char  status;
	unsigned int block_n=(unsigned int)block ;
	block_n &= 0x3FFFFFF ;
	block_n >>= 12 ;
	
      
 	opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS | BLOCK_ERASE | chip_en;
	address = ((unsigned int)block_n << 13);
	FLASH_CTRL_WRITE_REG(SFLASH_ADDRESS_OFFSET, address);
	FLASH_CTRL_WRITE_REG(SFLASH_ACCESS_OFFSET, opcode);
	opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
	while(opcode&0x80000000)
	{
		opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
		flash_delay();
	}
	read_status(READ_STATUS_SPI, &status, chip_en);
	while(!(status&0x80))
	{
		read_status(READ_STATUS_SPI, &status, chip_en);
		flash_delay();
	}
	      
	return FLASH_ERR_OK;	
	
}

//----------------------------------------------------------------------------
// Program Buffer
static int
sflash_program_buf(void* addr, void* data, int len, unsigned int chip_en)
{


    unsigned char  *pattern,cmd;
    unsigned short page, offset;
    unsigned int i;
    unsigned char *paddr=(unsigned char *)addr;
	  unsigned char *pdata=(unsigned char *)data;	
    unsigned int block_n=(unsigned int)addr ;
    block_n &= 0x3FFFFFF ;

    address_to_page((unsigned int)block_n, &page, &offset);
    
    while(len>0)
    {
        if(page & 0x01)						// Use buffer by turns
            cmd = BUFFER1_WRITE ;
        else
            cmd = BUFFER2_WRITE ;
        
        for(i=0; i<PAGE_SIZE; i++)
        {
           pattern =(unsigned char*)pdata; //data ;
           buffer_write(cmd,i,*pattern, chip_en);				// Write data to flash buffer1/2
           //data++;
           pdata++;
        }
        
        if(page & 0x01)						// Use buffer by turns
            cmd = BUFFER1_TO_MAIN_MEMORY ;
        else
            cmd = BUFFER2_TO_MAIN_MEMORY ;
    
        buffer_to_main_memory(cmd, page, chip_en);			// Write buffer to Main memory
        
        len -= PAGE_SIZE ;
        page++ ;
        
    }
    
    return SERIAL_FLASH_OK;

}


unsigned char address_to_page(unsigned int address, unsigned short *page, unsigned short *offset)
{
      address = address - FLASH_BASE;	 
      *page = address / PAGE_SIZE;
      *offset = address % PAGE_SIZE;
      return SERIAL_FLASH_OK;  	
}

/*
void FLASH_CTRL_WRITE_REG(unsigned int addr,unsigned int data)
{
    unsigned int *base;
    
    base = (unsigned int *)(SL2312_FLASH_CTRL_BASE + addr);
    *base = data;
    return;
}

void flash_delay(void)
{
      int i;
     
      for(i=0; i<50; i++)
           i=i;
}


unsigned int FLASH_CTRL_READ_REG(unsigned int addr)
{
    unsigned int *base;
    unsigned int data;
    
    base = (unsigned int *)(SL2312_FLASH_CTRL_BASE + addr);
    data = *base;
    return (data);
}
*/
unsigned char read_status(unsigned char cmd, unsigned char *data, unsigned int chip_en)
{
      unsigned int opcode;
      unsigned int value;
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_OPCODE_DATA | cmd | chip_en;
      FLASH_CTRL_WRITE_REG(SFLASH_ACCESS_OFFSET, opcode);
      opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
      while(opcode&0x80000000)
      {
          opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
          flash_delay();
      }
      
      value=FLASH_CTRL_READ_REG(SFLASH_READ_DATA_OFFSET);
      *data = value & 0xff;	
	
      return SERIAL_FLASH_OK;	
	
}

unsigned char buffer_write(unsigned char cmd, unsigned short offset, unsigned char data, unsigned int chip_en)
{
      unsigned int opcode;
      unsigned int address;
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS_DATA | cmd | chip_en;
      address = offset;	
      FLASH_CTRL_WRITE_REG(SFLASH_ADDRESS_OFFSET, address);
      FLASH_CTRL_WRITE_REG(SFLASH_WRITE_DATA_OFFSET, data);
      FLASH_CTRL_WRITE_REG(SFLASH_ACCESS_OFFSET, opcode);
      opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
      while(opcode&0x80000000)
      {
          opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
          flash_delay();
      }
      
      return SERIAL_FLASH_OK;	
	
}

unsigned char buffer_to_main_memory(unsigned char cmd, unsigned short page, unsigned int chip_en)
{
      unsigned int opcode;
      unsigned int address;
      unsigned char  status;
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS | cmd | chip_en;
      address = (page << 10);	
      FLASH_CTRL_WRITE_REG(SFLASH_ADDRESS_OFFSET, address);
      FLASH_CTRL_WRITE_REG(SFLASH_ACCESS_OFFSET, opcode);
      opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
      while(opcode&0x80000000)
      {
          opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
          flash_delay();
      }
      read_status(READ_STATUS_SPI, &status, chip_en);
      while(!(status&0x80))
      {
          read_status(READ_STATUS_SPI, &status, chip_en);
          flash_delay();
      }
      	
      return SERIAL_FLASH_OK;	
	
}

unsigned char main_memory_page_read(unsigned char cmd, unsigned short page, unsigned short offset, unsigned char *data, unsigned int chip_en)
{
      unsigned int opcode;
      unsigned int address;
      unsigned int value;
      
      opcode = 0x80000000 | FLASH_ACCESS_ACTION_SHIFT_ADDRESS_4X_DATA | cmd | chip_en;
      address = (page << 10) + offset;	
      FLASH_CTRL_WRITE_REG(SFLASH_ADDRESS_OFFSET, address);
      FLASH_CTRL_WRITE_REG(SFLASH_ACCESS_OFFSET, opcode);
      opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
      while(opcode&0x80000000)
      {
          opcode=FLASH_CTRL_READ_REG(SFLASH_ACCESS_OFFSET);
          flash_delay();
      }

      value=FLASH_CTRL_READ_REG(SFLASH_READ_DATA_OFFSET);
      *data = value & 0xff;	
	
      return SERIAL_FLASH_OK;	
}
//#endif //FLASH_TYPE_SERIAL



