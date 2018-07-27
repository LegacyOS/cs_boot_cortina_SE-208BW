/****************************************************************************
 * Copyright  Storlink Corp 2002-2003.  All rights reserved.                *
 *--------------------------------------------------------------------------*
 * Name:board.s                                                             *
 * Description:  SL231x specfic define                                      *
 * Author: Plus Chen                                                        *
 * Version: 0.9 Create
 * 
 ****************************************************************************/
#ifndef _SL2312_H
#define _SL2312_H


/*-------------------------------------------------------------------------------
 Memory Map definitions
 MIDWAY is defined for Gemini ASIC, 
 	- To enable it, Please define it in board/[projrct]/inc/board_config.h 
		//#define MIDWAY	1					// define in board_config.h if GEMINI chip
-------------------------------------------------------------------------------- */
//#define FPGA_EVALUATION				// define in board_config.h if FPGA stage

#ifndef MIDWAY
#define SL2312_ROM_BASE                 0x00000000       //  ROM code base before remap
#define SL2312_RAM_BASE                 0x10000000       //  RAM code base before remap 
#define SL2312_DRAM_BASE                0x00000000       //  DRAM base after remap  
#define SL2312_FLASH_BASE         		0x10000000	 	//  FLASH base after remap 
#define SL2312_GLOBAL_BASE              0x20000000      
#define SL2312_GPIO_BASE                0x21000000      
#define SL2312_UART_BASE                0x22000000      
#define SL2312_TIMER_BASE               0x23000000      
#define SL2312_INTERRUPT_BASE           0x24000000      
#define SL2312_RTC_BASE                 0x25000000      
#define SL2312_LPC_HOST_BASE            0x26000000      
#define SL2312_LPC_IO_BASE              0x27000000
#define SL2312_WAQTCHDOG_BASE           0x28000000
#define SL2312_PCI_IO_BASE              0x30000000 
#define SL2312_PCI_MEM_BASE             0x40000000
#define SL2312_EMAC_BASE                0x50000000      
#define SL2312_SECURITY_BASE            0x51000000
#define SL2312_IDE0_BASE                0x52000000
#define SL2312_IDE1_BASE                0x52800000
#define SL2312_USB_BASE                 0x53000000  
#define SL2312_FLASH_CTRL_BASE          0x54000000
#define SL2312_DRAM_CTRL_BASE           0x55000000
#define SL2312_FLASH_SHADOW             0x70000000
#define SL2312_BIG_ENDIAN_BASE			0x80000000
#define SL2312_TIMER1_BASE              SL2312_TIMER_BASE
#define SL2312_TIMER2_BASE              (SL2312_TIMER_BASE + 0x10)
#define SL2312_TIMER3_BASE              (SL2312_TIMER_BASE + 0x20)
#else
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

#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
#define SL2312_TOE_BASE             	0x60000000
#define SL2312_GMAC0_BASE               0x6000A000      
#define SL2312_GMAC1_BASE               0x6100E000 
#else
#define SL2312_EMAC_BASE				0x60000000
#define SL2312_GMAC0_BASE               0x60000000      
#define SL2312_GMAC1_BASE               0x61000000   
#endif   

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
#endif
/*-------------------------------------------------------------------------------
 Global Module
---------------------------------------------------------------------------------*/
#define GLOBAL_ID                       0x00
#define GLOBAL_CHIP_ID                  0x003316 
#define GLOBAL_CHIP_REV                 0xA0
#define GLOBAL_STATUS                   0x04

#define GLOBAL_DRIVE_CTRL				0x14
#define GLOBAL_SLEW_RATE_CTRL			0x18
#define GLOBAL_WDOG_RESET				0x00008000

#ifdef MIDWAY
#define GLOBAL_MISC_CTRL                0x30
#define GLOBAL_REMAP_BIT                0x82000000
#define GLOBAL_LCD_EN_BIT				0x00000080

//#ifdef FLASH_TYPE_SERIAL
#define GLOBAL_SFLASH_EN_BIT             0x00000001
//#endif
//
//#ifdef FLASH_TYPE_PARALLEL
#define GLOBAL_FLASH_EN_BIT             0x00000002
//#endif
//
//#ifdef	FLASH_TYPE_NAND
#define GLOBAL_NFLASH_EN_BIT             0x00000004
//#endif	

//#define GLOBAL_SNFLASH_EN_BIT             0x00000005
#define GLOBAL_RESET					0x0c
#else
#define GLOBAL_RESET					0x10
#define GLOBAL_MISC_CTRL                0x28
#define GLOBAL_REMAP_BIT                0x00010000
#define GLOBAL_FLASH_EN_BIT             0x00000001
#endif

#define GLOBAL_ARBI_CTRL				0x00000024
#define GLOBAL_ARBI_V					0x00001F10

#define GLOBAL_CTRL_CPU1_REG                    (SL2312_GLOBAL_BASE + 0x0C)
#define CPU1_RESET_BIT_MASK                     0x40000000
#define CPU0_STATUS                                 (SL2312_GLOBAL_BASE + 0x0038)
#define CPU1_STATUS                                 (SL2312_GLOBAL_BASE + 0x003C)
#define CPU_IPI_BIT_MASK                        0x80000000


/*-------------------------------------------------------------------------------
 DRAM Module
---------------------------------------------------------------------------------*/
#define DRAM_SIZE_32M                   0x2000000
#define DRAM_SIZE_64M                   0x4000000
#define DRAM_SIZE_128M                  0x8000000

#ifndef MIDWAY
#define DRAM_SIZE                       SRAM_SIZE_32M
#else
#define DRAM_SIZE                       SRAM_SIZE_128M
#endif

#define DRAM_SDRMR                      0x00
#define DRAM_SDRTR                      0x04
#define DRAM_MCR                        0x08
#define DRAM_SDTYPE                     0x0c
#define DRAM_RD_DLL		                0x14
#define DRAM_WR_DLL     	            0x18
#define DRAM_AHBCR                      0x1c

#ifdef LEPUS_ASIC
#define MCR_PREFETCH_DISABLE            0x00000001
#else
#define MCR_PREFETCH_DISABLE            0x00000002
#endif

#ifndef MIDWAY
#define SDRMR_DISABLE_DLL               0x80020000
#define SDRAM_SDRMR_DEFAULT             0x80000022
#define SDRAM_SDRTR_DEFAULT             0x0186D52A
#define SDRAM_SDTYPE_DEFAULT            0x80000002
#define SDRAM_SDTYPE_128M               0x00000003
#define DRAM_DRIVE_DEFAULT				0x041400f0
#define DRAM_SLEW_DEFAULT				0x00000060
#define MCR_DLLTM_MASK                  0xFFFFFFFD 
#define MCR_DLLTM_ENABLE                0x00000002
#define DRAM_TRAIN_PATTERN              0x55555555
//#define SDRAM_SDTYPE_128M               0x00000003
#define SDRAM_SDTYPE_64M                0x00000002
#else  //MIDWAY
	
#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)

#define SDRMR_16_DISABLE_DLL               0x80000133 //0x80000163
#define SDRAM_16_SDTYPE_64M              	 0x40000002 //64M

#ifdef GEMINI_16BIT
	#define SDRMR_DISABLE_DLL               0x80000133 //0x80000163
#else
	#ifdef LEPUS_ASIC
		#define SDRMR_DISABLE_DLL              0x80000132 //0x80000132
	#else	
		#define SDRMR_DISABLE_DLL               0x80000162
	#endif
#endif
	
	#ifdef LEPUS_ASIC
		#define SDRMR_TIMING                    0x83A8C7FF //0x83A8C7FF 0x1400C7FE
	#else
		#define SDRMR_TIMING                    0x1200C7FE 
	#endif

#ifdef	__ORIGINAL__
	#define SDRMR_RD_DLLDLY                 0x08080808		//0x07070707  //0x06060606 
#else	// ASTEL
	#define SDRMR_RD_DLLDLY                 0x08080808
#endif

	#define SDRMR_WR_DLLDLY                 0x0000001A		//0x00000019

	#ifdef LEPUS_FPGA
		#define MCR_DLLTM_ENABLE                0x00000012
		#define MCR_DLLTM_MASK                  0xFFFFFFFD 
	#else	
		#ifdef LEPUS_ASIC
			#define MCR_DLLTM_ENABLE                0x00000055
		#else
			#define MCR_DLLTM_ENABLE                0x00000015
		#endif
		#define MCR_DLLTM_MASK                  0xFFFFFFFE 
	#endif

#define LPC_CLK_ENABLE                  0x00010040 
#else //#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)
	
#define SDRMR_TIMING                    0x1200C7FE 
#define SDRMR_RD_DLLDLY                 0x06060606  //0x06060606 
#define SDRMR_WR_DLLDLY                 0x00000018  //0x00000019
///////////////
#define SDRMR_DISABLE_DLL               0x80010132
#define MCR_DLLTM_ENABLE                0x00000012
#define MCR_DLLTM_MASK                  0xFFFFFFFD 
#endif  //GEMINI_ASIC //#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)

#define DRAM_RMCR						0x40
#define RMCR_DEFAULT					0x04000040	// swap 64MB

#define SDRAM_SDRMR_DEFAULT             0x80000022
//#define SDRAM_SDRTR_DEFAULT             0x0186D52A
#define SDRAM_SDRTR_DEFAULT             0x80faa5bf
#define SDRAM_SDTYPE_DEFAULT            0x80000002

#ifdef GEMINI_16BIT
#define SDRAM_SDTYPE_128M               0x40000002 //64M
//#define SDRAM_SDTYPE_128M               0x40000001 //32M
#else
#define SDRAM_SDTYPE_128M               0x00000002 //0x00000002
#endif

#define SDRAM_SDTYPE_64M                0x00000001
#define DRAM_DRIVE_DEFAULT				0x041400f0
#define DRAM_SLEW_DEFAULT				0x00000060


#define DRAM_TRAIN_PATTERN              0x55555555
////////////////
#endif
/*-------------------------------------------------------------------------------
 Flash Module
---------------------------------------------------------------------------------*/
#ifdef MIDWAY
// #define NFLASH_TYPE						0x6500000C
// #define page_1							0x30000200
// #define page_4							0x30000800
//#ifdef FLASH_TYPE_SERIAL
//#define FLASH_ACCESS_OFFSET             0x10
//#else
#define FLASH_ACCESS_OFFSET             0x20
//#endif
#define ACCESS_CONTINUE_MODE            0x00008000
#define ACCESS_CONTINUE_DISABLE         0xFFFF7FFF
#define FLASH_DIRECT_ACCESS		 		0x00004000
#else
#define FLASH_ACCESS_OFFSET             0x10
#define ACCESS_CONTINUE_MODE            0x00008000
#define ACCESS_CONTINUE_DISABLE         0xFFFF7FFF
#define FLASH_DIRECT_ACCESS		 		0x00004000
#endif

/*-------------------------------------------------------------------------------
 UART definitions
 --------------------------------------------------------------------------------*/
#define SERIAL_THR                    	0x00	 		/*  Transmitter Holding Register(Write).*/
#define SERIAL_RBR                     	0x00	 		/*  Receive Buffer register (Read).*/
#define SERIAL_IER                     	0x04	 		/*  Interrupt Enable register.*/
#define SERIAL_IIR                     	0x08	 		/*  Interrupt Identification register(Read).*/
#define SERIAL_FCR                     	0x08	 		/*  FIFO control register(Write).*/
#define SERIAL_LCR                     	0x0C	 		/*  Line Control register.*/
#define SERIAL_MCR                     	0x10	 		/*  Modem Control Register.*/
#define SERIAL_LSR                     	0x14	 		/*  Line status register(Read) .*/
#define SERIAL_MSR                     	0x18	 		/*  Modem Status register (Read).*/
#define SERIAL_SPR                     	0x1C     		/*  Scratch pad register */
#define SERIAL_DLL                     	0x0      		/*  Divisor Register LSB */
#define SERIAL_DLM                     	0x4      		/*  Divisor Register MSB */
#define SERIAL_PSR                     	0x8     		/* Prescale Divison Factor */

#define SERIAL_MDR				0x20
#define SERIAL_ACR				0x24
#define SERIAL_TXLENL			0x28
#define SERIAL_TXLENH			0x2C
#define SERIAL_MRXLENL			0x30
#define SERIAL_MRXLENH			0x34
#define SERIAL_PLR				0x38
#define SERIAL_FMIIR_PIO		0x3C

/* IER Register */
#define SERIAL_IER_DR                  	0x1      	        /* Data ready Enable */
#define SERIAL_IER_TE                  	0x2      	        /* THR Empty Enable */
#define SERIAL_IER_RLS                 	0x4      	        /* Receive Line Status Enable */
#define SERIAL_IER_MS                  	0x8      	        /* Modem Staus Enable */
#define SERIAL_IER_ENABLE               0x5
#define SERIAL_IER_DISABLE              0x0 

/* IIR Register */
#define SERIAL_IIR_NONE                	0x1			/* No interrupt pending */
#define SERIAL_IIR_RLS                 	0x6			/* Receive Line Status */
#define SERIAL_IIR_DR                  	0x4			/* Receive Data Ready */
#define SERIAL_IIR_TIMEOUT             	0xc			/* Receive Time Out */
#define SERIAL_IIR_TE                  	0x2			/* THR Empty */
#define SERIAL_IIR_MODEM               	0x0			/* Modem Status */

/* FCR Register */
#define SERIAL_FCR_FE                  	0x1 	 		/* FIFO Enable */
#define SERIAL_FCR_RXFR                	0x2 	 		/* Rx FIFO Reset */
#define SERIAL_FCR_TXFR                	0x4 	 		/* Tx FIFO Reset */
#define SERIAL_FCR_FIFO_ENABLE          0x7

/* LCR Register */
#define SERIAL_LCR_LEN5                	0x0
#define SERIAL_LCR_LEN6                	0x1
#define SERIAL_LCR_LEN7                	0x2
#define SERIAL_LCR_LEN8                	0x3

#define SERIAL_LCR_STOP                	0x4
#define SERIAL_LCR_EVEN                	0x18 	 	        /* Even Parity */
#define SERIAL_LCR_ODD                 	0x8      	        /* Odd Parity */
#define SERIAL_LCR_PE                  	0x8			/* Parity Enable */
#define SERIAL_LCR_SETBREAK            	0x40	 		/* Set Break condition */
#define SERIAL_LCR_STICKPARITY         	0x20	 		/* Stick Parity Enable */
#define SERIAL_LCR_DLAB                	0x80     	        /* Divisor Latch Access Bit */

/* LSR Register */
#define SERIAL_LSR_DR                  	0x1      	        /* Data Ready */
#define SERIAL_LSR_OE                  	0x2      	        /* Overrun Error */
#define SERIAL_LSR_PE                  	0x4      	        /* Parity Error */
#define SERIAL_LSR_FE                  	0x8      	        /* Framing Error */
#define SERIAL_LSR_BI                  	0x10     	        /* Break Interrupt */
#define SERIAL_LSR_THRE                	0x20     	        /* THR Empty */
#define SERIAL_LSR_TE                  	0x40     	        /* Transmitte Empty */
#define SERIAL_LSR_DE                  	0x80     	        /* FIFO Data Error */

/* MCR Register */
#define SERIAL_MCR_DTR                 	0x1			/* Data Terminal Ready */
#define SERIAL_MCR_RTS                 	0x2			/* Request to Send */
#define SERIAL_MCR_OUT1                	0x4			/* output	1 */
#define SERIAL_MCR_OUT2                	0x8			/* output2 or global interrupt enable */
#define SERIAL_MCR_LPBK                	0x10	 		/* loopback mode */

/* MSR Register */
#define SERIAL_MSR_DELTACTS            	0x1			/* Delta CTS */
#define SERIAL_MSR_DELTADSR            	0x2			/* Delta DSR */
#define SERIAL_MSR_TERI                	0x4			/* Trailing Edge RI */
#define SERIAL_MSR_DELTACD             	0x8			/* Delta CD */
#define SERIAL_MSR_CTS                 	0x10	 		/* Clear To Send */
#define SERIAL_MSR_DSR                 	0x20	 		/* Data Set Ready */
#define SERIAL_MSR_RI                  	0x40	 		/* Ring Indicator */
#define SERIAL_MSR_DCD                 	0x80	 		/* Data Carrier Detect */

/* MDR register */
#define SERIAL_MDR_MODE_SEL		0x03
#define SERIAL_MDR_UART			0x0
#define SERIAL_MDR_SIR			0x1
#define SERIAL_MDR_FIR			0x2

/*-------------------------------------------------------------------------------
 Timer definitions
---------------------------------------------------------------------------------*/

#define SL2312_TIMER_COUNT              0x0
#define SL2312_TIMER_LOAD               0x4
#define SL2312_TIMER_MATCH1             0x8
#define SL2312_TIMER_MATCH2             0xC
#define SL2312_TIMER_CR                 0x30
#define SL2312_TIMER1_ENABLE            0x0001
#define SL2312_TIMER1_SRC_PCLK          0x0002
#define SL2312_TIMER1_OVERFLOW          0x0004
#define SL2312_TIMER2_ENABLE            0x0008
#define SL2312_TIMER2_SRC_PCLK          0x0010
#define SL2312_TIMER2_OVERFLOW          0x0020
#define SL2312_TIMER3_ENABLE            0x0040
#define SL2312_TIMER3_SRC_PCLK          0x0080
#define SL2312_TIMER3_OVERFLOW          0x0100
#define SL2312_TIMER_CTRL_BASE         (SL2312_TIMER_BASE + 0x30)

/*-------------------------------------------------------------------------------
 Interrupt Controller definitions
---------------------------------------------------------------------------------*/
// interrupt register
#define SL2312_IRQ_SOURCE              0
#define SL2312_IRQ_MASK                0x04
#define SL2312_IRQ_CLEAR               0x08
#define SL2312_IRQ_MODE                0x0c
#define SL2312_IRQ_LEVEL               0x10
#define SL2312_IRQ_STATUS              0x14

#define SL2312_FIQ_SOURCE              0x20
#define SL2312_FIQ_MASK                0x24
#define SL2312_FIQ_CLEAR               0x28
#define SL2312_FIQ_MODE                0x2c
#define SL2312_FIQ_LEVEL               0x30
#define SL2312_FIQ_STATUS              0x34

#ifndef MIDWAY    
#define SL2312_INTERRUPT_SERIRQ15		31 
#define SL2312_INTERRUPT_SERIRQ14		30 
#define SL2312_INTERRUPT_SERIRQ13		29 
#define SL2312_INTERRUPT_SERIRQ12		28 
#define SL2312_INTERRUPT_SERIRQ11		27 
#define SL2312_INTERRUPT_SERIRQ10		26 
#define SL2312_INTERRUPT_SERIRQ9		25 
#define SL2312_INTERRUPT_SERIRQ8		24 
#define SL2312_INTERRUPT_SERIRQ7		23 
#define SL2312_INTERRUPT_SERIRQ6		22 
#define SL2312_INTERRUPT_SERIRQ5		21 
#define SL2312_INTERRUPT_SERIRQ4		20	
#define SL2312_INTERRUPT_SERIRQ3		19 
#define SL2312_INTERRUPT_SERIRQ2		18 
#define SL2312_INTERRUPT_SERIRQ1		17 
#define SL2312_INTERRUPT_SERIRQ0		16 
#define SL2312_INTERRUPT_GPIO			13 
#define SL2312_INTERRUPT_SERIOCHK		12	
#define SL2312_INTERRUPT_PCI			11 
#define SL2312_INTERRUPT_RTC			10 
#define SL2312_INTERRUPT_TIMER3			9  
#define SL2312_INTERRUPT_TIMER2			8  
#define SL2312_INTERRUPT_TIMER1			7  
#define SL2312_INTERRUPT_UART			6  
#define SL2312_INTERRUPT_USB			5  
#define SL2312_INTERRUPT_IPSEC			4  
#define SL2312_INTERRUPT_IDE			3  
#define SL2312_INTERRUPT_RESERVED		2  
#define SL2312_INTERRUPT_EMAC			1  
#define SL2312_INTERRUPT_WATCHDOG		0  
#else
// gemini
#define	SL2312_INTERRUPT_SERIRQ			31
#define	SL2312_INTERRUPT_SERIRQ4		31
#define	SL2312_INTERRUPT_SERIRQ3       	30
#define	SL2312_INTERRUPT_SERIRQ2       	29
#define	SL2312_INTERRUPT_SERIRQ1       	28
#define	SL2312_INTERRUPT_SERIRQ0       	27
#define SL2312_INTERRUPT_PWR			26
#define SL2312_INTERRUPT_CIR			25
#define	SL2312_INTERRUPT_GPIO2         	24
#define	SL2312_INTERRUPT_GPIO1         	23
#define	SL2312_INTERRUPT_GPIO	       	22
#define	SL2312_INTERRUPT_SSP           	21	
#define SL2312_INTERRUPT_LPC            20
#define SL2312_INTERRUPT_LCD            19
#define	SL2312_INTERRUPT_UART       	18
#define	SL2312_INTERRUPT_RTC        	17
#define	SL2312_INTERRUPT_TIMER3        	16
#define	SL2312_INTERRUPT_TIMER2        	15 
#define	SL2312_INTERRUPT_TIMER1        	14
#define SL2312_INTERRUPT_FLASH			12
#define	SL2312_INTERRUPT_USB1          	11
#define SL2312_INTERRUPT_USB			10
#define	SL2312_INTERRUPT_DMA           	9
#define	SL2312_INTERRUPT_PCI        	8 
#define	SL2312_INTERRUPT_IPSEC      	7 
#define	SL2312_INTERRUPT_RAID           6 
#define	SL2312_INTERRUPT_IDE1          	5 
#define	SL2312_INTERRUPT_IDE        	4 
#define	SL2312_INTERRUPT_WATCHDOG       3 
#define	SL2312_INTERRUPT_GMAC1          2 
#define SL2312_INTERRUPT_GMAC0		    1
#define	SL2312_INTERRUPT_CPU0_IP_IRQ    0 
#endif
/*-------------------------------------------------------------------------------
 System Clock
---------------------------------------------------------------------------------*/
#ifdef FPGA_EVALUATION
#define SYS_CLK                     	20000000	
#else
#define SYS_CLK                     	100000000
#endif

#define AHB_CLK                     	SYS_CLK 
#define MAX_TIMER                   	3
#ifdef FPGA_EVALUATION
#define APB_CLK                     	SYS_CLK	
#else 
#define APB_CLK                     	(SYS_CLK / 4)	
#endif

#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)
#define UART_CLK                        48000000    //b0:48000000  a0:30000000
#define SL2312_UART_DIR					0x00000400 //0x00002c00
#define SL2312_UART_PIN					0x00000600  //0x0000ff00
#else
#define UART_CLK                        48000000
#define SL2312_UART_PIN					0x10200000
#endif

#define SL2312_BAUD_115200              (UART_CLK / 1843200)
#define SL2312_BAUD_57600               (UART_CLK / 921600) 
#define SL2312_BAUD_38400				(UART_CLK / 614400)
#define SL2312_BAUD_19200               (UART_CLK / 307200)
#define SL2312_BAUD_14400               (UART_CLK / 230400)
#define SL2312_BAUD_9600                (UART_CLK / 153600)

/*-------------------------------------------------------------------------------
 LPC module
---------------------------------------------------------------------------------*/
#define LPC_BUS_CTRL	                0
#define LPC_BUS_STATUS	                2
#define LPC_SERIAL_IRQ_CTRL	        	4

//#define LPC_IT8712						1

/*-------------------------------------------------------------------------------
 ARM9 CPSR Register defines
---------------------------------------------------------------------------------*/
#define ARM_CPSR_IRQ_DISABLE		0x80	// IRQ disabled when =1
#define ARM_CPSR_FIQ_DISABLE		0x40	// FIQ disabled when =1
#define ARM_CPSR_THUMB_ENABLE		0x20	// Thumb mode when =1
#define ARM_CPSR_FIQ_MODE			0x11
#define ARM_CPSR_IRQ_MODE			0x12
#define ARM_CPSR_SUPERVISOR_MODE	0x13
#define ARM_CPSR_UNDEF_MODE			0x1B

#define ARM_CPSR_MODE_BITS			0x1F

#define ARM_VECTOR_RESET                0
#define ARM_VECTOR_UNDEF_INSTRUCTION    1
#define ARM_VECTOR_SOFTWARE_INTERRUPT   2
#define ARM_VECTOR_ABORT_PREFETCH       3
#define ARM_VECTOR_ABORT_DATA           4
#define ARM_VECTOR_RESERVED             5
#define ARM_VECTOR_IRQ                  6
#define ARM_VECTOR_FIQ                  7

#define ARM_INTERRUPT_NONE				-1

#define HAL_DISABLE_INTERRUPTS(_old_)           \
    asm volatile (                              \
        "mrs %0,cpsr;"                          \
        "mrs r4,cpsr;"                          \
        "orr r4,r4,#0xC0;"                      \
        "msr cpsr,r4"                           \
        : "=r"(_old_)	/* output */			\
        :               /* input */				\
        : "r4"		/* affected registers */	\
        );

#define HAL_ENABLE_INTERRUPTS()                 \
    asm volatile (                              \
        "mrs r3,cpsr;"                          \
        "bic r3,r3,#0xC0;"                      \
        "msr cpsr,r3"                           \
        :               	/* output */		\
        :                   /* input */			\
        : "r3"		/* affected registers */	\
        );

#define HAL_RESTORE_INTERRUPTS(_old_)           \
    asm volatile (                              \
        "mrs r3,cpsr;"                          \
        "and r4,%0,#0xC0;"                      \
        "bic r3,r3,#0xC0;"                      \
        "orr r3,r3,r4;"                         \
        "msr cpsr,r3"                           \
        :                   /* output */		\
        : "r"(_old_)        /* input */			\
        : "r3", "r4"  /* affected registers */	\
        );

#define HAL_QUERY_INTERRUPTS(_old_)             \
    asm volatile (                              \
        "mrs r4,cpsr;"                          \
        "and r4,r4,#0xC0;"                      \
        "eor %0,r4,#0xC0;"                      \
        : "=r"(_old_)		/* output */		\
        :					/* input */			\
        : "r4"		/* affected registers */	\
        );

// Enable the instruction cache
#define HAL_ICACHE_ENABLE()                                             \
    asm volatile (                                                      \
        "mrc  p15,0,r1,c1,c0,0;"                                        \
        "orr  r1,r1,#0x1000;"                                           \
        "orr  r1,r1,#0x0003;" /*  enable ICache (also ensures */ 		\
                              /* that MMU and alignment faults */       \
                              /* are enabled)                  */       \
        "mcr  p15,0,r1,c1,c0,0"                                         \
        :                                                               \
        :                                                               \
        : "r1" /* Clobber list */                                       \
        );

// Disable the instruction cache (and invalidate it, required semanitcs)
#define HAL_ICACHE_DISABLE()                                            \
    asm volatile (                                                      \
        "mrc    p15,0,r1,c1,c0,0;"                                      \
        "bic    r1,r1,#0x1000;" /* disable ICache (but not MMU, etc) */ \
        "mcr    p15,0,r1,c1,c0,0;"                                      \
        "mov    r1,#0;"                                                 \
        "mcr    p15,0,r1,c7,c5,0;"  /* flush ICache */                  \
        "nop;" /* next few instructions may be via cache    */          \
        "nop;"                                                          \
        "nop;"                                                          \
        "nop;"                                                          \
        "nop;"                                                          \
        "nop"                                                           \
        :                                                               \
        :                                                               \
        : "r1" /* Clobber list */                                       \
        );

// Query the state of the instruction cache
#define HAL_ICACHE_IS_ENABLED(_state_)                                   \
    register cyg_uint32 reg;                                             \
    asm volatile ("mrc  p15,0,%0,c1,c0,0"                                \
                  : "=r"(reg)                                            \
                  :                                                      \
        );                                                               \
    (_state_) = (0 != (0x1000 & reg)); /* Bit 12 is ICache enable */

// Invalidate the entire cache
#define HAL_ICACHE_INVALIDATE_ALL()                                     \
    /* this macro can discard dirty cache lines (N/A for ICache) */     \
    asm volatile (                                                      \
        "mov    r1,#0;"                                                 \
        "mcr    p15,0,r1,c7,c5,0;"  /* flush ICache */                  \
        "mcr    p15,0,r1,c8,c5,0;"  /* flush ITLB only */               \
        "nop;" /* next few instructions may be via cache    */          \
        "nop;"                                                          \
        "nop;"                                                          \
        "nop;"                                                          \
        "nop;"                                                          \
        "nop;"                                                          \
        :                                                               \
        :                                                               \
        : "r1" /* Clobber list */                                       \
        );

#define HAL_DCACHE_ENABLE()                                             \
    asm volatile (                                                      \
        "mrc  p15,0,r1,c1,c0,0;"                                        \
        "orr  r1,r1,#0x000F;" /* enable DCache (also ensures    */      \
                              /* the MMU, alignment faults, and */      \
                              /* write buffer are enabled)      */      \
        "mcr  p15,0,r1,c1,c0,0"                                         \
        :                                                               \
        :                                                               \
        : "r1" /* Clobber list */                                       \
        );

// Disable the data cache (and invalidate it, required semanitcs)
#define HAL_DCACHE_DISABLE()                                            \
    asm volatile (                                                      \
        "mrc  p15,0,r1,c1,c0,0;"                                        \
        "bic  r1,r1,#0x000C;" /* disable DCache AND write buffer  */    \
                              /* but not MMU and alignment faults */    \
        "mcr  p15,0,r1,c1,c0,0;"                                        \
        "mov    r1,#0;"                                                 \
        "mcr  p15,0,r1,c7,c6,0" /* clear data cache */                  \
        :                                                               \
        :                                                               \
        : "r1" /* Clobber list */                                       \
        ); 

// Query the state of the data cache
#define HAL_DCACHE_IS_ENABLED(_state_)                                   \
    register int reg;                                                    \
    asm volatile ("mrc  p15,0,%0,c1,c0,0;"                               \
                  : "=r"(reg)                                            \
                  :                                                      \
        );                                                               \
    (_state_) = (0 != (4 & reg)); /* Bit 2 is DCache enable */

// Flush the entire dcache (and then both TLBs, just in case)
#define HAL_DCACHE_INVALIDATE_ALL()                                     \
    asm volatile (                                                      \
		"mov    r0,#0;"                                                 \
        "mcr    p15,0,r0,c7,c6,0;" /* flush d-cache */                  \
        "mcr    p15,0,r0,c8,c7,0;" /* flush i+d-TLBs */                 \
        :                                                               \
        :                                                               \
        : "r0","memory" /* clobber list */);

// Synchronize the contents of the cache with memory.
#define HAL_DCACHE_SYNC()                                               \
    asm volatile (                                                      \
        "mov    r0, #0;"                                                \
        "mcr    p15,0,r0,c7,c10,0;"  /* clean DCache */                 \
        "1: mrc p15,0,r0,c15,c4,0;"  /* wait for dirty flag to clear */ \
        "ands   r0,r0,#0x80000000;"                                     \
        "bne    1b;"                                                    \
        "mov    r0,#0;"                                                 \
        "mcr    p15,0,r0,c7,c6,0;"  /* flush DCache */                  \
        "mcr    p15,0,r0,c7,c10,4;" /* and drain the write buffer */    \
        :                                                               \
        :                                                               \
        : "r0" /* Clobber list */                                       \
        );

// Synchronize the contents of the cache with memory.
// (which includes flushing out pending writes)
#define HAL_ICACHE_SYNC()                                       \
    HAL_DCACHE_SYNC(); /* ensure data gets to RAM */            \
    HAL_ICACHE_INVALIDATE_ALL(); /* forget all we know */

#endif
