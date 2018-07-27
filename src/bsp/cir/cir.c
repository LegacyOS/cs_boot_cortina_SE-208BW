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

#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include "cir.h"


void cir_init(int mode);

/*----------------------------------------------------------------------
* 	flash_init
*----------------------------------------------------------------------*/
int sl_cir_init(void)
{
	if((CIR_IP_ID&0xFFFFFF00)!=STORLINK_CIR_ID){
		printf("Cortina IR Module Not Found!!\n");
		return -1;							// Storlink CIR not found!!
	}
	
	

	if(CIR_STATUS_REG&BIT(2))		// First Power On
	{
		printf("Cortina CIR Initialization\n");
		printf("Please reboot now.");
		cir_init(TV1_PROTOCOL);
	}
	
}


void cir_init(int mode)
{
	unsigned int baud;
	unsigned int reg_v;
	
	if(mode==VCC_PROTOCOL) {			// VCR-33
		/*=================== Set RX/TX baud rate ==================*/
		baud = VCC_BAUD*EXT_CLK ;
		CIR_CTR_REG &= 0x0000FFE8 ;
		CIR_CTR_REG |= (baud << 16) |BIT(4);
		CIR_TX_CTR_REG = 0x0 ;
		CIR_CTR_REG |= (baud << 16);
		
	
		/*=================== Set Power key ==================*/
		CIR_PWR_REG = VCR_KEY_POWER;
		CIR_PWR_EXT_REG = 0;
		
		/*=============== Set Carrier Frequency ==============*/
		CIR_TX_FEQ_REG = (unsigned int)(EXT_CLK/CARR_FREQ);
	
		CIR_CTR_REG &= 0xFFFF00FF;
		CIR_CTR_REG |= (VCC_H_ACT_PER<<12)|(VCC_L_ACT_PER<<8);
	
		CIR_STATUS_REG &= 0xFFC0FFFF;
		CIR_STATUS_REG |= VCC_DATA_LEN<<16 ;

		CIR_TX_CTR_REG &= 0xFFFF0000;					// Set TX para
		CIR_TX_CTR_REG |= (VCC_H_ACT_PER<<12)|(VCC_L_ACT_PER<<8)|(VCC_DATA_LEN);

		CIR_TX_FEQ_REG = (unsigned int)(EXT_CLK/CARR_FREQ);
	
	}
	else if(mode==TV1_PROTOCOL) {			// TV1-26
		/*=================== Set RX/TX baud rate ==================*/
		baud = TV1_BAUD*EXT_CLK ;
		CIR_CTR_REG &= 0x0000FFE8 ;
		CIR_CTR_REG |= (baud << 16) | BIT(4);
		CIR_TX_CTR_REG = 0x0 ;
		CIR_CTR_REG |= (baud << 16);
	
		/*=================== Set Power key ==================*/
		CIR_PWR_REG = TV1_KEY_POWER;
		CIR_PWR_EXT_REG = TV1_KEY_POWER_EXT;
	
		/*=============== Set Carrier Frequency ==============*/
		CIR_TX_FEQ_REG = (unsigned int)(EXT_CLK/CARR_FREQ);
	
		CIR_CTR_REG &= 0xFFFF00FF;
		CIR_CTR_REG |= (TV1_H_ACT_PER<<12)|(TV1_L_ACT_PER<<8);

		CIR_STATUS_REG &= 0xFFC0FFFF;
		CIR_STATUS_REG |= TV1_DATA_LEN<<16 ;

		CIR_TX_CTR_REG &= 0xFFFF0000;					// Set TX para
		CIR_TX_CTR_REG |= (TV1_H_ACT_PER<<12)|(TV1_L_ACT_PER<<8)|(TV1_DATA_LEN);

		CIR_TX_FEQ_REG = (unsigned int)(EXT_CLK/CARR_FREQ);
	}
#if 0	
	CIR_CTR_REG |= BIT(0);		// Initial complete
	CIR_STATUS_REG &= ~BIT(1);	// Clear Interrupt
	hal_delay_us(10*1000);
	
	reg_v = PWR_CTRL_REG;
	hal_delay_us(10*1000);
	PWR_CTRL_REG = reg_v | BIT(2) |BIT(1);	// Clear cir interrupt;
	hal_delay_us(10*1000);
	reg_v &= ~BIT(1);
	reg_v |= BIT(0);
	PWR_CTRL_REG = reg_v|BIT(2);	// Power off machine because of first power on
	
#endif	

}

