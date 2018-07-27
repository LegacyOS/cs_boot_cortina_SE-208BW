
/* DO NOT EDIT!! - this file automatically generated
 *                 from .s file by awk -f s2h.awk
 */
/****************************************************************************
 * Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
 *--------------------------------------------------------------------------*
 * Name:cpe.s                                                               *
 * Description: CPE board specfic define                                    *
 * Author: Fred Chien                                                       *
 * **************************************************************************
 */

/* 
 *   CPE address map;
 * 
 *                +==========================================
 *     0x00000000 | ROM 
 *                |
 *     0x00080000 |==========================================
 *                | SRAM
 *                |==========================================
 *     0x10000000 | SDRAM 
 *     0x8FFFFFFF |
 *                |==========================================
 *     0x90000000 |Controller's reg				
 *                |						
 *                |0x90100000 Arbiter/Decoder             						
 *                |0x90200000 SRAM controller reg  
 *        		|0x902ffffc Debug Numberic LED					
 *                |0x90900000 SDRAM controller reg        						
 *                |0x90400000 DMA controller reg          						
 *                |0x90500000 AHB2APB bridge              						
 *                |0x90600000 Reserved                    						
 *                |0x91000000-91F00000 DMA0-DMA15 Device; 						
 *                |0x92400000 DSP                         						
 *                |0x92500000 Reserved                         						
 *                |0x96500000 LCD                         						
 *                |0x96600000 Bluetooth                        						
 *                |0x96700000 MAC        						
 *                |0x96800000 PCI                       						
 *                |0x96900000 USB2.0 host                       
 *                |0x98000000-9AFFFFFF APB Device                                        
 *     0x98000000 |==========================================
 *                | APB Device's Reg
 *                |
 *                |0x98000000 Reserved
 *                |0x98100000 Power Managemnet
 *                |0x98200000 UART1
 *                |0x98300000 UART2/IrDA
 *                |0x98400000 Timer
 *                |0x98500000 Watchdog Timer
 *                |0x98600000 RTC
 *                |0x98700000 GPIO
 *                |0x98800000 INTC
 *                |0x98900000 UART3
 *                |0x98A00000 I2C
 *                |0x98B00000 SSP1
 *                |0x98C00000 USB Device
 *                |0x98D00000 Compact Flash
 *                |0x98E00000 Secure Digital
 *                |0x98F00000 SMC
 *                |0x99000000 MS
 *                |0x99100000 SCI
 *                |0x99200000 ECP/EPP
 *                |0x99300000 KBC                 
 *                |0x99400000 I2S                 
 *                |0x99500000 AC97                 
 *                |0x99600000 SSP2                 
 *                |0x99700000 Mouse                 
 *                |0x9AFFFFFF Reserved                             
 *                |          
 *                +==========================================
 */
#ifndef __CPE_H
#define __CPE_H


#include 	"chipset.h"

/***************************************
Board definition
****************************************/
//#define FPLAT_CPE10x
//#define FPLAT_CPE110
#define FPLAT_CPE120

//#define PCLK                            0
//#define EXTCLK                          1


/* ===============================================================================
 *  CPE definitions
 * ===============================================================================
 * -------------------------------------------------------------------------------
 *  Memory definitions
 * -------------------------------------------------------------------------------
 */


#define CPE_ROM_LO                      0x00000000
#define CPE_ROM_HI                      0x80000000
#define CPE_ROM_BASE                    CPE_ROM_HI	   /* remap position */
#define CPE_ROM_SIZE                    SZ_128K

#define CPE_SRAM_LO                     0x00080000
#define SRAMBANK_BASE               	CPE_SRAM_LO
#define CPE_SRAM_HI                     0x80080000  
#define CPE_SRAM_BASE                   CPE_SRAM_HI    /* remap position */
#define CPE_SRAM_SIZE                   SZ_512K

#define CPE_SDRAM_LO                    0x00000000
#define CPE_SDRAM_HI                    0x10000000
#define CPE_SDRAM_BASE                  CPE_SDRAM_LO   /* remap postion  */
#define CPE_SDRAM_SIZE                  SZ_128M


/*  -------------------------------------------------------------------------------
 *   CPE system registers
 *  ------------------------------------------------------------------------------- 
 */


/************************************* CPE120 ************************************/
#ifdef FPLAT_CPE120
#define CPE_DECODER_BASE                0x90100000
#define CPE_SRAMC_BASE                  0x90200000
#define CPE_SDRAMC_BASE                 0x90300000
#define CPE_DMA_BASE					0x90400000
#define CPE_APB_BASE                	0x90500000
#define CPE_PWM_BASE                    0x98100000      /*   Power Management 		*/
#define CPE_UART1_BASE                  0x98200000	 	/*   UART 1 				*/
#define CPE_UART2_BASE                  0x98300000	 	/*   UART 2/IrDA 			*/
#define CPE_CT_BASE                     0x98400000	 	/*   Counter/Timers 		*/
#define CPE_WHTDOG_BASE                 0x98500000      /*   Watchdog Timer 		*/
#define CPE_RTC_BASE                    0x98600000	 	/*   Real Time Clock 		*/
#define CPE_GPIO_BASE                   0x98700000      /*   GPIO 					*/
#define CPE_IC_BASE                     0x98800000		/*   Interrupt Controller 	*/
#define CPE_UART3_BASE                  0x98900000      /*   UART 3  				*/
#define CPE_I2C_BASE                    0x98A00000      /*   I2C 					*/
#define CPE_SSP1_BASE                   0x98B00000      /*   SSP1  					*/
#define CPE_CF_BASE                    	0x98D00000      /*   CF   					*/ 
#define CPE_SD_BASE                    	0x98E00000      /*   SD   					*/ 
#define CPE_SMMC_BASE                   0x98F00000      /*   SMC  					*/ 
#define CPE_I2S_BASE                    0x99400000      /*   I2S	 				*/
#define CPE_AC97_BASE                   0x99400000      /*   AC97	 				*/
#define CPE_UART4_BASE                  0x99600000      /*   SSP2   				*/
#define CPE_MAC_BASE                    0x90900000   //RISC platform,new
#define CPE_PMU_BASE                    0x98100000
#define CPE_IRDA_BASE                   0x98900000
#define CPE_FUSB220_BASE				0x90b00000
#define CPE_TIMER1_BASE                 CPE_CT_BASE
#define CPE_TIMER2_BASE                 (CPE_CT_BASE + 0x10)
#define CPE_TIMER3_BASE                 (CPE_CT_BASE + 0x20)
#define CPE_A321_IRDA_BASE                0xb0900000
#define CPE_A321_I2S_BASE                 0xb1400000
#define CPE_A321_APB_BASE                 0x90e00000


#else
/************************************* CPE110 ************************************/
#define CPE_DECODER_BASE                0x90100000
#define CPE_SRAMC_BASE                  0x90200000
#define CPE_SDRAMC_BASE                 0x90300000
#define CPE_DMA_BASE					0x90400000
#define CPE_APB_BASE                	0x90500000
#define CPE_PWM_BASE                    0x98100000      /*   Power Management 		*/
#define CPE_UART1_BASE                  0x98200000	 	/*   UART 1 				*/
#define CPE_UART2_BASE                  0x98300000	 	/*   UART 2/IrDA 			*/
#define CPE_CT_BASE                     0x98400000	 	/*   Counter/Timers 		*/
#define CPE_WHTDOG_BASE                 0x98500000      /*   Watchdog Timer 		*/
#define CPE_RTC_BASE                    0x98600000	 	/*   Real Time Clock 		*/
#define CPE_GPIO_BASE                   0x98700000      /*   GPIO 					*/
#define CPE_IC_BASE                     0x98800000		/*   Interrupt Controller 	*/
#define CPE_UART3_BASE                  0x98900000      /*   UART 3  				*/
#define CPE_I2C_BASE                    0x98A00000      /*   I2C 					*/
#define CPE_SSP1_BASE                   0x98B00000      /*   SSP1  					*/
#define CPE_CF_BASE                    	0x98D00000      /*   CF   					*/ 
#define CPE_SD_BASE                    	0x98E00000      /*   SD   					*/ 
#define CPE_SMMC_BASE                   0x98F00000      /*   SMC  					*/ 
#define CPE_I2S_BASE                    0x99400000      /*   I2S	 				*/
#define CPE_AC97_BASE                   0x99400000      /*   AC97	 				*/
#define CPE_SSP2_BASE                   0x99600000      /*   SSP2   				*/
#define CPE_KBD_BASE                    0x99300000	 	/*   Keyboard  				*/
#define CPE_MOUSE_BASE                  0x99b00000	 	/*   Mouse  				*/
#define CPE_MMSC_BASE                   0x99900000	 	/*   Memory Stick			*/
#define CPE_MAC_BASE				    0x96700000
#define CPE_PMU_BASE                    0x98100000
#define CPE_IRDA_BASE                   0x98200000
#define CPE_FUSB220_BASE				0x96700000
#define CPE_TIMER1_BASE                 CPE_CT_BASE
#define CPE_TIMER2_BASE                 (CPE_CT_BASE + 0x10)
#define CPE_TIMER3_BASE                 (CPE_CT_BASE + 0x20)
#endif


//#define SYS_CLK                     	(12288000*3)	//18432000//22118400
#define SYS_CLK                     	(33000000)	//18432000//22118400
#define AHB_CLK                     	SYS_CLK
#define MAX_TIMER                   	3
#define APB_CLK                     	(SYS_CLK / 2)	
#define DEFAULT_TICK                	1

//ivan for FPGA testing mode
//#define UART_CLOCK   22118400 
#define UART_CLOCK   18432000 
/*  System Clock=22.1184MHZ  FCLK=System Clock/2   PCLK=System Clock/2
 *  UART clock =22118400/2	 DLL= UART clock/(16 * baud) 
 */

//#if (SYS_CLK == 22118400)
#define CPE_BAUD_115200                 (UART_CLOCK / 1843200)
#define CPE_BAUD_57600                  (UART_CLOCK / 921600) 
#define CPE_BAUD_38400				  	(UART_CLOCK / 614400)
#define CPE_BAUD_19200                  (UART_CLOCK / 307200)
#define CPE_BAUD_14400                  (UART_CLOCK / 230400)
#define CPE_BAUD_9600                   (UART_CLOCK / 153600)
#define CPE_BAUD_4800                   (UART_CLOCK / 76800)
#define CPE_BAUD_2400                   (UART_CLOCK / 38400)
#define CPE_BAUD_1200                   (UART_CLOCK / 19200)

//#endif

/*#if (SYS_CLK == 18432000)
 
//CPE_BAUD_115200    can't divide
#define CPE_BAUD_57600                  (APB_CLK / 921600) 
#define CPE_BAUD_38400				  	(APB_CLK / 614400)
#define CPE_BAUD_19200                  (APB_CLK / 307200)
#define CPE_BAUD_14400                  (APB_CLK / 230400)
#define CPE_BAUD_9600                   (APB_CLK / 153600)
#define CPE_BAUD_4800                   (APB_CLK / 76800)
#define CPE_BAUD_2400                   (APB_CLK / 38400)
#define CPE_BAUD_1200                   (APB_CLK / 19200)
#endif*/

/*#if (SYS_CLK == 10000000)


#define CPE_BAUD_9600                   (APB_CLK / 153600)
#define CPE_BAUD_4800                   (APB_CLK / 76800)
#define CPE_BAUD_2400                   (APB_CLK / 38400)
#define CPE_BAUD_1200                   (APB_CLK / 19200)
#endif*/


#define HOST_COMPORT                    CPE_UART1_BASE  
#define HOST_IRQBIT_NUMBER              IRQ_UART1       

/* Default port for use by Operating System or program  */
#define OS_COMPORT                      CPE_UART2_BASE
#define OS_IRQBIT_NUMBER                IRQ_UART2     

#define DEBUG_COMPORT                   OS_COMPORT
#define DEBUG_IRQBIT                    OS_IRQBIT
#define MAX_UART_NUM                    2	

/* Values to set given baud rates */
#define DEFAULT_HOST_BAUD               CPE_BAUD_38400


/* NUMBERIC DISPLAY LED	*/
#define CPE_DBG_ALPHA                   0x902ffffc

#define RISC_AC97PinSel_Bit    3
#define AC97ClkSel     		   4
#define I2sClkSel              5
#define SspClkSel              6
#define UsbClkSel              7
#define UartClkSel             8
#define IrdaClkSel             9
#define Pwm0PinSel             10
#define Pwm1PinSel             11
#define IrdaPinSel             12
#define AC97ClkOutSel          13
#define ModemPinSel            14
#define Dma1PinSel             15
#define Dma0PinSel             16



#define FPLAT_CPE120_ID     0x03200000
#define FPLAT_CPE120_ID2     0x03200010

/* clock source */
#define  INTERNAL_CLK_SORCE     0x0
#define  EXTERNAL_CLK_SORCE     0x1


/* core definition */
#define CORE_FA51X  1
#define CORE_FA52X  2

#endif /* __CPE_H*/
