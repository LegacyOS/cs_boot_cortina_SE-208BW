
/* DO NOT EDIT!! - this file automatically generated
 *                 from .s file by awk -f s2h.awk
 */

 
/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name:except.s                                                            *
* Description: excpetion define                                            *
* Author: Fred Chien                                                       *
****************************************************************************/ 


#define T_BIT                           0x20
#define F_BIT                           0x40
#define I_BIT                           0x80
 

#define NoINTS                          (I_BIT | F_BIT) 
#define MskINTS                         NoINTS


#define AllIRQs                         0xFF		 /*  Mask for interrupt controller */


#define ResetV                          0x00
#define UndefV                          0x04
#define SwiV                            0x08
#define PrefetchV                       0x0c
#define DataV                           0x10	
#define IrqV                            0x18
#define FiqV                            0x1C


#define ModeMask                        0x1F		 /* Processor mode in CPSR */



#define SVC32Mode                       0x13
#define IRQ32Mode                       0x12
#define FIQ32Mode                       0x11
#define User32Mode                      0x10
#define Sys32Mode                       0x1F

/* Error modes */

#define Abort32Mode                     0x17
#define Undef32Mode                     0x1B


#define UserStackSize	        0x8000
#define SVCStackSize		0x140000
#define IRQStackSize		0xA0000
#define UndefStackSize		0x8000
#define FIQStackSize		0x8000
#define AbortStackSize		0x8000

#define HeapSize                0x200000
 

/* 	END	 */


