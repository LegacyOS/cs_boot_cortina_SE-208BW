#ifndef __sl2312_h
#define __sl2312_h
#include "midway.h"
/****************************************************************************
 * Copyright  Storlink Corp 2002-2003.  All rights reserved.                *
 *--------------------------------------------------------------------------*
 * Name:board.s                                                             *
 * Description:  SL231x specfic define                                      *
 * Author: Plus Chen                                                        *
 * Version: 0.9 Create
 ****************************************************************************/

/*
  CPE address map;

               +====================================================
    0x00000000 | FLASH
    0x0FFFFFFF | 
               |====================================================
    0x10000000 | SDRAM 
    0x1FFFFFFF |
               |====================================================
    0x20000000 | Global Registers        0x20000000-0x20FFFFFF  
               | EMAC and DMA            0x21000000-0x21FFFFFF
               | UART Module             0x22000000-0x22FFFFFF
               | Timer Module            0x23000000-0x23FFFFFF
               | Interrupt Module        0x24000000-0x24FFFFFF
               | RTC Module              0x25000000-0x25FFFFFF
               | LPC Host Controller     0x26000000-0x26FFFFFF 
               | LPC Peripherial IO      0x27000000-0x27FFFFFF 
               | WatchDog Timer          0x28000000-0x28FFFFFF  
    0x2FFFFFFF | Reserved                0x29000000-0x29FFFFFF
               |=====================================================
    0x30000000 | PCI IO, Configuration Registers
    0x3FFFFFFF |
               |=====================================================
    0x40000000 | PCI Memory
    0x4FFFFFFF |
               |=====================================================
    0x50000000 | Ethernet MAC and DMA    0x50000000-0x50FFFFFF
               | Security and DMA        0x51000000-0x51FFFFFF    
               | IDE Register            0x52000000-0x52FFFFFF
               | USB Register            0x53000000-0x53FFFFFF
               | Flash Controller        0x54000000-0x54FFFFFF
               | DRAM Controller         0x55000000-0x55FFFFFF
    0x5FFFFFFF | Reserved                0x56000000-0x5FFFFFFF
               |=====================================================
    0x60000000 | Reserved
    0x6FFFFFFF |
               |=====================================================
    0x70000000 | FLASH shadow Memory
    0x7FFFFFFF |
               |=====================================================
    0x80000000 | Big Endian of memory    0x00000000-0x7FFFFFFF 
    0xFFFFFFFF |
               +=====================================================
*/



/*-------------------------------------------------------------------------------
 Memory Map definitions
-------------------------------------------------------------------------------- */
#ifdef MIDWAY_DIAG
	#define SL2312_SRAM_BASE                0x00000000       //  SRAM base before remap
	#define SL2312_DRAM_BASE                0x00000000       //  DRAM base after remap  
	#define SL2312_RAM_BASE                 0x10000000       //  RAM code base before remap 
	#define SL2312_FLASH_BASE         	    0x30000000	 
	#define SL2312_ROM_BASE                 0x30000000 
	#define SL2312_GLOBAL_BASE              0x40000000      
	#define SL2312_WAQTCHDOG_BASE           0x41000000
	#define SL2312_UART_BASE                0x42000000      
	#define SL2312_TIMER_BASE               0x43000000      
	#define SL2312_LCD_BASE                 0x44000000      
	#define SL2312_RTC_BASE                 0x45000000      
	#define SL2312_SATA_BASE                0x46000000      
	#define SL2312_LPC_HOST_BASE            0x47000000      
	#define SL2312_LPC_IO_BASE              0x47800000
	#define SL2312_INTERRUPT_BASE           0x48000000      
	#define SL2312_INTERRUPT0_BASE          0x48000000      
	#define SL2312_INTERRUPT1_BASE          0x49000000      
	#define SL2312_SSP_CTRL_BASE            0x4A000000      
	#define SL2312_POWER_CTRL_BASE          0x4B000000      
	#define SL2312_CIR_BASE                 0x4C000000      
	#define SL2312_GPIO_BASE                0x4D000000      
	#define SL2312_GPIO_BASE1               0x4E000000      
	#define SL2312_GPIO_BASE2               0x4F000000      
	#define SL2312_PCI_IO_BASE              0x50000000 
	#define SL2312_PCI_MEM_BASE             0x58000000
	#define SL2312_GMAC0_BASE               0x60000000      
	#define SL2312_GMAC1_BASE               0x61000000      
	#define SL2312_SECURITY_BASE            0x62000000
	#define SL2312_IDE0_BASE                0x63000000
	#define SL2312_IDE1_BASE				0x63400000
	#define SL2312_RAID_BASE                0x64000000  
	#define SL2312_FLASH_CTRL_BASE          0x65000000
	#define SL2312_DRAM_CTRL_BASE           0x66000000
	#define SL2312_GENERAL_DMA_BASE         0x67000000  
	#define SL2312_USB_BASE                 0x68000000  
	#define SL2312_USB0_BASE                0x68000000  
	#define SL2312_USB1_BASE                0x69000000  
	#define SL2312_FLASH_SHADOW             0x30000000
	#define SL2312_BIG_ENDIAN_BASE			0x80000000
	
	#define SL2312_TIMER1_BASE              SL2312_TIMER_BASE
	#define SL2312_TIMER2_BASE              (SL2312_TIMER_BASE + 0x10)
	#define SL2312_TIMER3_BASE              (SL2312_TIMER_BASE + 0x20)
#else
	#define SL2312_ROM_BASE                 0x00000000       //  ROM code base before remap
	#define SL2312_RAM_BASE                 0x10000000       //  RAM code base before remap 
	#define SL2312_DRAM_BASE                0x00000000       //  DRAM base after remap  
	#define SL2312_FLASH_BASE         		0x10000000	 //  FLASH base after remap 
	#define SL2312_GLOBAL_BASE              0x20000000      
	#define SL2312_GPIO_BASE2               0x21000000      
	#define SL2312_GPIO_BASE1               0x21000000      
	#define SL2312_GPIO_BASE                0x21000000      
	#define SL2312_UART_BASE                0x22000000      
	#define SL2312_TIMER_BASE               0x23000000      
	#define SL2312_INTERRUPT_BASE           0x24000000      
	#define SL2312_INTERRUPT1_BASE          0x24000000      
	#define SL2312_RTC_BASE                 0x25000000      
	#define SL2312_LPC_HOST_BASE            0x26000000      
	#define SL2312_LPC_IO_BASE              0x27000000
	#define SL2312_WAQTCHDOG_BASE           0x28000000
	#define SL2312_PCI_IO_BASE              0x30000000 
	#define SL2312_PCI_MEM_BASE             0x40000000
	#define SL2312_EMAC_BASE                0x50000000      
	#define SL2312_SECURITY_BASE            0x51000000
	#define SL2312_IDE0_BASE                0x52000000
	#define SL2312_IDE1_BASE				0x52800000
	#define SL2312_USB_BASE                 0x53000000  
	#define SL2312_FLASH_CTRL_BASE          0x54000000
	#define SL2312_DRAM_CTRL_BASE           0x55000000
	#define SL2312_FLASH_SHADOW             0x70000000
	#define SL2312_BIG_ENDIAN_BASE			0x80000000
	#define SL2312_SSP_CTRL_BASE            0x4A000000      
	#define SL2312_GENERAL_DMA_BASE         0x67000000  
	
	#define SL2312_TIMER1_BASE              SL2312_TIMER_BASE
	#define SL2312_TIMER2_BASE              (SL2312_TIMER_BASE + 0x10)
	#define SL2312_TIMER3_BASE              (SL2312_TIMER_BASE + 0x20)
#endif
/*-------------------------------------------------------------------------------
 Global Module
---------------------------------------------------------------------------------*/
#define GLOBAL_ID                       0x00
#define GLOBAL_CHIP_ID                  0x002311 
#define GLOBAL_CHIP_REV                 0xA0
#define GLOBAL_STATUS                   0x04
#define GLOBAL_CONTROL                  0x1C
#define GLOBAL_REMAP_BIT                0x01

/*-------------------------------------------------------------------------------
 DRAM Module
---------------------------------------------------------------------------------*/
#define DRAM_SIZE_32M                   0x2000000
#define DRAM_SIZE_64M                   0x4000000
#define DRAM_SIZE_128M                  0x8000000

#define DRAM_SIZE                       SRAM_SIZE_32M

#define DRAM_SDRMR                      0x00
#define SDRMR_DISABLE_DLL               0x80010000
     
/*-------------------------------------------------------------------------------
 Interrupt Controllers
---------------------------------------------------------------------------------*/
#define IRQ_SOURCE         	        0
#define IRQ_MASK           	        0x04
#define IRQ_CLEAR	   	        0x08
#define IRQ_MODE	                0x0c
#define IRQ_LEVEL	         	0x10
#define IRQ_STATUS	                0x14

#define FIQ_SOURCE               	0x20
#define FIQ_MASK                   	0x24
#define FIQ_CLEAR	                0x28
#define FIQ_MODE	          	0x2c
#define FIQ_LEVEL	                0x30
#define FIQ_STATUS	                0x34

/*-------------------------------------------------------------------------------
 System Clock
---------------------------------------------------------------------------------*/
#define ASIC_SL3516                     1
//#define SL2312_ASIC                     1
#ifndef SYS_CLK
#ifdef SL2312_ASIC
#define SYS_CLK                     	100000000	
#else
#define SYS_CLK                     	20000000	
#endif
#endif

#define AHB_CLK                     	SYS_CLK 
#define MAX_TIMER                   	3
#ifndef APB_CLK
#define APB_CLK                     	(SYS_CLK / 4)	
#endif
#define UART_CLK                        48000000

#define SL2312_BAUD_115200              (UART_CLK / 1843200)
#define SL2312_BAUD_57600               (UART_CLK / 921600) 
#define SL2312_BAUD_38400		(UART_CLK / 614400)
#define SL2312_BAUD_19200               (UART_CLK / 307200)
#define SL2312_BAUD_14400               (UART_CLK / 230400)
#define SL2312_BAUD_9600                (UART_CLK / 153600)

#ifndef DEFAULT_HOST_BAUD
#define DEFAULT_HOST_BAUD               SL2312_BAUD_19200
#endif


void delay_ms(int ms);
void delay_us(int us);

#endif


