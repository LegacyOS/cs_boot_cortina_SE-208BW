/****************************************************************************
 * Copyright  Cortina systems 2009-2010.  All rights reserved.                *
 *--------------------------------------------------------------------------*
 * Name:board.s                                                             *
 * Description:  Board common define, Define board-dependent configuration, *
 * including common software configuration:                                    *
 * Author: Middle Huang                                                        *
 * Version: 1.4.2 Create
 * 
 ****************************************************************************/
#ifndef _BOARD_COM_H
#define _BOARD_COM_H

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

//#define GEMINI_16BIT							1	// 16-bit
// Hardware setting
////////GEMINI_16BIT
//#define BOARD_DRAM_SIZE					128 * 1024 * 1024 //128M
//#define BOARD_DRAM_SIZE				64 * 1024 * 1024 //64M
////////endGEMINI_16BIT

#define BOARD_CLOCK_RATE				100000000
#define BOARD_TICKS_PER_SECOND			100
#define BOARD_SYS_TIMER_UNIT			10		// 10 ms
#define BOARD_TPS						BOARD_TICKS_PER_SECOND
#define BOARD_BIG_ENDIAN				0
#define BOARD_LITTLE_ENDIAN				1
#define BOARD_ENDIAN					BOARD_LITTLE_ENDIAN

// Hardware configuration
#define SYS_MAC_NUM						2
//#define GEMINI_ASIC                     1
#define MIDWAY							1	// Gemini

//#define LEPUS_FPGA						1
#define LEPUS_ASIC   						1	// LEPUS

//#define LPC_IT8712					0

//#define CONFIG_ADM_6996				1
//#define CONFIG_ADM_6999				0

//#define	NETBSD		1
#define BOOT_VERSION	"1.4.3"

#define PWR_CIR_CTL				1
#define BOARD_SUPPORT_VCTL				1

//#define BOARD_SUPPORT_WEB	1
#ifdef BOARD_SUPPORT_WEB
	#define BOARD_DEFAULT_WEB_IP_ADDR			{192,168,0,168}
	#define BOARD_FLASH_BLOCK_SIZE			(2 * 64 * 1024)
	#define BOARD_WEB_TIMEOUT				3	// wait time-out interval 
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
//      

#define SATA_LED              1
#ifdef SATA_LED
#define GPIO_SATA0_LED       14
#endif

#define BOARD_FLASH_BASE_ADDR			0x30000000
#define BOARD_FLASH_BOOT_ADDR			BOARD_FLASH_BASE_ADDR

#define BOARD_FIS_BOOT_NAME			"BOOT"
#define BOARD_FIS_DIRECTORY_NAME		"FIS directory"
#define BOARD_FIS_KERNEL_NAME			"Kernel"
#define BOARD_FIS_RAM_DISK_NAME			"Ramdisk"
#define BOARD_FIS_APPS_NAME			"Application"

#ifdef BOARD_NAND_BOOT
	#define BOARD_FIS_CONFIG_NAME			"Configure"
	#define BOARD_ECOS_NAME			"eCos"
	#define BOARD_RESV_NAME			"Reserved"
#else
	#define BOARD_FIS_CONFIG_NAME			"CurConf"
#endif	

#define BOARD_FIS_VCTL_NAME			"VCTL"
//#define BOARD_FIS_CPU2_NAME			"CPU2"

#define BOARD_FIS_CPU2_NAME			"Kernel2"
#define BOARD_FIS_RAM_DISK2_NAME		"Ramdisk2"

#define CPU1_BOOT_OFFSET			  0x1C00000
#define CPU2_BOOT_OFFSET			  0x1600000
#define CPU_RD_OFFSET			  0x800000

#define BOARD_DRAM_BASE_ADDR			0x0
#define BOARD_DRAM_BOOT2_ADDR			(BOARD_DRAM_BASE_ADDR + BOARD_DRAM_SIZE - 0x200000)
#define BOARD_DRAM_KERNEL_ADDR			(BOARD_DRAM_BASE_ADDR + CPU1_BOOT_OFFSET)
#define BOARD_DRAM_KERNEL_START_ADDR	(BOARD_DRAM_KERNEL_ADDR + 0x00)
#define BOARD_DRAM_RAM_DISK_ADDR		(BOARD_DRAM_BASE_ADDR + CPU_RD_OFFSET)

#define BOARD_DRAM_KERNEL2_ADDR			(CPU2_RAM_BASE + CPU2_BOOT_OFFSET)
#define BOARD_DRAM_RAM_DISK2_ADDR		(CPU2_RAM_BASE + BOARD_DRAM_RAM_DISK_ADDR)

#define BOARD_BOOT2_MALLOC_BASE			(BOARD_DRAM_BOOT2_ADDR - BOARD_BOOT2_MALLOC_SIZE)

#define KERNEL_FILENAME					"zImage"
#define INITRD_FILENAME					"rd.gz"
#define KERNEL1_FILENAME					"zImage2"
#define INITRD1_FILENAME					"rd2.gz"

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


#endif // _BOARD_COM_H

