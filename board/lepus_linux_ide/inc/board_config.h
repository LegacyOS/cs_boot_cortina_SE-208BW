/****************************************************************************
 * Copyright  Storm Corp 2005.  All rights reserved.                
 *--------------------------------------------------------------------------
 * Name			: board_config.h
 * Description	: 
 *		Define board-dependent configuration, including
 *		software configuration:
 *			(1) default MAC address,....
 *		hardware setting,
 *			(1) Clock rate
 *			(2) DRAM size and memory map
 *			(3) Flash Size and memory map
 *
 * History
 *
 *	Date		Writer		Description
 *	-----------	-----------	-------------------------------------------------
 *	04/18/2005	Gary Chen	Create
 *
 ****************************************************************************/
#ifndef _BOARD_CONFIG_H
#define _BOARD_CONFIG_H

// #include "sl2312.h"

// software configuration
#define BOARD_BOOT_LOADER_NAME			"Cortina CS35xx Boot Loader [Linux]"
#define BOARD_BUILD_TOOL				"linux"
#define BOARD_COMPANY_NAME				"Cortina"
#define BOARD_MODEL_NAME				"CS35xx"
#define BOARD_CLI_PROMPT				"cs-boot>"
#define BOARD_UART_BAUD_RATE			19200
#define BOARD_BOOT_TIMEOUT				3	// wait time-out interval 
											// to check break (ctrl-c) key
#define BOARD_DEFAULT_MAC0_ADDR			{0x00,0x50,0xc2,0x11,0x11,0x11}
#define BOARD_DEFAULT_MAC1_ADDR			{0x00,0x50,0xc2,0x22,0x22,0x22}
#define BOARD_DEFAULT_IP_ADDR			{192,168,0,200}
#define BOARD_DEFAULT_IP_NETMASK		{255,255,255,0}
#define BOARD_DEFAULT_GATEWAY			{192,168,0,254}

//bootp
#define BOARD_BOOTS_IP_ADDR			{255,255,255,255}
#define BOOT_VERSION	" "
// Hardware setting
////////GEMINI_16BIT
//#define BOARD_DRAM_SIZE					128 * 1024 * 1024 //128M
//#define BOARD_DRAM_SIZE					64 * 1024 * 1024 //64M
//#define BOARD_DRAM_SIZE					32 * 1024 * 1024 //32M
////////endGEMINI_16BIT
#define BOARD_FLASH_SIZE				(512 * 1024)//(8 * 1024 * 1024)
#define BOARD_CLOCK_RATE				100000000
#define BOARD_TICKS_PER_SECOND			100
#define BOARD_SYS_TIMER_UNIT			10		// 10 ms
#define BOARD_TPS						BOARD_TICKS_PER_SECOND
#define BOARD_BIG_ENDIAN				0
#define BOARD_LITTLE_ENDIAN				1
#define BOARD_ENDIAN					BOARD_LITTLE_ENDIAN

// Hardware configuration
#define PWR_CIR_CTL				1
#define SYS_MAC_NUM						2
//#define GEMINI_ASIC                     1
#define MIDWAY							1	// Gemini
//#define LEPUS_FPGA						1
//#define GEMINI_16BIT							1	// 16-bit
////////////////////
//ifdef GEMINI_16BIT then BOARD_DRAM_SIZE must be modify 
//and sl2312.h must modify & boot2.ld 64MB:0x3E00000 ; 32MB:0x1E00000
//
////////////////////
#define LEPUS_ASIC   						1	// LEPUS
//#define LPC_IT8712					0
//#define CONFIG_ADM_6996				1
//#define CONFIG_ADM_6999				0
#define BOARD_SUPPORT_VCTL				1

//#define BOARD_SUPPORT_TWO_CPU                   1			
#define CPU2_BOOT_OFFSET			  0x1600000

#ifdef BOARD_SUPPORT_TWO_CPU
#define CPU2_RAM_SIZE                           32
#define CPU2_RAM_BASE                           (96<<20)
#define CPU2_FLASH_LOCATION                       0x7D00000
#define CPU2_RAM_LOCATION          (JPEG_RAM_LOCATION + 0x40)
#define JPEG_MAX_SIZE                           (3 * 1024 * 1024)
#define MEM_MAP_CPU1_TO_CPU2(x)         (x - CPU2_RAM_BASE)
#define BOARD_DRAM_CPU2_ADDR		CPU2_RAM_BASE
#define BOARD_DRAM_SIZE					128 * 1024 * 1024 //128M
#define BOARD_BOOT2_MALLOC_SIZE			((32+64) * 1024 * 1024)
#else
	#define BOARD_DRAM_SIZE					64 * 1024 * 1024 //128M
	#define BOARD_BOOT2_MALLOC_SIZE			(32 * 1024 * 1024)
#endif

//boot from flash
//#define BOARD_SUPPORT_FIS				1

//boot from ide
#define LOAD_FROM_IDE					1

// Note: To support the IDE, the BOARD_SUPPORT_IDE0 && BOARD_SUPPORT_IDE1 must be defined
//       with value 0 or 1, otherwise only support IDE1 for backward compatiable isuue

#define BOARD_SUPPORT_IDE				1	
#define BOARD_SUPPORT_IDE0				1	// support IDE-0
#define BOARD_SUPPORT_IDE1				1	// not support IDE-1

#define SATA_LED              1
#ifdef SATA_LED
#define GPIO_SATA0_LED       14
#endif

#ifdef BOARD_SUPPORT_WEB
	#define BOARD_DEFAULT_WEB_IP_ADDR			{192,168,0,168}
	#define BOARD_FLASH_BLOCK_SIZE			(2 * 64 * 1024)
	#define BOARD_WEB_TIMEOUT				3	// wait time-out interval 
#endif

#ifndef BOARD_SUPPORT_TWO_CPU
#define USB_DEV_MO					1	// USB Device mode
#define USB_SPEED_UP		1
//#define BOARD_SUPPORT_RAID				1	// Support DAS-RAID  //boot from hd do not use raid
#define CHECK_PATTERN1		0x91A2C3D4
#define CHECK_PATTERN2		0x4D3C2A19
#define SW_PIPE_SIZE		8192
#define RECOVER_FROM_BOOTP				1

#endif
// Flash memory Map
//		+--------------------------+ ---------->+-----------------------+
//		|	   BOOT-1 Program      |			|		BOOT-1 Program	|
//      |             +            |			|		(2K)			|
//		|      BOOT-2 Program      |     		+-----------------------+
//      |   	               	   |	 		|		BOOT-2 Program	|
//      |        (128K)            |	        |		(126K)			|
//		+--------------------------+----------->+-----------------------+
//      |  		   KERNEL    	   |
//      |         1M Bytes         |
//		+--------------------------+
//      |         RAM DISK         |
//      |         2M Bytes         |
//		+--------------------------+
//      |     			       	   |
//      |                          |
//		+--------------------------+

#define BOARD_FLASH_BASE_ADDR			0x30000000
#define BOARD_FLASH_BOOT_ADDR			BOARD_FLASH_BASE_ADDR
#ifndef BOARD_SUPPORT_TWO_CPU
#define BOARD_FLASH_BOOT_SIZE			(2 * 64 * 1024)	// include BOOT-1 & BOOT-2
#else
	#define BOARD_FLASH_BOOT_SIZE			(4 * 64 * 1024)	// include BOOT-1 & BOOT-2
#endif
//#define BOARD_FLASH_BOOT1_ADDR			BOARD_FLASH_BOOT_ADDR
//#define BOARD_FLASH_BOOT1_SIZE			(2 * 1024)	
//#define BOARD_FLASH_BOOT2_ADDR			(BOARD_FLASH_BOOT1_ADDR + BOARD_FLASH_BOOT1_SIZE)
//#define BOARD_FLASH_BOOT2_SIZE			(BOARD_FLASH_BOOT_SIZE - BOARD_FLASH_BOOT1_SIZE)
//////2.6.10
////#define BOARD_FLASH_KERNEL_ADDR			(BOARD_FLASH_BASE_ADDR + 0x20000)
////#define BOARD_KERNEL_SIZE				(0x00180000)
////#define BOARD_FLASH_RAM_DISK_ADDR		(BOARD_FLASH_BASE_ADDR + 0x1A0000)
////#define BOARD_RAM_DISK_SIZE				(0x00200000)
////#define BOARD_FLASH_APPS_ADDR			(BOARD_FLASH_BASE_ADDR + 0x3A0000)
////#define BOARD_APPS_SIZE					(0x00420000)
////2.6.15 flashmap chamge
//#define BOARD_FLASH_KERNEL_ADDR			(BOARD_FLASH_BASE_ADDR + 0x20000)
//#define BOARD_KERNEL_SIZE				(0x00200000)
//#define BOARD_FLASH_RAM_DISK_ADDR		(BOARD_FLASH_BASE_ADDR + 0x220000)
//#define BOARD_RAM_DISK_SIZE				(0x00280000)
//#define BOARD_FLASH_APPS_ADDR			(BOARD_FLASH_BASE_ADDR + 0x4A0000)
//#define BOARD_APPS_SIZE					(0x00320000)
////
////
//#define BOARD_FLASH_CONFIG_ADDR			(BOARD_FLASH_BASE_ADDR + 0x7D0000)
//#define BOARD_FLASH_CONFIG_SIZE			(64 * 1024)
//#define BOARD_FLASH_VCTL_ADDR			(BOARD_FLASH_BASE_ADDR + 0x7C0000)
//#define BOARD_FLASH_VCTL_SIZE			(64 * 1024)
//#define BOARD_FLASH_FIS_ADDR			(BOARD_FLASH_BASE_ADDR + 0x7F0000)
//#define BOARD_FLASH_FIS_SIZE			(1 * 64 * 1024)

//IDE boot
#define BOARD_FLASH_BOOT1_ADDR			BOARD_FLASH_BOOT_ADDR
#define BOARD_FLASH_BOOT1_SIZE			(2 * 1024)		
#define BOARD_FLASH_BOOT2_ADDR			(BOARD_FLASH_BOOT1_ADDR + BOARD_FLASH_BOOT1_SIZE)
#define BOARD_FLASH_BOOT2_SIZE			(BOARD_FLASH_BOOT_SIZE - BOARD_FLASH_BOOT1_SIZE)
#define BOARD_FLASH_VCTL_ADDR			(BOARD_FLASH_BASE_ADDR + 0x70000)
#define BOARD_FLASH_VCTL_SIZE			(2 * 64 * 1024)

#ifdef BOARD_SUPPORT_TWO_CPU
#define BOARD_FLASH_CPU2_ADDR			(BOARD_FLASH_BASE_ADDR + 0xB40000)
#define BOARD_CPU2_SIZE				0x180000
#define BOARD_FLASH_CPU2_RD_ADDR		(BOARD_FLASH_BASE_ADDR + 0xCC0000)
#define BOARD_CPU2_RD_SIZE			0x300000
#endif

#define BOARD_FIS_BOOT_NAME				"BOOT"
#define BOARD_FIS_DIRECTORY_NAME		"FIS directory"
#define BOARD_FIS_KERNEL_NAME			"Kernel"
#define BOARD_FIS_RAM_DISK_NAME			"Ramdisk"
#define BOARD_FIS_APPS_NAME				"Application"
#define BOARD_FIS_CONFIG_NAME			"CurConf"
#define BOARD_FIS_VCTL_NAME				"VCTL"
#define BOARD_FIS_CPU2_NAME			"CPU2"

#ifdef BOARD_SUPPORT_TWO_CPU
#define BOARD_FIS_CPU2_NAME			"Kernel2"
#define BOARD_FIS_RAM_DISK2_NAME		"Ramdisk2"
#endif

// DRAM memory Map
///		+--------------------------+
//      |  		             	   |
//      |                          |
//		+--------------------------+
//		|	  		BOOT2          |  The location could not use 0
//      |     memory allocation    |  The size must be more large then kernel size
//		|           area           |  
//      |   	                   |
//      |                          |
//		+--------------------------+
//      |          BOOT2           | 
//      |       code + data        |
//		|                          |
//      |     			       	   |
//      |                          |
//		+--------------------------+
//      
#define BOARD_DRAM_BASE_ADDR			0x0
#define BOARD_DRAM_BOOT2_ADDR			(BOARD_DRAM_BASE_ADDR + BOARD_DRAM_SIZE - 0x200000)
#define BOARD_DRAM_KERNEL_ADDR			(BOARD_DRAM_BASE_ADDR + 0x01600000)
#define BOARD_DRAM_KERNEL_START_ADDR	(BOARD_DRAM_KERNEL_ADDR + 0x00)
#define BOARD_DRAM_RAM_DISK_ADDR		(BOARD_DRAM_BASE_ADDR + 0x00800000)
#ifdef BOARD_SUPPORT_TWO_CPU
#define BOARD_DRAM_KERNEL2_ADDR			(CPU2_RAM_BASE + CPU2_BOOT_OFFSET)
#define BOARD_DRAM_RAM_DISK2_ADDR		(CPU2_RAM_BASE + BOARD_DRAM_RAM_DISK_ADDR)
#endif

#define KERNEL_FILENAME					"zImage"
#define INITRD_FILENAME					"rd.gz"
#define KERNEL1_FILENAME					"zImage2"
#define INITRD1_FILENAME					"rd2.gz"

//#define BOARD_BOOT2_MALLOC_SIZE			(32 * 1024 * 1024)
#define BOARD_BOOT2_MALLOC_BASE			(BOARD_DRAM_BOOT2_ADDR - BOARD_BOOT2_MALLOC_SIZE)


//#define	FLASH_TYPE_NAND	1
//#define FLASH_TYPE_PARALLEL	1
//#define  FLASH_TYPE_SERIAL	 1
//#define  FLASH_SERIAL_STM	 1

// define supported flash chip
#define FLASH_SUPPORT_MX29LV400			1
#define FLASH_SUPPORT_MX29LV800			1
#define FLASH_SUPPORT_S29DL800			1
#define FLASH_SUPPORT_AM29DL400			1
#define FLASH_SUPPORT_S29AL004D			1
#define FLASH_SUPPORT_AM29LV640M		1
#define FLASH_SUPPORT_MX29LV640BT		1
#define FLASH_SUPPORT_MX29LV640BB		1
#define FLASH_SUPPORT_AMIC_AM29LV640MB	1

//define flash type of boot  
#define	FLASH_TYPE		        	0x0000000C
#define	NFLASH_ACCESS		        0x00000030
#define N2KPAGE						0x840
#define N2KDATA						0x800
#define NPAGE						0x210
#define NDATA						0x200


#endif // _BOARD_CONFIG_H


