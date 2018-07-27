/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: cir.c
* Description	: 
*		Handle device driver
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	07/13/2006	Middle Huang	Create
*
****************************************************************************/
#ifndef _CIR_H_
#define _CIR_H_

#define VCR_KEY_POWER		0x613E609F
#define TV1_KEY_POWER		0x40040100
#define TV1_KEY_POWER_EXT	0xBCBD
#define RC5_KER_POWER		0x0CF3

#define VCC_H_ACT_PER		(16-1)
#define VCC_L_ACT_PER		(8-1)
#define VCC_DATA_LEN		(32-1)
#define TV1_H_ACT_PER		(8-1)
#define TV1_L_ACT_PER		(4-1)
#define TV1_DATA_LEN		(48-1)

#define VCC_BAUD		540
#define TV1_BAUD		430

#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)
#define	EXT_CLK			60
#else
#define	EXT_CLK			20
#endif

#define	NEC_PROTOCOL	0x0
#define	RC5_PROTOCOL	0x1
#define VCC_PROTOCOL	0x0
#define TV1_PROTOCOL	0x01

#define STORLINK_CIR_ID		0x00010400


#define	CIR_IP_ID			(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x00))
#define	CIR_CTR_REG			(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x04))
#define	CIR_STATUS_REG		(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x08))
#define	CIR_RX_REG			(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x0C))
#define	CIR_RX_EXT_REG		(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x10))
#define	CIR_PWR_REG			(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x14))
#define	CIR_PWR_EXT_REG		(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x18))
#define	CIR_TX_CTR_REG		(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x1C))
#define	CIR_TX_FEQ_REG		(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x20))
#define	CIR_TX_REG			(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x24))
#define	CIR_TX_EXT_REG		(*(volatile unsigned long  * const)(SL2312_CIR_BASE+0x28))

#define	PWR_CTRL_ID			(*(volatile unsigned long  * const)(SL2312_POWER_CTRL_BASE+0x00))
#define	PWR_CTRL_REG		(*(volatile unsigned long  * const)(SL2312_POWER_CTRL_BASE+0x04))
#define	PWR_STATUS_REG		(*(volatile unsigned long  * const)(SL2312_POWER_CTRL_BASE+0x08))

#define BIT(x)			(1<<x)
#define TX_STATUS		BIT(3)

#define	PWR_STAT_CIR		0x10
#define	PWR_STAT_RTC		0x20
#define	PWR_STAT_PUSH		0x40
#define	PWR_SHUTDOWN		0x01

#define CARR_FREQ		38000

#endif  // _CIR_H_
