#ifndef _FLASH_M25P80_H
#define _FLASH_M25P80_H


#define PAGE_SIZE  0x100
#define SECTOR_SIZE  0x10000
#define FLASH_BASE 				(SL2312_FLASH_SHADOW)

#define BULK_ERASE                                      1
#define SECTOR_ERASE                                    2
#define SECTOR_SIZE                                     0x10000
#define M25P20_TEST_START                               0
//#define M25P20_TEST_END                                 0x200
#define M25P20_TEST_END                                 0x2000

#define M25P20_WRITE_ENABLE                  		0x06
#define M25P20_WRITE_DISABLE                 		0x04
#define M25P20_READ_STATUS                   		0x05
#define M25P20_WRITE_STATUS              		0x01
#define M25P20_READ                      		0x03
#define M25P20_FAST_READ                 		0x0B
#define M25P20_PAGE_PROGRAM              		0x02 
#define M25P20_SECTOR_ERASE              		0xD8 
#define M25P20_BULK_ERASE                		0xC7 

#define SERIAL_FLASH_CHIP0_EN            0x00000000  // 16th bit = 0 

#define	FLASH_ACCESS_OFFSET	        0x00000010
#define	FLASH_ADDRESS_OFFSET            0x00000014
#define	FLASH_WRITE_DATA_OFFSET         0x00000018
#define	FLASH_READ_DATA_OFFSET          0x00000018
#define	FLASH_TIMING_OFFSET             0x0000001c


#define SERIAL_FLASH_CHIP1_EN            0x00010000  // 16th bit = 1


#define FLASH_ACCESS_ACTION_OPCODE                	0x0000
#define FLASH_ACCESS_ACTION_OPCODE_DATA           	0x0100
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS         	0x0200
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_DATA    	0x0300
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_X_DATA  	0x0600
#define FLASH_ACCESS_ACTION_SHIFT_ADDRESS_4X_DATA 	0x0700


//void m25p20_erase(UINT8,UINT32);
//void m25p20_write_cmd(UINT8,UINT32);
//void m25p20_read_status(UINT8 *,UINT32);
//void m25p20_write_status(UINT8,UINT32);
//void m25p20_read(UINT32, UINT8 *,UINT32);
//void m25p20_page_program(UINT32, UINT8,UINT32);
//void m25p20_sector_erase(UINT32, UINT32);
//void m25p20_bulk_erase(UINT32);
//void m25p20_bulk_erase(UINT32);


#endif
